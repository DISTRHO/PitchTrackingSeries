# DISTRHO PitchTracking Series

A collection of plugins for audio pitch tracking, outputting results as either CV or MIDI,

For now there is only 1 variant - CV - for audio input and CV output (in 1V/Oct range).

# CV

The Audio To CV Pitch plugin is a tool that turns your audio signal into CV pitch and CV gate signals.
This allows audio from instruments (such as guitars) to play and control synth sounds and effects.

It detects the pitch in your incoming audio signal and outputs a 1V/Oct CV pitch signal on the top output.
The bottom output carries the CV gate signal, it sends out 10V while a pitch is detected, and resets to 0V when the pitch can no longer be detected.

The Sensitivity parameter can be increased to detect quieter signals, or decreased to reduce artifacts.
The Octave parameter allows you to shift the detected pitch up or down by a maximum of 4 octaves. When set to 0, it will output the same pitch as is detected on the input.

The plugin also features 3 parameters that are not visible on the plugin interface:
 - The Hold Pitch parameter sets whether the plugin resets its outputs to 0, or holds the last detected pitch.
 - Increase the Confidence Threshold to make sure the correct pitch is being output, or decrease it to get a faster response time.
 - The Tolerance parameter influences how quickly you can change pitch, turn it down for a more accurate pitch output, or turn it up to make it easier to jump from one pitch to the next.
