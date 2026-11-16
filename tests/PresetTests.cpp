// Plays every factory preset through the real plugin processor. Exits non-zero on failure.
#include "PluginProcessor.h"
#include "PresetBrowser.h"

#include <cstdio>

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter();

int main()
{
    juce::ScopedJuceInitialiser_GUI gui;
    int failures = 0;

    std::unique_ptr<juce::AudioProcessor> plugin (createPluginFilter());
    const double sampleRate = 48000.0;
    const int blockSize = 512;
    plugin->setPlayConfigDetails (0, 2, sampleRate, blockSize);

    for (int program = 0; program < plugin->getNumPrograms(); ++program)
    {
        plugin->setCurrentProgram (program);
        plugin->prepareToPlay (sampleRate, blockSize);

        float peak = 0.0f;
        double energy = 0.0;
        bool finite = true;

        // A three note chord held for a second and a half, then released.
        for (int block = 0; block < 280; ++block)
        {
            juce::MidiBuffer midi;
            for (int note : { 50, 57, 62 })
            {
                if (block == 0)
                    midi.addEvent (juce::MidiMessage::noteOn (1, note, 0.9f), 0);
                if (block == 140)
                    midi.addEvent (juce::MidiMessage::noteOff (1, note), 0);
            }

            juce::AudioBuffer<float> buffer (2, blockSize);
            buffer.clear();
            plugin->processBlock (buffer, midi);

            for (int ch = 0; ch < 2; ++ch)
                for (int i = 0; i < blockSize; ++i)
                {
                    const float s = buffer.getSample (ch, i);
                    finite = finite && std::isfinite (s);
                    peak = juce::jmax (peak, std::abs (s));
                    if (block < 140)
                        energy += (double) s * s;
                }
        }

        const double rms = std::sqrt (energy / (2.0 * 140.0 * blockSize));
        const bool ok = finite && peak <= 1.0f && rms > 0.01 && rms < 0.5;
        std::printf ("%s  %-24s peak %.2f  rms %.3f\n", ok ? "ok  " : "FAIL", plugin->getProgramName (program).toRawUTF8(), (double) peak, rms);
        failures += ok ? 0 : 1;
    }

    // An imported wavetable is saved with the project and comes back in a new instance.
    {
        const auto file = juce::File::getSpecialLocation (juce::File::tempDirectory).getChildFile ("amvsynth-test-table.wav");
        file.deleteFile();

        juce::AudioBuffer<float> audio (1, 2048 * 4);
        for (int i = 0; i < audio.getNumSamples(); ++i)
            audio.setSample (0, i, (float) std::sin (juce::MathConstants<double>::twoPi * (double) ((i / 2048) + 1) * (double) i / 2048.0));

        bool written = false;
        {
            juce::WavAudioFormat wav;
            if (auto stream = std::unique_ptr<juce::OutputStream> (file.createOutputStream()))
                if (auto writer = wav.createWriterFor (stream, juce::AudioFormatWriterOptions().withSampleRate (48000.0).withNumChannels (1).withBitsPerSample (24)))
                    written = writer->writeFromAudioSampleBuffer (audio, 0, audio.getNumSamples());
        }

        auto* first = dynamic_cast<AMVSynthAudioProcessor*> (plugin.get());
        const bool imported = written && first != nullptr && first->importWavetable (1, file) && first->getUserWavetable (1) != nullptr
                                  && first->getUserWavetable (0) == nullptr && ! first->importWavetable (0, file.getSiblingFile ("amvsynth-no-such-file.wav"));

        juce::MemoryBlock state;
        plugin->getStateInformation (state);
        std::unique_ptr<juce::AudioProcessor> other (createPluginFilter());
        other->setStateInformation (state.getData(), (int) state.getSize());
        auto* second = dynamic_cast<AMVSynthAudioProcessor*> (other.get());

        bool restored = imported && second != nullptr && second->getUserWavetable (1) != nullptr
                            && second->getUserWavetable (1)->getName() == "amvsynth-test-table";
        for (int i = 0; restored && i < 2048; ++i)
            restored = std::abs (second->getUserWavetable (1)->wave (15, 0)[i] - first->getUserWavetable (1)->wave (15, 0)[i]) < 1.0e-6f;

        // A factory preset leaves the imported table in place.
        other->setCurrentProgram (3);
        restored = restored && second->getUserWavetable (1) != nullptr;

        std::printf ("%s  imported wavetable is read from a WAV file and saved with the project (state is %d KB)\n",
                     imported && restored ? "ok  " : "FAIL", (int) (state.getSize() / 1024));
        failures += imported && restored ? 0 : 1;
        file.deleteFile();
    }

    // A table made in the editor is heard, saved with the project and exported as a WAV that imports back.
    {
        std::unique_ptr<juce::AudioProcessor> plugin (createPluginFilter());
        auto* first = dynamic_cast<AMVSynthAudioProcessor*> (plugin.get());
        const int size = amv::UserWavetable::tableSize, count = amv::UserWavetable::numFrames;
        std::vector<float> frames ((size_t) (size * count));
        for (int f = 0; f < count; ++f)
            for (int i = 0; i < size; ++i)
                frames[(size_t) (f * size + i)] = std::sin (juce::MathConstants<float>::twoPi * (float) ((f + 1) * i) / (float) size);

        bool ok = first != nullptr;
        if (ok)
        {
            first->setUserWavetableFrames (0, frames, "drawn", true);
            ok = first->getUserWavetable (0) != nullptr && first->getUserWavetable (0)->getName() == "drawn";
        }

        const auto file = juce::File::getSpecialLocation (juce::File::tempDirectory).getChildFile ("amvsynth-export-test.wav");
        file.deleteFile();
        ok = ok && first->exportWavetable (0, file) && ! first->exportWavetable (1, file.getSiblingFile ("amvsynth-none.wav"))
                 && first->importWavetable (1, file) && first->getUserWavetable (1) != nullptr;
        for (int f = 0; ok && f < count; f += 5)
            for (int i = 0; ok && i < size; ++i)
                ok = std::abs (first->getUserWavetable (1)->wave (f, 0)[i] - first->getUserWavetable (0)->wave (f, 0)[i]) < 1.0e-4f;

        juce::MemoryBlock state;
        plugin->getStateInformation (state);
        std::unique_ptr<juce::AudioProcessor> other (createPluginFilter());
        other->setStateInformation (state.getData(), (int) state.getSize());
        auto* second = dynamic_cast<AMVSynthAudioProcessor*> (other.get());
        ok = ok && second != nullptr && second->getUserWavetable (0) != nullptr && second->getUserWavetable (0)->getName() == "drawn";

        std::printf ("%s  an edited wavetable is saved with the project and exports to a WAV that imports back\n", ok ? "ok  " : "FAIL");
        failures += ok ? 0 : 1;
        file.deleteFile();
    }

    // A sample is loaded from a file, saved with the project and removed again.
    {
        const auto file = juce::File::getSpecialLocation (juce::File::tempDirectory).getChildFile ("amvsynth-sample-test.wav");
        file.deleteFile();
        juce::AudioBuffer<float> audio (2, 22050);
        for (int i = 0; i < audio.getNumSamples(); ++i)
        {
            audio.setSample (0, i, 0.7f * std::sin (0.03f * (float) i));
            audio.setSample (1, i, 0.4f * std::sin (0.011f * (float) i));
        }
        {
            juce::WavAudioFormat wav;
            std::unique_ptr<juce::OutputStream> stream (file.createOutputStream());
            if (stream != nullptr)
                if (auto writer = wav.createWriterFor (stream, juce::AudioFormatWriterOptions().withSampleRate (44100.0).withNumChannels (2).withBitsPerSample (24)))
                    writer->writeFromAudioSampleBuffer (audio, 0, audio.getNumSamples());
        }

        std::unique_ptr<juce::AudioProcessor> plugin (createPluginFilter());
        auto* first = dynamic_cast<AMVSynthAudioProcessor*> (plugin.get());
        bool ok = first != nullptr && first->getSample() == nullptr && ! first->loadSample (file.getSiblingFile ("amvsynth-no-sample.wav"))
                      && first->loadSample (file) && first->getSample() != nullptr && first->getSample()->getLength() == 22050
                      && first->getSample()->isStereo() && first->getSample()->getName() == "amvsynth-sample-test";

        juce::MemoryBlock state;
        plugin->getStateInformation (state);
        std::unique_ptr<juce::AudioProcessor> other (createPluginFilter());
        other->setStateInformation (state.getData(), (int) state.getSize());
        auto* second = dynamic_cast<AMVSynthAudioProcessor*> (other.get());
        ok = ok && second != nullptr && second->getSample() != nullptr && second->getSample()->getLength() == 22050
                 && second->getSample()->getSampleRate() == 44100.0 && second->getSample()->getName() == "amvsynth-sample-test";
        for (int i = 0; ok && i < 22050; i += 7)
            ok = std::abs (second->getSample()->getChannel (0)[i] - audio.getSample (0, i)) < 1.0e-4f
                     && std::abs (second->getSample()->getChannel (1)[i] - audio.getSample (1, i)) < 1.0e-4f;

        // Playing it through the whole plugin gives sound.
        if (ok)
        {
            auto set = [second] (const char* paramId, float v) { auto* p = second->apvts.getParameter (paramId); p->setValueNotifyingHost (p->convertTo0to1 (v)); };
            set (amv::id::smpLevel, 1.0f);
            set (amv::id::engineLevel, 0.0f);
            set (amv::id::smpMode, 1.0f);
            other->prepareToPlay (48000.0, 512);
            float peak = 0.0f;
            for (int block = 0; block < 40; ++block)
            {
                juce::AudioBuffer<float> buffer (2, 512);
                juce::MidiBuffer midi;
                if (block == 0)
                    midi.addEvent (juce::MidiMessage::noteOn (1, 60, 0.9f), 0);
                other->processBlock (buffer, midi);
                peak = juce::jmax (peak, buffer.getMagnitude (0, 512));
            }
            ok = peak > 0.05f && peak < 2.0f;
        }

        const int kb = (int) (state.getSize() / 1024);
        if (ok)
        {
            first->setSample (nullptr);
            juce::MemoryBlock empty;
            plugin->getStateInformation (empty);
            ok = first->getSample() == nullptr && empty.getSize() < state.getSize() / 2;
        }

        std::printf ("%s  a sample is loaded, saved with the project (state is %d KB) and removed again\n", ok ? "ok  " : "FAIL", kb);
        failures += ok ? 0 : 1;
        file.deleteFile();
    }

    // A factory preset brings its sequence and its drawn LFO shape, and the next preset takes them away again.
    {
        std::unique_ptr<juce::AudioProcessor> p (createPluginFilter());
        auto* synth = dynamic_cast<AMVSynthAudioProcessor*> (p.get());
        const auto& presets = amv::getFactoryPresets();
        int withSequence = -1, withShape = -1;
        for (size_t i = 0; i < presets.size(); ++i)
        {
            if (withSequence < 0 && ! presets[i].sequence.empty())
                withSequence = (int) i;
            if (withShape < 0 && ! presets[i].lfoShapes.empty())
                withShape = (int) i;
        }

        bool ok = withSequence > 0 && withShape > 0;
        if (ok)
        {
            const auto& sequence = presets[(size_t) withSequence].sequence;
            p->setCurrentProgram (withSequence);
            bool same = synth->apvts.getRawParameterValue (amv::id::seqOn)->load() > 0.5f;
            for (size_t i = 0; i < sequence.size(); ++i)
                same = same && synth->getSeqPattern (0).steps[i].note == sequence[i].note && synth->getSeqPattern (0).steps[i].slide == sequence[i].slide;

            const auto& [lfo, points] = presets[(size_t) withShape].lfoShapes.front();
            p->setCurrentProgram (withShape);
            const bool shaped = synth->getLfoPoints (lfo).size() == points.size();

            p->setCurrentProgram (0);
            bool cleared = synth->getLfoPoints (lfo).size() == 2;
            for (const auto& step : synth->getSeqPattern (0).steps)
                cleared = cleared && step.note == 0 && ! step.slide;

            ok = same && shaped && cleared;
        }

        std::printf ("%s  a factory preset sets its sequence and LFO shape, and Init clears them\n", ok ? "ok  " : "FAIL");
        failures += ok ? 0 : 1;
    }

    // The browser lists by category, by favourites and by the words in the search box.
    {
        using Entry = PresetBrowser::Entry;
        const std::vector<Entry> all { { "Psy Bass", "Bass", 1, {} }, { "Kaval Pad", "Pad", 2, {} }, { "Acid Kolo 7/8 Seq", "Sequence", 3, {} },
                                       { "My Bass", "User", -1, juce::File::getSpecialLocation (juce::File::tempDirectory).getChildFile ("My Bass.amvpreset") } };
        const juce::StringArray favourites { all[1].key(), all[3].key() };
        const auto names = [&] (const juce::String& category, const juce::String& search)
        {
            juce::StringArray result;
            for (const auto& e : PresetBrowser::filter (all, category, search, favourites))
                result.add (e.name);
            return result.joinIntoString ("|");
        };

        const bool ok = names ("All", "") == "Psy Bass|Kaval Pad|Acid Kolo 7/8 Seq|My Bass" && names ("Bass", "") == "Psy Bass" && names ("User", "") == "My Bass"
                        && names ("Favourites", "") == "Kaval Pad|My Bass" && names ("All", "bass") == "Psy Bass|My Bass" && names ("All", "kolo 7/8") == "Acid Kolo 7/8 Seq"
                        && names ("All", "sequence") == "Acid Kolo 7/8 Seq" && names ("Pad", "bass").isEmpty();
        std::printf ("%s  the preset browser filters by category, favourites and search words\n", ok ? "ok  " : "FAIL");
        failures += ok ? 0 : 1;
    }

    // The sequencer pattern is saved with the project, and the sequencer plays it.
    {
        const auto check = [&failures] (bool ok, const char* what)
        {
            std::printf ("%s  %s\n", ok ? "ok  " : "FAIL", what);
            failures += ok ? 0 : 1;
        };

        std::unique_ptr<juce::AudioProcessor> plugin (createPluginFilter());
        auto* first = dynamic_cast<AMVSynthAudioProcessor*> (plugin.get());
        amv::SeqPattern pattern;
        pattern.steps[1].note = 7;
        pattern.steps[2].gate = 0.0f;
        pattern.steps[3].slide = true;
        pattern.steps[3].velocity = 0.35f;
        first->setSeqPattern (0, pattern, true);
        amv::SeqPattern second2;
        second2.steps[0].note = -5;
        first->setSeqPattern (2, second2, true);
        first->setSeqChain ({ 0, 2, 2, 1 });
        first->apvts.getParameter (amv::id::seqOn)->setValueNotifyingHost (1.0f);

        juce::MemoryBlock state;
        plugin->getStateInformation (state);
        std::unique_ptr<juce::AudioProcessor> other (createPluginFilter());
        other->setStateInformation (state.getData(), (int) state.getSize());
        auto* second = dynamic_cast<AMVSynthAudioProcessor*> (other.get());
        const auto& restored = second->getSeqPattern (0);
        check (second->getSeqPattern (2).steps[0].note == -5 && second->getSeqPattern (1).steps[0].note == 0
                   && second->getSeqChain() == std::vector<int> ({ 0, 2, 2, 1 }),
               "the other patterns and the chain are saved with the project");
        check (restored.steps[1].note == 7 && restored.steps[2].gate == 0.0f && restored.steps[3].slide && std::abs (restored.steps[3].velocity - 0.35f) < 0.002f
                   && restored.steps[0].note == 0 && ! restored.steps[0].slide,
               "the sequencer pattern is saved with the project");

        // A held key makes sound and moves the step that is shown.
        other->setPlayConfigDetails (0, 2, 48000.0, 512);
        other->prepareToPlay (48000.0, 512);
        float peak = 0.0f;
        int lastStep = -1;
        for (int b = 0; b < 60; ++b)
        {
            juce::AudioBuffer<float> buffer (2, 512);
            juce::MidiBuffer midi;
            if (b == 0)
                midi.addEvent (juce::MidiMessage::noteOn (1, 57, 0.9f), 0);
            other->processBlock (buffer, midi);
            peak = juce::jmax (peak, buffer.getMagnitude (0, 512));
            lastStep = juce::jmax (lastStep, second->seqStep.load());
        }
        check (peak > 0.01f && lastStep >= 3 && lastStep < 7, "the sequencer plays from a held key and reports its step");
    }

    // Every factory preset has its four macros named and wired; the names follow the preset and the project.
    {
        const auto check = [&failures] (bool ok, const char* what)
        {
            std::printf ("%s  %s\n", ok ? "ok  " : "FAIL", what);
            failures += ok ? 0 : 1;
        };

        const auto& presets = amv::getFactoryPresets();
        bool named = true, wired = true, single = true, resting = true;

        for (size_t i = 1; i < presets.size(); ++i)
        {
            std::array<int, amv::mod::numMacros> sources {};
            std::array<int, amv::mod::numSlots> sourceEntries {};

            for (const auto& [name, value] : presets[i].values)
            {
                for (int slot = 0; slot < amv::mod::numSlots; ++slot)
                    if (name == amv::id::slot (slot, "Source"))
                    {
                        ++sourceEntries[(size_t) slot];
                        const int macro = juce::roundToInt (value) - amv::mod::srcMacro1;
                        if (macro >= 0 && macro < amv::mod::numMacros)
                            ++sources[(size_t) macro];
                    }

                for (int macro = 0; macro < amv::mod::numMacros; ++macro)
                    if (name == amv::id::macro (macro) && value != 0.0f)
                        resting = false;
            }

            for (int macro = 0; macro < amv::mod::numMacros; ++macro)
            {
                named = named && presets[i].macroNames[(size_t) macro].isNotEmpty();
                wired = wired && sources[(size_t) macro] >= 1;
            }

            for (const int entries : sourceEntries)
                single = single && entries <= 1;

            if (! (named && wired && single))
            {
                std::printf ("      first preset with a macro problem: %s\n", presets[i].name);
                break;
            }
        }

        check (named && wired, "every factory preset names and wires its four macros");
        check (single && resting, "the macros use free slots only and rest at zero");

        std::unique_ptr<juce::AudioProcessor> plugin (createPluginFilter());
        auto* first = dynamic_cast<AMVSynthAudioProcessor*> (plugin.get());
        check (first->getMacroName (0) == "MACRO 1" && first->getMacroName (3) == "MACRO 4", "the macros start with their plain names");

        plugin->setCurrentProgram (1);
        bool follows = true;
        for (int macro = 0; macro < amv::mod::numMacros; ++macro)
            follows = follows && first->getMacroName (macro) == presets[1].macroNames[(size_t) macro];
        check (follows, "a preset gives the macros its names");

        first->setMacroName (1, "  growl of the long kind  ");
        check (first->getMacroName (1) == "GROWL OF THE L", "a macro name is trimmed, shortened and in capitals");

        juce::MemoryBlock state;
        plugin->getStateInformation (state);
        std::unique_ptr<juce::AudioProcessor> other (createPluginFilter());
        other->setStateInformation (state.getData(), (int) state.getSize());
        auto* second = dynamic_cast<AMVSynthAudioProcessor*> (other.get());
        check (second->getMacroName (1) == "GROWL OF THE L" && second->getMacroName (0) == presets[1].macroNames[0], "the macro names are saved with the project");

        plugin->setCurrentProgram (0);
        check (first->getMacroName (0) == "MACRO 1" && first->getMacroName (1) == "MACRO 2", "Init gives the macros their plain names back");

        // The wiring reaches the parameters: in Psy Bass the first macro opens the filter from the first slot.
        int bass = -1;
        for (size_t i = 0; i < presets.size(); ++i)
            if (juce::String (presets[i].name) == "Psy Bass")
                bass = (int) i;

        plugin->setCurrentProgram (bass);
        const auto value = [first] (const juce::String& paramId)
        {
            auto* param = first->apvts.getParameter (paramId);
            return param->convertFrom0to1 (param->getValue());
        };
        check (bass > 0 && first->getMacroName (0) == "BRIGHT" && first->getMacroName (3) == "TIMBRE"
                   && juce::roundToInt (value (amv::id::slot (0, "Source"))) == amv::mod::srcMacro1
                   && juce::roundToInt (value (amv::id::slot (0, "Dest"))) == amv::mod::dstCutoff
                   && std::abs (value (amv::id::slot (0, "Amount")) - 0.4f) < 0.01f
                   && juce::roundToInt (value (amv::id::slot (3, "Dest"))) == amv::mod::dstOscAPosition
                   && value (amv::id::macro (0)) == 0.0f,
               "a preset loads its macro wiring into the modulation slots");
    }

    // A drawn LFO shape is saved with the project, and a modulated effect is heard.
    {
        std::unique_ptr<juce::AudioProcessor> plugin (createPluginFilter());
        auto* first = dynamic_cast<AMVSynthAudioProcessor*> (plugin.get());
        bool ok = first != nullptr && first->getLfoPoints (2).size() == 2;
        if (ok)
            first->setLfoShape (2, { { 0.0f, -1.0f, 0.3f }, { 0.25f, 0.5f, 0.0f }, { 1.0f, 1.0f, 0.0f } }, true);

        juce::MemoryBlock state;
        plugin->getStateInformation (state);
        std::unique_ptr<juce::AudioProcessor> other (createPluginFilter());
        other->setStateInformation (state.getData(), (int) state.getSize());
        auto* second = dynamic_cast<AMVSynthAudioProcessor*> (other.get());
        ok = ok && second != nullptr && second->getLfoPoints (2).size() == 3 && std::abs (second->getLfoPoints (2)[1].x - 0.25f) < 1.0e-4f
                 && std::abs (second->getLfoPoints (2)[0].curve - 0.3f) < 1.0e-3f && second->getLfoPoints (0).size() == 2;

        // The level long after the note, with the reverb mix at zero and then opened by a macro.
        const auto tail = [] (bool modulated)
        {
            std::unique_ptr<juce::AudioProcessor> p (createPluginFilter());
            auto& apvts = dynamic_cast<AMVSynthAudioProcessor*> (p.get())->apvts;
            auto set = [&apvts] (const juce::String& paramId, float v) { auto* param = apvts.getParameter (paramId); param->setValueNotifyingHost (param->convertTo0to1 (v)); };
            set (amv::id::reverbOn, 1.0f);
            set (amv::id::reverbMix, 0.0f);
            set (amv::id::reverbSize, 0.9f);
            set (amv::id::macro (0), 1.0f);
            if (modulated)
            {
                set (amv::id::slot (0, "Source"), (float) amv::mod::srcMacro1);
                set (amv::id::slot (0, "Dest"), (float) amv::mod::dstReverbMix);
                set (amv::id::slot (0, "Amount"), 1.0f);
            }

            p->prepareToPlay (48000.0, 512);
            float peak = 0.0f;
            for (int block = 0; block < 120; ++block)
            {
                juce::AudioBuffer<float> buffer (2, 512);
                juce::MidiBuffer midi;
                if (block == 0)
                    midi.addEvent (juce::MidiMessage::noteOn (1, 62, 0.9f), 0);
                if (block == 20)
                    midi.addEvent (juce::MidiMessage::noteOff (1, 62), 0);
                p->processBlock (buffer, midi);
                if (block >= 90)
                    peak = juce::jmax (peak, buffer.getMagnitude (0, 512));
            }
            return peak;
        };
        const float dry = tail (false), wet = tail (true);
        ok = ok && dry < 1.0e-4f && wet > 1.0e-3f && wet < 2.0f;

        std::printf ("%s  a drawn LFO shape is saved with the project and a macro opens the reverb (tail %.5f dry, %.5f modulated)\n",
                     ok ? "ok  " : "FAIL", (double) dry, (double) wet);
        failures += ok ? 0 : 1;
    }

    std::printf ("%d failure(s)\n", failures);
    return failures == 0 ? 0 : 1;
}
