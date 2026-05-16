//this includes quarter & dotted eighth modes
//includes tap, filter, modulation
//includes reverb

#include "daisy_seed.h"
#include "daisysp.h"
#include "daisy_petal.h"
#include "terrarium.h"

using namespace daisy;
using namespace daisysp;
using namespace terrarium;

//The terrarium.h  class re-maps the inputs of the DaisyPetal hardware patch. 
DaisyPetal hw;

Led led1, led2;
//Set the names of the direct knob processes.
Parameter knob_time, knob_fdbk, knob_mix, knob_tone, knob_mod, knob_rvb;
//Set the names of the varibles that read from the knobs.
float read_time_knob, read_fdbk_knob, read_mix, read_tone, read_mod, read_rvb;
float read_time_knob_prev, var_time, var_fdbk, rvb_decay, rvb_mix;

float del_out, rvb_outL, rvb_outR;

bool bypass = true;
float led_counter = 0.0f;
float tap_counter = 0.0f;
bool tap_flag = false;

float sample_rate;
int blocksize;
float increment;

static Oscillator lfo;  //for modulation

// Reverb
ReverbSc        verb;  //must turn on LGPL in the makefile to use this class.

// Set max delay time to 1 second
#define MAX_DELAY static_cast<size_t>(48000 * 1.0f) 
DelayLine<float, MAX_DELAY> DSY_SDRAM_BSS delayLine; // I don't know if SDRAM is needed...

struct delay
{
    DelayLine<float, MAX_DELAY> *del; //should be Static??
    //external input variables
    float delaytime, delaytimesmooth, feedback, dly_ratio, lpf_k2, mod_depth;
    bool active, hpf_active;
	//internally calculated variables
    float read_dry, read, lpf_temp, hpf_temp, read_lpf, read_hpf;
	float lfo_val, mod_delay_time;

    float Process(float in)
    {
        //set delay times
        lfo_val = lfo.Process(); // LFO range -1 to 1
        fonepole(delaytimesmooth, delaytime, .0002f); //smooth delaytime changes
		mod_delay_time = (delaytimesmooth * dly_ratio) + (lfo_val * 36.0f * mod_depth); 

        del->SetDelay(mod_delay_time);
        read_dry = del->Read();
		
		//lpf hard coded (similar to FV-1 instructions)
		read_lpf = (read_dry - lpf_temp) * lpf_k2 + lpf_temp;
		lpf_temp = read_lpf;

		//hpf hard coded (similar to FV-1 instructions)
		if (hpf_active) {
			read_hpf = (read_lpf - hpf_temp) * 0.026 + hpf_temp;  //this is a lpf around 200Hz.
			hpf_temp = read_hpf;
			read = read_lpf - read_hpf; // subtracting the LPF from the input makes an approximate HPF
		} else {
			read = read_lpf;
		} 

        if (active) {
            del->Write((feedback * read) + in);
        } else {
            del->Write(feedback * read); // if not active, don't write any new sound to buffer
        }
        return (read);
    }
};

delay delay1;

