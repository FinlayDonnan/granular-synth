# Granular Synth

A MIDI-playable granular synthesiser plugin built with [JUCE](https://juce.com/), written as the final project for an Audio Programming course at the University of Edinburgh.

The plugin chops built-in audio samples into short overlapping grains and scatters them in time, pitch and stereo space to simulate natural, textural sounds such as fire, smoke, rain and water. The idea draws on Curtis Roads' analogy between granular synthesis and particle systems in computer graphics, where many tiny particles are combined to render things like flames, clouds and grass that would be costly to model directly.

It is aimed at ambient music and sound design, and at minimalist work in the vein of Luc Ferrari or Terry Riley, though some patches turn out harmonically rich enough to sit in more conventional music.

## Features

- **Granular engine** with 12 voices and up to 128 grains per voice
- **Sample blend** across three built-in samples, crossfading between any two neighbours
- **MIDI-controlled grain position**: notes from C2 (36) to C6 (84) select where in the sample grains are read from
- **Per-grain randomisation**: position spray, random detune and random stereo panning
- **Pitch transpose** of ±12 semitones
- **Effects chain**: tanh soft-clip distortion, feedback delay and reverb (Room / Hall / Church)
- **Seven presets** for REAPER: Fire, Smoke, Cloud, Grass, Water, Rain and Aurora

## Parameters

| Parameter | Range | Default | Notes |
|---|---|---|---|
| Sample blend | 0 – 2 | 1 | 0 = music-box scratch, 1 = drums, 2 = song; values in between mix two samples |
| Volume | 0 – 1 | 0.5 | Overlapping grains get loud quickly, so start low |
| Grain Length | 5 – 500 ms | 100 ms | |
| Grain Density | 0.5 – 100 grains/s | 10 | |
| Grain Position Adjust | −0.01 – 0.01 | 0 | Fine offset on top of the MIDI-selected position |
| Position spray | 1 – 10000 | 10 | Random jitter in where each grain starts |
| Stereo Width | 0 – 1 | 0 | Random pan per grain |
| Pitch transpose | −12 – +12 semitones | 0 | |
| Amount of detune | 0 – 0.25 | 0 | Random pitch variation per grain |
| Distortion level | 1 – 100 | 1 | 1 is effectively clean |
| Delay dry / wet | 0 – 1 | 1 / 0 | |
| Delay time | 1 – 1000 ms | 500 ms | |
| Delay feedback | 0 – 0.99 | 0 | |
| Reverb room size | Room / Hall / Church | Room | |
| Reverb dry / wet / width | 0 – 1 | 1 / 0 / 0 | |

The plugin currently uses JUCE's generic editor, so all parameters appear as a list of sliders in your DAW.

## Project structure

```
granular-synth/
├── AP_Ass3.jucer            Projucer project
├── AP_Ass3_PresetLib.RPL    REAPER preset library
└── Source/
    ├── PluginProcessor.h/.cpp   Parameters, sample loading, synth and effects chain
    ├── PluginEditor.h/.cpp      Default editor (unused; generic editor is used)
    ├── GranularSynth.h          Granular voice and sound classes
    ├── Delay.h                  Feedback delay line
    ├── FileLoader.h             Audio file loading helper
    └── Audio/                   Built-in samples (embedded as binary data)
```

## Future work

- Let MIDI notes control grain length, rate or pitch, not just position
- Add an interface for loading your own samples
- Add an XY pad that places loaded samples on a 2D grid and interpolates between them (custom GUI)

## Credits

- `FileLoader.h` was provided by Tom Mudd as part of the course materials.
- Samples:
  - O'Toole, L. [Liadain O'Toole]. (2025, April 22). Cameron Winter Live at First Unitarian Church | 03/07/2025 [Video]. YouTube. https://youtu.be/lYB-ZZAG0ts?si=23m433Tu0J34o2L0,
  - Dexter [MightyMouseDex]. (2021, April 20). Olivia Rodrigo - deja vu | Drums Only [Video]. YouTube. https://youtu.be/nKuTUnkiAhE?si=VDHx15qyiQDpOcS6, \\
  - Jérémy Chevalier [@jrmychevalier]. (2026, March 26). Music Box 2026, Research for a future installation [Video]. Instagram. https://www.instagram.com/p/DWWrZd7giW_/
- Roads, C. (1988). Introduction to Granular Synthesis. *Computer Music Journal*, 12(2), 11–13.
