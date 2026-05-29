# granular-synth
Audio Programming Final Project

Aim: To build a granular synth which would take in samples and manipulate them to sound like, or at least simulate, natural sounds, hinting towards the analogy between granular synthesis and the technique particle synthesis for computer graphics (‘Introduction to Granular Synthesis', Curtis Roads, 1988).
One use for particle synthesis is to render images such as fire, smoke, clouds, grass, and water, which would otherwise be exceedingly expensive to do using traditional computer graphics modelling techniques, and I wanted to mimic this with my granular synth.

For who: To be used in ambient works and sound design projects and even perhaps minimalist pieces like that of Luc Ferrari or Terry Riley.
I had thought from quite early on that my granular synth wouldn’t be a plugin used in more mainstream, traditionally written music, and so I wasn’t going to worry about it being particularly melody; however some of the sounds did end up being very rich harmonically and could still be used in such a context.

Features: Granular parameters (grain size and length, grain position spray, random detune, and stereo spray), reverb parameters, distortion, delay parameters.

To add: Currently the MIDI notes are mapped to grain position in the sample, but it would be interesting to give the user the option to map incoming MIDI notes to the grain position, grain length, grain rate, or grain pitch.
Add a simple interface that allows users to load their own samples into the granular engine. Perhaps add some kind of XY pad where the loaded samples are positioned on a 2D grid and you can interpolate between them based on the distance of the XY controller from each sample.