void AudioCallback(AudioHandle::InputBuffer in, AudioHandle::OutputBuffer out, size_t size)
{
	hw.ProcessAllControls();
	led1.Update();
    led2.Update();

	read_time_knob = knob_time.Process();
    read_fdbk_knob = knob_fdbk.Process();
    read_mix = knob_mix.Process();
	read_tone = knob_tone.Process();
	read_mod = knob_mod.Process();
    read_rvb = knob_rvb.Process();

    fonepole(var_fdbk, read_fdbk_knob, .0002f); //smooth feedback if it's set to zero during time jump

    if(hw.switches[Terrarium::FOOTSWITCH_1].RisingEdge())
    {
        bypass = !bypass;
        led1.Set(bypass ? 0.0f : 1.0f);
    }
	delay1.active = !bypass;

    if(tap_counter < 1.1f) { //make sure it doesn't keep growing unnecessarily
        tap_counter += increment; // default 0.001 since the audio callback runs at 1ms (48/48000)
    }

    if(hw.switches[Terrarium::FOOTSWITCH_2].RisingEdge()) {
        if(tap_counter < 1.0f) { //check for the 2nd tap within 1 second
            var_time = tap_counter;
	        delay1.delaytimesmooth = (var_time*sample_rate); //update smooth value to skip smoothing
            tap_flag = true;
            read_time_knob_prev = read_time_knob;
            led_counter = 0;
            tap_counter = 0;
            var_fdbk = 0; //clear the feedback if there's a time jump
        }
        tap_counter = 0.0f;   
    }

	//Set the delay time by knob (if applicable)
    if(!tap_flag){
		var_time = read_time_knob; // knob is very responsive, if we are not checking to overwrite tap time
    } else {
        //check if the time knob has moved significantly to overwrite tap
        if(read_time_knob - read_time_knob_prev > 0.05 || read_time_knob - read_time_knob_prev < -0.05) {
            var_time = read_time_knob;
	        delay1.delaytimesmooth = (var_time*sample_rate); //update smooth value to skip smoothing
            tap_flag = false;
            var_fdbk = 0; //clear the feedback if there's a time jump
        }
    }

	led_counter += increment; // default 0.001 (48/48000)
    if(led_counter < var_time * 0.2) //blink lasts 20% of the delay time
    {
        led2.Set(1.0f);
    } else if(var_time > 0.2) { // if var_time is <200ms, don't blink (too fast)
        led2.Set(0.0f);
    } else {
        led2.Set(1.0f);       
    }

    if(led_counter > var_time)
    {
        led_counter = 0.0f;
    }


    //SWITCH 1 down is quarter note, up is dotted eighth
    if(hw.switches[Terrarium::SWITCH_1].Pressed()) {
        delay1.dly_ratio = 0.75f;
    } else {
        delay1.dly_ratio = 1.0f;
    }

    //SWITCH 2 down is hpf OFF, up is hpf ON
	if(hw.switches[Terrarium::SWITCH_2].Pressed()) {
        //delay1.lpf_k2 = 0.123f; // ~ 1000Hz  (math from fv1 site: K1=e-(2*pi*F*t), K2= 1-K1. t=sample period(1/sample rate)
		delay1.hpf_active = true;
    } else {
		delay1.hpf_active = false;
    }
    
    delay1.lpf_k2 = read_tone * read_tone * 0.9 + 0.05;

    //SWITCH 3 down is mod OFF, up is mod ON
	if(hw.switches[Terrarium::SWITCH_3].Pressed()) {
        delay1.mod_depth = read_mod;
    } else {
        delay1.mod_depth = 0.0f;
    }

	//SWITCH 4 down is RVB OFF, up is RVB ON
	if(hw.switches[Terrarium::SWITCH_4].Pressed()) {
		verb.SetFeedback(1-((1-read_rvb)*(1-read_rvb))); //consider modifying equation if needed
		rvb_mix = 1-((1-read_rvb)*(1-read_rvb)); //reverse log taper
	} else {
		verb.SetFeedback(0.0);
		rvb_mix = 0.0;
	}

    //set delayline parameters
    delay1.feedback = var_fdbk;
	delay1.delaytime = (var_time*sample_rate);  // *48000
  
    //This is the audio processing which runs every sample
	for (size_t i = 0; i < size; i++)
	{
        float sig_in = in[0][i];



        // Read from delay line
		float del_out = sig_in + delay1.Process(sig_in) * read_mix;   
		verb.Process(del_out, del_out, &rvb_outL, &rvb_outR);
		
		if(bypass) {
            out[0][i] = sig_in;     // left
        }
        else {
            out[0][i] = del_out + (rvb_outL + rvb_outR) * 0.5 * rvb_mix;     // left
        }
	}
}

int main(void)
{
	hw.Init();
    sample_rate = hw.AudioSampleRate();
    blocksize = 48;
    hw.SetAudioBlockSize(blocksize);
    increment = 0.001f; //blocksize / sample_rate;
    
    //del.Init();
    delayLine.Init();
    delay1.del = &delayLine;
    delay1.delaytime = 2400; // in samples
    delay1.feedback = 0.0f;
    delay1.active = false;     // Default to no delay
    delay1.lpf_temp = 0.0f;
	delay1.lpf_k2 = 0.5f;
	delay1.hpf_active = false;
	delay1.mod_depth = 0.0f;
    delay1.dly_ratio = 1.0f;

	lfo.Init(sample_rate);
    lfo.SetWaveform(Oscillator::WAVE_SIN);
    lfo.SetFreq(2.3f); // 2Hz chorus speed
    lfo.SetAmp(1.0f);  // Full range modulation

	verb.Init(sample_rate);
	verb.SetFeedback(0.0);
    verb.SetLpFreq(10000.0); 

    // Initialize your knobs here like so:
    knob_time.Init(hw.knob[Terrarium::KNOB_1], 0.05f, 1.0f, Parameter::LINEAR);
    knob_fdbk.Init(hw.knob[Terrarium::KNOB_2], 0.0f, 1.0f, Parameter::LINEAR);
    knob_mix.Init(hw.knob[Terrarium::KNOB_3], 0.0f, 1.4f, Parameter::LINEAR); // over-boost allowed
    knob_tone.Init(hw.knob[Terrarium::KNOB_4], 0.0f, 1.0f, Parameter::LINEAR);
    knob_mod.Init(hw.knob[Terrarium::KNOB_5], 0.0f, 1.0f, Parameter::LINEAR);
    knob_rvb.Init(hw.knob[Terrarium::KNOB_6], 0.0f, 1.0f, Parameter::LINEAR);

    // Init the LEDs and set activate bypass
    led1.Init(hw.seed.GetPin(Terrarium::LED_1),false);
	led2.Init(hw.seed.GetPin(Terrarium::LED_2),false);
    led1.Update();
	led2.Update();

    hw.StartAdc();
    hw.StartAudio(AudioCallback);

	while(1) {}
}

