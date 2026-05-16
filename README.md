# TapDelayProject

(Post in progress....)

This is a tap tempo effect for the pedalpcb Terrarium.  It includes 1/4 - dotted 1/8 modes, HP and LP filters, modulation, and reverb (using the ReverbSc class from DaisySP library).  

The Tap Tempo is quite simple, and it just recognizes the last 2 clicks (within 1 second).  For the blinking LED and Tap Tempo, I use a counter that runs each audio-callback loop, so in my case it's 0.001.  If you change the block size, be sure to update this "increment" variable.

I worked out a couple of details which I think make the tap tempo operate more smoothly:<br>
-The Time knob has a smoothing filter so it doesn't sound glitchy when you change the time (this is standard on most DSP delay code).<br>
-The smoothing is bypassed when a tap change is detected (the official delay time automatically "catches up" with the target time).<br>
-The feedback is temporarily zeroed.<br>
These processes help avoid annoying time change sounds repeating when you use tap to change the delay time.

I used the terrarium.h class, which after some puzzling I realized is just a modifier for re-mapping pins of the daisy_petal.h class from libDaisy.  If you are compiling this, make sure the DaisySP and libDaisy folder paths are setup correctly in your MakeFile.  (In my folder structure, they are 2 folders back).  The MakeFile needs to have the USE_DAISYSP_LGPL = 1 command in order to use the ReverbSc class.

I have also posted the .bin which you can directly flash via the Electrosmith-Web-Programmer online tool.

I'm new to Daisy after years working on FV-1 code.  GuitarML projects and code were a huge resource for getting up to speed on the Daisy.


## Layout
Knob1 = Time<br>
Knob2 = Feedback<br>
Knob3 = Mix (Delay Level)<br>
Knob4 = Tone (LPF cutoff frequency)<br>
Knob5 = Modulation Depth<br>
Knob6 = Reverb (Decay time and Reverb Level both controlled by this knob)

SW1 = Subdiv (down=1/4, up=dotted 1/8)<br>
SW2 = HPF (down=off, up=on).  The HPF makes the repeats fade away in the distance.<br>
SW3 = Modulation (down=off, up=on)<br>
SW4 = Reverb (down=off, up=on)

FTSW 1 (left) = Bypass/On<br>
FTSW 2 (right) = Tap Tempo

LED1 = Effect On indicator<br>
LED2 = Delay rate blinking.  Below 200ms, it stays solid.
