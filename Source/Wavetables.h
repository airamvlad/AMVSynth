#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_dsp/juce_dsp.h>

#include <array>
#include <cmath>
#include <complex>
#include <vector>

namespace amv
{
inline const std::array<const char*, 8>& getWavetableNames()
{
    static const std::array<const char*, 8> names { "Basic Shapes", "Acid Saw", "PWM", "Vowels", "Kaval", "Zurla", "Reeds", "Bells" };
    return names;
}

/** All wavetables, built once from harmonic spectra.

    Each table has 16 frames that the position control morphs through. Every frame is stored at
    10 band-limited levels: level k keeps 1024 >> k harmonics, so a note can always pick a level
    whose highest harmonic stays below half the sample rate. */
class WavetableBank
{
public:
    static constexpr int numTables = 8;
    static constexpr int numFrames = 16;
    static constexpr int numLevels = 10;
    static constexpr int tableSize = 2048;
    static constexpr int maxHarmonics = tableSize / 2;

    /** Builds the bank on first use. Call it once outside the audio thread. */
    static const WavetableBank& get()
    {
        static const WavetableBank bank;
        return bank;
    }

    /** One cycle of tableSize + 1 samples; the last sample repeats the first. */
    const float* wave (int table, int frame, int level) const noexcept
    {
        return data.data() + (size_t) ((table * numFrames + frame) * numLevels + level) * (size_t) (tableSize + 1);
    }

    /** The level to use for a phase increment (frequency / sample rate). */
    static int levelFor (float phaseInc) noexcept
    {
        int level = 0;
        float harmonics = (float) maxHarmonics;
        while (level < numLevels - 1 && harmonics * phaseInc > 0.5f)
        {
            harmonics *= 0.5f;
            ++level;
        }

        return level;
    }

private:
    using Spectrum = std::array<std::complex<float>, (size_t) maxHarmonics + 1>; // real = cosine part, imag = sine part

    WavetableBank()
    {
        data.assign ((size_t) (numTables * numFrames * numLevels) * (size_t) (tableSize + 1), 0.0f);

        juce::dsp::FFT fft (11);
        std::vector<float> buffer ((size_t) tableSize * 2);
        auto spectrum = std::make_unique<Spectrum>();

        for (int table = 0; table < numTables; ++table)
            for (int frame = 0; frame < numFrames; ++frame)
            {
                spectrum->fill ({});
                fillSpectrum (*spectrum, table, (float) frame / (float) (numFrames - 1));
                float scale = 1.0f;

                for (int level = 0; level < numLevels; ++level)
                {
                    const int harmonics = maxHarmonics >> level;
                    std::fill (buffer.begin(), buffer.end(), 0.0f);

                    // Bin h holds (cos - i sin) / 2, the mirrored bin its conjugate.
                    for (int h = 1; h <= juce::jmin (harmonics, maxHarmonics - 1); ++h)
                    {
                        const auto c = (*spectrum)[(size_t) h];
                        buffer[(size_t) h * 2] = c.real();
                        buffer[(size_t) h * 2 + 1] = -c.imag();
                        buffer[(size_t) (tableSize - h) * 2] = c.real();
                        buffer[(size_t) (tableSize - h) * 2 + 1] = c.imag();
                    }

                    fft.performRealOnlyInverseTransform (buffer.data());

                    if (level == 0)
                    {
                        float peak = 0.0f;
                        for (int i = 0; i < tableSize; ++i)
                            peak = juce::jmax (peak, std::abs (buffer[(size_t) i]));
                        scale = peak > 1.0e-9f ? 1.0f / peak : 0.0f;
                    }

                    auto* dest = const_cast<float*> (wave (table, frame, level));
                    for (int i = 0; i < tableSize; ++i)
                        dest[i] = buffer[(size_t) i] * scale;
                    dest[tableSize] = dest[0];
                }
            }
    }

    static float lerp (float a, float b, float t) noexcept { return a + (b - a) * t; }

    /** A repeatable pseudo-random phase for a harmonic. */
    static std::complex<float> scattered (int h, float amp) noexcept
    {
        const float angle = std::fmod ((float) h * 2.399963f + (float) (h * h) * 0.618034f, juce::MathConstants<float>::twoPi);
        return std::polar (amp, angle);
    }

