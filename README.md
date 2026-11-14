# AMVSynth

A free synthesizer plugin that brings Serbian and Balkan folk sounds into psytrance. All sound is synthesized (wavetables and physical modelling), with no samples.

Status: early development. What works now:

- Six models in the Balkan engine: Južni Vetar keyboard lead, accordion, gusle, frula, kaval and gajde with a drone on the scale root
- Odd-meter arpeggiator with accent groups (5/8, 7/8, 9/8, 11/8), synced to the host tempo, with a polymeter mode that lets the pattern cycle over a 4/4 bar
- Scales with quarter tones (Hijaz, Hijazkar, Nikriz, Rast, Ussak, Huseyni, Saba and more), with optional snap so every key plays a scale note
- Ornaments played automatically: grace note, mordent, trill and fall
- Mono legato or 16 voices, glide, delayed vibrato, pitch bend, low-pass filter and two envelopes

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
- `Source/Tuning.h`: scales and microtonal key mapping
- `Source/Arpeggiator.h`: the odd-meter arpeggiator
- `Source/SynthEngine.h`: voices, ornaments, MIDI handling and DSP
- `Source/PluginProcessor.*`: the plugin itself
- `Source/PluginEditor.*`: the interface
- `tests/EngineTests.cpp`: offline checks, run with `ctest --test-dir build -C Release`

## License

AMVSynth is free software, released under the [GNU Affero General Public License v3](LICENSE). It is built with [JUCE](https://juce.com), which is used here under the same license.
