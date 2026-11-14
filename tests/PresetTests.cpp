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

    std::printf ("%d failure(s)\n", failures);
    return failures == 0 ? 0 : 1;
}
