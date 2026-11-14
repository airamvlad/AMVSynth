# AMVSynth

A free synthesizer plugin that brings Serbian and Balkan folk sounds into psytrance. All sound is synthesized (wavetables and physical modelling), with no samples.

Status: early skeleton. The plugin currently has one voice type, the Južni Vetar style keyboard lead, with a filter, two envelopes, glide, delayed vibrato and pitch bend.

## Formats

- VST3 for Windows and macOS (the main target, tested in Bitwig)
- AU for macOS
- Standalone app for quick testing

## Download a build

Every change is built automatically. Open the **Actions** tab, pick the latest successful **Build** run and download `AMVSynth-Windows` or `AMVSynth-macOS` from the Artifacts section.

Install the VST3:

- Windows: copy `AMVSynth.vst3` to `C:\Program Files\Common Files\VST3`
- macOS: copy `AMVSynth.vst3` to `~/Library/Audio/Plug-Ins/VST3`

The macOS build is not signed yet. If macOS blocks it, run this once in Terminal:

```
xattr -dr com.apple.quarantine ~/Library/Audio/Plug-Ins/VST3/AMVSynth.vst3
```

## Build from source

Requires CMake 3.22 or newer and a C++20 compiler (Visual Studio 2022 or Xcode). JUCE is downloaded automatically.

```
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
```

The results are in `build/AMVSynth_artefacts/Release`.

## Layout

- `Source/Parameters.h`: all plugin parameters
- `Source/SynthEngine.h`: voices, MIDI handling and DSP
- `Source/PluginProcessor.*`: the plugin itself
- `Source/PluginEditor.*`: the interface

Built with [JUCE](https://juce.com).