    static void fillSpectrum (Spectrum& s, int table, float t)
    {
        const float pi = juce::MathConstants<float>::pi;

        switch (table)
        {
            case 0: // Basic Shapes: sine, triangle, saw, square
            {
                const float x = t * 3.0f;
                const int seg = juce::jmin (2, (int) x);
                const float f = x - (float) seg;

                for (int h = 1; h <= maxHarmonics; ++h)
                {
                    const bool odd = (h & 1) == 1;
                    const float fh = (float) h;
                    const float shapes[4] {
                        h == 1 ? 1.0f : 0.0f,
                        odd ? (((h / 2) & 1) == 0 ? 1.0f : -1.0f) / (fh * fh) : 0.0f,
                        0.6f / fh,
                        odd ? 0.8f / fh : 0.0f
                    };
                    s[(size_t) h] = { 0.0f, lerp (shapes[seg], shapes[seg + 1], f) };
                }
                break;
            }

            case 1: // Acid Saw: a saw with a resonant peak that sweeps up
            {
                const float centre = 2.0f * std::pow (24.0f, t);
                for (int h = 1; h <= maxHarmonics; ++h)
                {
                    const float fh = (float) h;
                    const float d = (fh - centre) / (0.18f * centre + 0.8f);
                    const float lowpass = 1.0f / std::sqrt (1.0f + std::pow (fh / (1.6f * centre), 4.0f));
                    s[(size_t) h] = { 0.0f, (1.0f / fh) * lowpass * (1.0f + 3.5f * std::exp (-d * d)) };
                }
                break;
            }

            case 2: // PWM: pulse width from 50% down to 4%
            {
                const float width = lerp (0.5f, 0.04f, t);
                for (int h = 1; h <= maxHarmonics; ++h)
                    s[(size_t) h] = { std::sin (pi * (float) h * width) / (float) h, 0.0f };
                break;
            }

            case 3: // Vowels: A E I O U formants over a buzzing source
            {
                static const float vowels[5][3] { { 730.0f, 1090.0f, 2440.0f }, { 530.0f, 1840.0f, 2480.0f }, { 270.0f, 2290.0f, 3010.0f },
                                                  { 570.0f, 840.0f, 2410.0f }, { 300.0f, 870.0f, 2240.0f } };
                const float x = t * 4.0f;
                const int seg = juce::jmin (3, (int) x);
                const float f = x - (float) seg;
                const float widths[3] { 70.0f, 95.0f, 130.0f }, gains[3] { 1.0f, 0.6f, 0.35f };

                for (int h = 1; h <= maxHarmonics; ++h)
                {
                    const float hz = (float) h * 130.0f;
                    float gain = 0.015f;
                    for (int k = 0; k < 3; ++k)
                    {
                        const float d = (hz - lerp (vowels[seg][k], vowels[seg + 1][k], f)) / widths[k];
                        gain += gains[k] / (1.0f + d * d);
                    }
                    s[(size_t) h] = { 0.0f, gain / std::sqrt ((float) h) / (1.0f + std::pow (hz / 6000.0f, 4.0f)) };
                }
                break;
            }

            case 4: // Kaval: flute spectrum, from soft to overblown, with breathy upper harmonics
            {
                s[1] = { 0.0f, 1.0f - 0.5f * t };
                s[2] = { 0.0f, 0.2f + 0.9f * t };
                s[3] = { 0.0f, 0.3f + 0.3f * t };
                for (int h = 4; h <= 64; ++h)
                    s[(size_t) h] = scattered (h, 0.22f * std::exp (-(float) (h - 3) * (0.5f - 0.32f * t)));
                break;
            }

            case 5: // Zurla: a piercing double reed, the formant moves up with the position
            {
                const float centre = lerp (4.0f, 13.0f, t);
                for (int h = 1; h <= maxHarmonics; ++h)
                {
                    const float fh = (float) h;
                    const float d = (fh - centre) / 3.0f;
                    const float amp = std::pow (fh, -0.6f) * (1.0f + 3.0f * std::exp (-d * d)) / (1.0f + (fh / 48.0f) * (fh / 48.0f));
                    s[(size_t) h] = { (h & 1) == 1 ? 0.0f : amp * 0.5f, amp };
                }
                break;
            }

            case 6: // Reeds: organ-like stops that come in one after another
            {
                static const int stops[8] { 1, 2, 3, 4, 6, 8, 12, 16 };
                for (int k = 0; k < 8; ++k)
                {
                    const float level = k == 0 ? 1.0f : juce::jlimit (0.0f, 1.0f, t * 7.0f - (float) (k - 1)) * 0.8f;
                    for (int m = 1; m <= 7; m += 2)
                        s[(size_t) (stops[k] * m)] += std::complex<float> (0.0f, level / (float) (m * m));
                }
                break;
            }

            default: // Bells: sparse partials, brighter with the position
            {
                static const int partials[10] { 1, 3, 4, 7, 11, 15, 19, 26, 34, 47 };
                for (int k = 0; k < 10; ++k)
                    s[(size_t) partials[k]] = scattered (partials[k], std::exp (-(float) k * (1.3f - 1.1f * t)));
                break;
            }
        }
    }

    std::vector<float> data;
};

/** A wavetable made from the user's own audio, in the same form as the tables of the bank:
    16 frames, each at 10 band-limited levels.

    The audio is read the way wavetable files are usually laid out: a file whose length is a
    multiple of 2048 samples is a row of 2048-sample frames, anything else is one single cycle.
    More or fewer than 16 frames are spread over the 16 positions by crossfading neighbours. */
class UserWavetable
{
public:
    static constexpr int numFrames = WavetableBank::numFrames;
    static constexpr int numLevels = WavetableBank::numLevels;
    static constexpr int tableSize = WavetableBank::tableSize;
    static constexpr int sourceSize = numFrames * tableSize;
    static constexpr int maxSourceFrames = 256;

