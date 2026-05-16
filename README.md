# Daisy-Tap-Delay
This is a tap-tempo delay for the Daisy Seed, using the pedalpcb Terrarium build.  Includes 1/4 - dotted 1/8 modes, HPF and LPF filters, modulation, and Reverb (using the ReverbSc class from DaisySp library).

Some details that I find help the performance:
-Smoothing on the Time control so it doesn't sound glitchy (this is standard on DSP delay code I've seen)
-The Smoothing is bypassed when a change is detected from tap tempo or moving the knob to overwrite the tap tempo. 
-At the same time, the feedback is temporarily zeroed.
These operations get rid of annoying time change sounds repeating when you use tap.

The tap tempo is very simple, it just uses the last 2 taps (within 1 second).  For the tap tempo and blinking LED, I use a counter that increments each audio-callback loop (in my case, once per 0.001 seconds).  If you modify the block size, be sure to update this "increment" variable.

Shout out to GuitarML, whose projects and posted code were very helpful and inspiring.
