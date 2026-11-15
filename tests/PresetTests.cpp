// Plays every factory preset through the real plugin processor. Exits non-zero on failure.
#include "PluginProcessor.h"

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

    std::printf ("%d failure(s)\n", failures);
    return failures == 0 ? 0 : 1;
}
