# AMVSynth

A free synthesizer plugin that brings Serbian and Balkan folk sounds into psytrance. The instruments are synthesized (wavetables and physical modelling), with no sample library; a sampler with a granular mode is there for your own recordings.

What it does:

- Seventeen models in the Balkan engine: Južni Vetar keyboard lead, accordion, gusle, frula, kaval, gajde with a drone on the scale root, zurla, tambura (plucked string), tapan (drum), dvojnice (double flute), clarinet, truba (brass), violin, šargija with a drone string, kanun with tremolo picking, darbuka and a sung voice
- Two wavetable oscillators (Basic Shapes, Acid Saw, PWM, Vowels, Kaval, Zurla, Reeds, Bells) with up to 7 voice unison spread in stereo, plus a sub oscillator and noise, on their own page and mixed with the Balkan engine
- Drag and drop modulation: drag a source from the MOD SOURCES row onto a knob, shift-drag the knob to set the depth, right-click to invert or remove. Knobs show a ring for each modulation and a dot at the value as it moves
- Each modulation is unipolar or bipolar: Auto keeps LFOs swinging both ways and everything else one way, and you can force either from the MOD page or the right-click menu of a knob
- Four LFOs with a shape editor: set an LFO to Custom and draw it with points and curves on a 16-step grid, or start from stairs, a sidechain pump or random steps. Each LFO runs free, retriggers with every note, or runs once like an envelope
- A third envelope (ENV 3) that is only a modulation source
- Envelope times and the effects are modulation destinations too: amp and filter attack, decay and release, distortion drive and mix, gate depth, delay feedback, tone and mix, reverb size and mix, the bitcrusher rate and the chorus, flanger and phaser mix
- Warp on both wavetable oscillators (Bend +, Bend -, PWM, Sync, Mirror) with a WARP knob that can be modulated, and a 3D view of all frames of the table with the playing position marked
- FM, ring and amplitude modulation of each wavetable oscillator by the other oscillator, the sub, the noise, the Balkan engine or the sampler. The modulator works even with its own level turned down, and the amount is a modulation destination
- Four macros on the main page, named by the preset: every factory preset wires them (BRIGHT or DARK, EDGE, VIBRATO, TIMBRE unless the preset uses a macro for something of its own), and a double click on a name changes it
- Modulation on its own page: four LFOs (free or tempo-synced), a third envelope, four macros and an eight slot matrix with LFOs, envelopes, velocity, mod wheel, macros, pressure and timbre as sources
- The Balkan models can be stacked: up to five copies per note as a detuned unison spread left and right, or as octaves, fifths and minor, major or sus4 chords from one key
- 54 factory presets for the folk models and for psytrance basses, leads, pads, arps and sequences, plus saving and loading your own presets as files
- Preset browser (BROWSE in the header): the factory presets by category, your own presets from the `AMVSynth Presets` folder in Documents (subfolders included), a search box and favourites. A click or the arrow keys load a preset straight away, so you can play through the list
- Odd-meter arpeggiator with accent groups (5/8, 7/8, 9/8, 11/8), synced to the host tempo, with a polymeter mode that lets the pattern cycle over a 4/4 bar
- Odd-meter step sequencer on the SEQ page: up to 32 steps, each with its own note (two octaves around the held key, which transposes the pattern), velocity, gate and slide
  - The grid is grouped like the chosen meter (5/8, 7/8, 9/8, 11/8 or 4/4), and 1 BAR and 2 BARS set the length to it; any length from 1 to 32 steps works
  - A slide glides into the next step in Mono Legato, and ties two steps with the same note into one long note
  - Swing, a gate scale, latch, and a free cycle mode that lets an odd pattern run over a 4/4 bar, or restart on every bar
  - Four patterns, A to D, per sound. The pattern is chosen on the page or by automation, with KEY SWITCH also by the MIDI keys C0 to D#0 while you play, and CHAIN plays up to eight of them one after the other
  - Pattern tools: accents from the meter, random notes from the chosen scale, shift, reverse, clear, copy and paste. The patterns and the chain are saved with the project and in preset files
