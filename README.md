# AMVSynth

A free synthesizer plugin that brings Serbian and Balkan folk sounds into psytrance. All sound is synthesized (wavetables and physical modelling), with no samples.

What it does:

- Seventeen models in the Balkan engine: Južni Vetar keyboard lead, accordion, gusle, frula, kaval, gajde with a drone on the scale root, zurla, tambura (plucked string), tapan (drum), dvojnice (double flute), clarinet, truba (brass), violin, šargija with a drone string, kanun with tremolo picking, darbuka and a sung voice
- Two wavetable oscillators (Basic Shapes, Acid Saw, PWM, Vowels, Kaval, Zurla, Reeds, Bells) with up to 7 voice unison spread in stereo, plus a sub oscillator and noise, on their own page and mixed with the Balkan engine
- Modulation on its own page: two LFOs (free or tempo-synced), four macros and a six slot matrix with LFOs, filter envelope, velocity, mod wheel and macros as sources
- Factory presets for the folk models and for psytrance basses, arps, leads and pads, plus saving and loading your own presets as files
- Odd-meter arpeggiator with accent groups (5/8, 7/8, 9/8, 11/8), synced to the host tempo, with a polymeter mode that lets the pattern cycle over a 4/4 bar
- Effects on their own page: distortion (tube, fold, crush; tube and fold run oversampled), trance gate with odd-meter patterns, tempo-synced ping-pong delay, reverb, kick duck and an output limiter, with a choice of effect order and an output level meter
- Scales with quarter tones (Hijaz, Hijazkar, Nikriz, Rast, Ussak, Huseyni, Saba and more), with optional snap so every key plays a scale note
- Ornaments played automatically: grace note, mordent, trill and fall
- Mono legato or 16 voices, glide, delayed vibrato, pitch bend, low-pass filter and two envelopes

## Formats

- VST3 for Windows and macOS (the main target, tested in Bitwig)
- AU for macOS
- Standalone app for quick testing

## Download a build

Download `AMVSynth-Windows.zip` or `AMVSynth-macOS.zip` from the latest entry on the **Releases** page.

Every change is also built automatically: open the **Actions** tab, pick the latest successful **Build** run and download `AMVSynth-Windows` or `AMVSynth-macOS` from the Artifacts section. The macOS artifact contains a second zip, which keeps the file permissions intact.

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
- `Source/Effects.h`: the effect chain
- `Source/Wavetables.h`: the wavetables, built from harmonic spectra at startup
- `Source/Modulation.h`: LFOs and the modulation sources and destinations
- `Source/Presets.h`: the factory presets
- `Source/SynthEngine.h`: voices, ornaments, MIDI handling and DSP
- `Source/PluginProcessor.*`: the plugin itself
- `Source/PluginEditor.*`: the interface
- `tests/PresetTests.cpp`: plays every factory preset through the plugin
- `tests/EngineTests.cpp`: offline checks, run with `ctest --test-dir build -C Release`

## License

AMVSynth is free software, released under the [GNU Affero General Public License v3](LICENSE). It is built with [JUCE](https://juce.com), which is used here under the same license.