    /** `frames` holds numFrames cycles of tableSize samples, one after another. */
    UserWavetable (std::vector<float> frames, juce::String tableName)
        : source (std::move (frames)), name (std::move (tableName))
    {
        source.resize ((size_t) sourceSize, 0.0f);
        data.assign ((size_t) (numFrames * numLevels) * (size_t) (tableSize + 1), 0.0f);

        juce::dsp::FFT fft (11);
        std::vector<float> spectrum ((size_t) tableSize * 2), buffer ((size_t) tableSize * 2);
        float peak = 0.0f;

        for (int frame = 0; frame < numFrames; ++frame)
        {
            std::fill (spectrum.begin(), spectrum.end(), 0.0f);
            std::copy (source.begin() + frame * tableSize, source.begin() + (frame + 1) * tableSize, spectrum.begin());
            fft.performRealOnlyForwardTransform (spectrum.data());

            for (int level = 0; level < numLevels; ++level)
            {
                // Keep harmonics 1 to `harmonics` and their mirror images; the constant part goes.
                const int harmonics = juce::jmin (WavetableBank::maxHarmonics >> level, WavetableBank::maxHarmonics - 1);
                std::fill (buffer.begin(), buffer.end(), 0.0f);
                for (int h = 1; h <= harmonics; ++h)
                    for (int bin : { h, tableSize - h })
                    {
                        buffer[(size_t) bin * 2] = spectrum[(size_t) bin * 2];
                        buffer[(size_t) bin * 2 + 1] = spectrum[(size_t) bin * 2 + 1];
                    }

                fft.performRealOnlyInverseTransform (buffer.data());

                auto* dest = data.data() + (size_t) (frame * numLevels + level) * (size_t) (tableSize + 1);
                std::copy (buffer.begin(), buffer.begin() + tableSize, dest);
                dest[tableSize] = dest[0];

                if (level == 0)
                    for (int i = 0; i < tableSize; ++i)
                        peak = juce::jmax (peak, std::abs (dest[i]));
            }
        }

        // One gain for the whole table, so quiet frames stay quiet next to loud ones.
        const float scale = peak > 1.0e-9f ? 1.0f / peak : 0.0f;
        for (auto& v : data)
            v *= scale;
    }

    /** Cuts audio into frames as described above. Returns null when there is nothing usable. */
    static std::unique_ptr<UserWavetable> fromAudio (const float* samples, int numSamples, const juce::String& tableName)
    {
        if (samples == nullptr || numSamples < 8)
            return {};

        for (int i = 0; i < numSamples; ++i)
            if (! std::isfinite (samples[i]))
                return {};

        const bool isRow = numSamples >= 2 * tableSize && numSamples % tableSize == 0;
        const int sourceFrames = isRow ? juce::jmin (maxSourceFrames, numSamples / tableSize) : 1;
        const int cycleLength = isRow ? tableSize : numSamples;

        // Reads one cycle of the source, stretched to tableSize samples.
        const auto read = [&] (int frame, int i)
        {
            const float* cycle = samples + (size_t) frame * (size_t) cycleLength;
            const double pos = (double) i * (double) cycleLength / (double) tableSize;
            const int i0 = (int) pos, i1 = (i0 + 1) % cycleLength;
            return cycle[i0] + (cycle[i1] - cycle[i0]) * (float) (pos - (double) i0);
        };

        std::vector<float> frames ((size_t) sourceSize);
        for (int frame = 0; frame < numFrames; ++frame)
        {
            const float framePos = (float) frame / (float) (numFrames - 1) * (float) (sourceFrames - 1);
            const int f0 = juce::jmin ((int) framePos, sourceFrames - 1), f1 = juce::jmin (f0 + 1, sourceFrames - 1);
            const float mix = framePos - (float) f0;

            for (int i = 0; i < tableSize; ++i)
                frames[(size_t) (frame * tableSize + i)] = read (f0, i) + (read (f1, i) - read (f0, i)) * mix;
        }

        return std::make_unique<UserWavetable> (std::move (frames), tableName);
    }

    /** One cycle of tableSize + 1 samples; the last sample repeats the first. */
    const float* wave (int frame, int level) const noexcept
    {
        return data.data() + (size_t) (frame * numLevels + level) * (size_t) (tableSize + 1);
    }

    /** The 16 cycles the table was built from, for saving with the project. */
    const std::vector<float>& getSource() const noexcept { return source; }
    const juce::String& getName() const noexcept { return name; }

private:
    std::vector<float> source, data;
    juce::String name;
};
} // namespace amv