- Twelve effects on their own page, laid out in the order the sound runs through them: compressor, bitcrusher, distortion (tube, fold, crush; tube and fold run oversampled), trance gate with odd-meter patterns, chorus, flanger, phaser, tempo-synced ping-pong delay, reverb, three-band EQ with a movable mid band, kick duck and an output limiter, with a choice of order for distortion, gate, delay and reverb and an output level meter
- Scales with quarter tones (Hijaz, Hijazkar, Nikriz, Rast, Ussak, Huseyni, Saba and more), with optional snap so every key plays a scale note
- Pythagorean tuning (all twelve keys from pure fifths on the chosen root) in the same scale list
- Wavetable editor on the WAVE page: draw each of the 16 frames by hand (free or on a step grid), build it from 48 harmonic bars, start from basic shapes or a copy of a built-in table, smooth, normalize, invert, reverse, copy and paste frames, fill the table, morph from the first frame to the selected one, undo, and export the result as a WAV
- Sampler on the SAMPLE page, mixed with the oscillators and the Balkan engine before the filter. Load or drop a WAV, AIFF, FLAC, MP3 or OGG file (up to 60 seconds); it is saved inside the project as FLAC
  - Classic mode: root note, key tracking, start and end, forward and ping pong loops with a crossfade, reverse, pitch, fine and pan
  - Granular mode: grain position, size, density, position spray, pitch spray, shape, stereo spread, reverse chance, and SCAN, which moves the position on its own so the sample can be frozen, stretched or played backwards at any pitch
  - Markers are dragged in the wave display, which also shows where the playing note is reading
  - Tools: normalize, crop, fade edges, find the root note from the pitch, and cut single cycles into the User wavetable of oscillator A or B
  - Sampler level and pitch and the grain position, size, density, spray and scan are modulation destinations
- Import of your own wavetables on both oscillators: a WAV, AIFF or FLAC file with a row of 2048-sample frames, or a single cycle of any length. The table is saved inside the project and inside presets you save
- Ornaments played automatically: grace note, mordent, trill and fall
- Mono legato or 16 voices, glide, delayed vibrato, pitch bend
- Two filters with nine types each (low pass and high pass at 12 and 24 dB, band pass, notch, comb, formant, phaser). Filter 2 runs after filter 1 or beside it, and the Balkan engine, each oscillator, the sub, the noise and the sampler each choose their filter
- Two envelopes that are drawn as curves and edited by dragging or with their A, D, S, R knobs
- Reference pitch from 400 to 480 Hz (type 432 for A4 = 432 Hz); it belongs to the project and stays when you change presets
- MPE: per-note pitch bend, pressure and timbre on channels 2 to 16, so Bitwig's Micro-pitch and MPE controllers work. In Bitwig switch on "Use MPE" for the plugin and leave the bend range at 48

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
- `Source/Sequencer.h`: the step sequencer; `Source/StepEditor.*` is its grid
- `Source/PresetBrowser.*`: the preset browser
- `Source/Effects.h`: the effect chain
- `Source/Wavetables.h`: the wavetables, built from harmonic spectra at startup
- `Source/WavetableEditor.h`, `.cpp`: the WAVE page
- `Source/Sampler.h`: the sample, the classic and granular players and the sample tools
- `Source/SampleDisplay.h`, `.cpp`: the wave display of the SAMPLE page
- `Source/Modulation.h`: LFOs and the modulation sources and destinations
- `Source/Presets.h`: the factory presets
- `Source/SynthEngine.h`: voices, ornaments, MIDI handling and DSP
- `Source/PluginProcessor.*`: the plugin itself
- `Source/PluginEditor.*`: the interface
- `tests/PresetTests.cpp`: plays every factory preset through the plugin
- `tests/EngineTests.cpp`: offline checks, run with `ctest --test-dir build -C Release`

## License

AMVSynth is free software, released under the [GNU Affero General Public License v3](LICENSE). It is built with [JUCE](https://juce.com), which is used here under the same license.
