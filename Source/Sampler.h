#pragma once

#include <juce_audio_basics/juce_audio_basics.h>

#include <array>
#include <cmath>
#include <memory>
#include <vector>

namespace amv
{
/** A loaded sample: two channels of the same length (a mono file fills both). */
class SampleData
{
public:
    static constexpr double maxSeconds = 60.0;

    /** Copies up to maxSeconds of `buffer`. Returns null when there is nothing to play. */
    static std::unique_ptr<SampleData> fromBuffer (const juce::AudioBuffer<float>& buffer, double sampleRate, const juce::String& name)
    {
        const int length = juce::jmin (buffer.getNumSamples(), (int) (maxSeconds * sampleRate));
        if (buffer.getNumChannels() < 1 || length < 16 || sampleRate < 1000.0)
            return nullptr;

        auto data = std::make_unique<SampleData>();
        data->sampleRate = sampleRate;
        data->name = name;
        data->stereo = buffer.getNumChannels() > 1;

        // A few silent samples after the end let the interpolation read past it.
        for (int ch = 0; ch < 2; ++ch)
        {
            const float* source = buffer.getReadPointer (juce::jmin (ch, buffer.getNumChannels() - 1));
            data->channels[(size_t) ch].assign ((size_t) length + guard, 0.0f);
            for (int i = 0; i < length; ++i)
                data->channels[(size_t) ch][(size_t) i] = std::isfinite (source[i]) ? juce::jlimit (-4.0f, 4.0f, source[i]) : 0.0f;
        }

        data->length = length;
        return data;
    }

    int getLength() const noexcept { return length; }
    double getSampleRate() const noexcept { return sampleRate; }
    bool isStereo() const noexcept { return stereo; }
    const juce::String& getName() const noexcept { return name; }
    const float* getChannel (int channel) const noexcept { return channels[(size_t) channel].data(); }

    /** The sample as a buffer, for saving and for the tools of the editor. */
    juce::AudioBuffer<float> toBuffer() const
    {
        juce::AudioBuffer<float> buffer (stereo ? 2 : 1, length);
        for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
            buffer.copyFrom (ch, 0, channels[(size_t) ch].data(), length);
        return buffer;
    }

    static constexpr int guard = 4;

private:
    std::array<std::vector<float>, 2> channels;
    int length = 0;
    double sampleRate = 44100.0;
    bool stereo = false;
    juce::String name;
};

/** Finds the pitch of a sample as a MIDI note, or returns -1 when it has no clear pitch. */
inline int detectRootNote (const SampleData& data)
{
    // Look at a short piece right after the loudest moment, where a note has settled.
    const int length = data.getLength();
    const auto rate = (float) data.getSampleRate();
    const int window = juce::jmin (length, juce::jmax (2048, (int) (rate * 0.12f)));
    const int maxLag = juce::jmin (window / 2, (int) (rate / 40.0f));
    const int minLag = juce::jmax (2, (int) (rate / 2000.0f));
    if (maxLag <= minLag + 2)
        return -1;

    const float* l = data.getChannel (0);
    const float* r = data.getChannel (1);
    int loudest = 0;
    float peak = 0.0f;
    for (int i = 0; i < length; ++i)
        if (std::abs (l[i]) > peak)
        {
            peak = std::abs (l[i]);
            loudest = i;
        }

    if (peak < 1.0e-4f)
        return -1;

    const int begin = juce::jlimit (0, length - window, loudest + (int) (rate * 0.03f));
    std::vector<float> x ((size_t) window);
    for (int i = 0; i < window; ++i)
        x[(size_t) i] = 0.5f * (l[begin + i] + r[begin + i]);

    // Normalised square difference: 1 where the sample repeats exactly.
    std::vector<float> score ((size_t) maxLag + 1, 0.0f);
    float best = 0.0f;
    for (int lag = minLag; lag <= maxLag; ++lag)
    {
        double product = 0.0, energy = 0.0;
        for (int i = 0; i + lag < window; ++i)
        {
            product += (double) x[(size_t) i] * x[(size_t) (i + lag)];
            energy += (double) x[(size_t) i] * x[(size_t) i] + (double) x[(size_t) (i + lag)] * x[(size_t) (i + lag)];
        }
        score[(size_t) lag] = energy > 1.0e-12 ? (float) (2.0 * product / energy) : 0.0f;
        best = juce::jmax (best, score[(size_t) lag]);
    }

    if (best < 0.5f)
        return -1;

    // The first strong peak is the period; later ones are its multiples.
    for (int lag = minLag + 1; lag < maxLag; ++lag)
        if (score[(size_t) lag] > 0.9f * best && score[(size_t) lag] >= score[(size_t) lag - 1] && score[(size_t) lag] >= score[(size_t) lag + 1])
        {
            const float a = score[(size_t) lag - 1], b = score[(size_t) lag], c = score[(size_t) lag + 1];
            const float shift = std::abs (a - 2.0f * b + c) > 1.0e-9f ? 0.5f * (a - c) / (a - 2.0f * b + c) : 0.0f;
            const float hz = rate / ((float) lag + shift);
            return juce::jlimit (0, 127, juce::roundToInt (69.0f + 12.0f * std::log2 (hz / 440.0f)));
        }

    return -1;
}

/** Cuts single cycles out of a sample for a wavetable: `numFrames` cycles of `frameSize`
    samples, taken at even steps between `start` and `end` (0..1). The length of a cycle
    comes from `rootNote`, so the root has to match the pitch of the sample. */
inline std::vector<float> sampleToWavetable (const SampleData& data, float start, float end, int rootNote, int numFrames, int frameSize)
{
    std::vector<float> frames ((size_t) (numFrames * frameSize), 0.0f);
    const auto length = (double) data.getLength();
    const double period = juce::jlimit (4.0, length - 2.0, data.getSampleRate() / (440.0 * std::exp2 (((double) rootNote - 69.0) / 12.0)));
    const double first = juce::jlimit (0.0, length - period - 1.0, (double) start * length);
    const double last = juce::jlimit (first, length - period - 1.0, (double) end * length - period);
    const float* l = data.getChannel (0);
    const float* r = data.getChannel (1);

    for (int f = 0; f < numFrames; ++f)
    {
        const double begin = first + (last - first) * (double) f / (double) juce::jmax (1, numFrames - 1);
        float* frame = frames.data() + (size_t) (f * frameSize);

        for (int i = 0; i <= frameSize; ++i)
        {
            const double position = begin + period * (double) i / (double) frameSize;
            const auto whole = (int) position;
            const auto t = (float) (position - (double) whole);
            const float value = 0.5f * (l[whole] + r[whole] + (l[whole + 1] + r[whole + 1] - l[whole] - r[whole]) * t);

            // The cycle rarely closes exactly; the difference is spread over its length.
            if (i == frameSize)
            {
                const float drift = value - frame[0];
                for (int k = 0; k < frameSize; ++k)
                    frame[k] -= drift * ((float) k / (float) frameSize - 0.5f);
            }
            else
            {
                frame[i] = value;
            }
        }
    }

    return frames;
}

namespace sampler
{
enum Mode
{
    classic = 0,
    granular,
    numModes
};

enum Loop
{
    loopOff = 0,
    loopForward,
    loopPingPong,
    numLoops
};
} // namespace sampler

inline const std::array<const char*, sampler::numModes>& getSamplerModes()
{
    static const std::array<const char*, sampler::numModes> names { "Classic", "Granular" };
    return names;
}

inline const std::array<const char*, sampler::numLoops>& getSamplerLoops()
{
    static const std::array<const char*, sampler::numLoops> names { "Loop Off", "Loop", "Ping Pong" };
    return names;
}

/** The sampler as the voices see it. Positions are 0..1 through the whole sample. */
struct SamplerParams
{
    const SampleData* data = nullptr; // not owned
    float level = 0.0f;
    int mode = sampler::classic;
    float pitch = 0.0f;        // semitones
    float fineCents = 0.0f;
    int rootNote = 60;         // the key that plays the sample at its own pitch
    bool keyTrack = true;      // off: every key plays the sample at its own pitch
    float pan = 0.0f;          // -1..1
    bool reverse = false;

    float start = 0.0f, end = 1.0f; // the part of the sample that is played
    int loop = sampler::loopOff;
    float loopStart = 0.0f, loopEnd = 1.0f;
    float crossfade = 0.0f;    // 0..1, share of the loop that fades into its own start

    // Granular
    float position = 0.0f;     // 0..1 between start and end
    float grainMs = 80.0f;
    float density = 20.0f;     // grains per second
    float spray = 0.0f;        // 0..1, random spread of the grain positions
    float pitchSpray = 0.0f;   // semitones, random detune of each grain
    float scan = 0.0f;         // -2..2, how fast the position travels on its own; 1 is the sample's own speed
    float shape = 0.5f;        // 0..1, where the grain is loudest: 0 percussive, 0.5 even, 1 swelling
    float spread = 0.5f;       // 0..1, random pan of each grain
    float reverseChance = 0.0f; // 0..1, share of the grains that play backwards
};

/** The sampler of one voice. */
class SamplerVoice
{
public:
    static constexpr int maxGrains = 32;

    void start (const SamplerParams& p, juce::uint32 seed) noexcept
    {
        rng = seed | 1u;
        for (auto& g : grains)
            g.on = false;

        spawn = 1.0f; // the first grain starts with the note
        scanPos = 0.0;
        finished = false;
        direction = p.reverse ? -1.0 : 1.0;

        if (p.data != nullptr)
        {
            const Region r (p);
            pos = p.reverse ? r.end - 1.0 : r.start;
        }
    }

    /** Positions of what is playing, 0..1 through the sample, for the display. Returns how many. */
    int getPositions (const SamplerParams& p, float* out, int capacity) const noexcept
    {
        if (p.data == nullptr || capacity < 1)
            return 0;

        const auto length = (float) p.data->getLength();
        if (p.mode == sampler::classic)
        {
            if (finished)
                return 0;
            out[0] = (float) pos / length;
            return 1;
        }

        int count = 0;
        for (const auto& g : grains)
            if (g.on && count < capacity)
                out[count++] = (float) g.pos / length;
        return count;
    }

    /** Adds one sample of output. `ratio` is the playback speed for the played key and
        `sampleRate` the rate of the output. `raw` receives the sample before level and pan. */
    void process (const SamplerParams& p, float ratio, float sampleRate, float& left, float& right, float* raw = nullptr) noexcept
    {
        const SampleData* data = p.data;
        if (data == nullptr)
            return;

        const float panL = juce::jmin (1.0f, 1.0f - p.pan), panR = juce::jmin (1.0f, 1.0f + p.pan);
        float l = 0.0f, r = 0.0f;
        const Region region (p);
        const double speed = (double) ratio * data->getSampleRate() / (double) sampleRate;

        if (p.mode == sampler::granular)
            processGrains (p, region, speed, sampleRate, l, r);
        else
            processClassic (p, region, speed, l, r);

        left += l * panL * p.level;
        right += r * panR * p.level;
        if (raw != nullptr)
            *raw = 0.5f * (l + r); // before level and pan, for use as a modulator
    }

private:
    /** The played part and the loop inside it, in samples. */
    struct Region
    {
        explicit Region (const SamplerParams& p) noexcept
        {
            const auto length = (double) p.data->getLength();
            start = juce::jlimit (0.0, length - 8.0, (double) p.start * length);
            end = juce::jlimit (start + 8.0, length, (double) p.end * length);
            loopStart = juce::jlimit (start, end - 8.0, (double) p.loopStart * length);
            loopEnd = juce::jlimit (loopStart + 8.0, end, (double) p.loopEnd * length);
        }

        double start, end, loopStart, loopEnd;
    };

    struct Grain
    {
        bool on = false;
        double pos = 0.0, inc = 1.0;
        float phase = 0.0f, phaseInc = 0.0f, peak = 0.5f, gainL = 1.0f, gainR = 1.0f;
    };

    float random() noexcept // 0..1
    {
        rng = rng * 1664525u + 1013904223u;
        return (float) (rng >> 8) * (1.0f / 16777216.0f);
    }

    static float hermite (const float* d, double position) noexcept
    {
        const auto whole = (int) position;
        const auto t = (float) (position - (double) whole);
        const float y0 = d[whole > 0 ? whole - 1 : 0], y1 = d[whole], y2 = d[whole + 1], y3 = d[whole + 2];
        const float c1 = 0.5f * (y2 - y0);
        const float c2 = y0 - 2.5f * y1 + 2.0f * y2 - 0.5f * y3;
        const float c3 = 0.5f * (y3 - y0) + 1.5f * (y1 - y2);
        return ((c3 * t + c2) * t + c1) * t + y1;
    }

    static float linear (const float* d, double position) noexcept
    {
        const auto whole = (int) position;
        const auto t = (float) (position - (double) whole);
        return d[whole] + (d[whole + 1] - d[whole]) * t;
    }

    void processClassic (const SamplerParams& p, const Region& region, double speed, float& l, float& r) noexcept
    {
        if (finished)
            return;

        const SampleData& data = *p.data;
        const float* dl = data.getChannel (0);
        const float* dr = data.getChannel (1);
        const bool forward = direction > 0.0;
        const bool looping = p.loop != sampler::loopOff;
        pos = juce::jlimit (0.0, (double) data.getLength() - 1.0, pos);

        l = hermite (dl, pos);
        r = hermite (dr, pos);

        // The end of a loop fades into the audio just before its start, so the jump back is smooth.
        if (p.loop == sampler::loopForward && p.crossfade > 0.0f)
        {
            const double loopLength = region.loopEnd - region.loopStart;
            const double fadeLength = juce::jmin ((double) p.crossfade * 0.5 * loopLength, p.reverse ? (double) data.getLength() - region.loopEnd : region.loopStart);

            if (fadeLength >= 1.0)
            {
                const double into = p.reverse ? region.loopStart + fadeLength - pos : pos - (region.loopEnd - fadeLength);
                if (into > 0.0 && into <= fadeLength)
                {
                    const auto mix = (float) (into / fadeLength);
                    const double other = juce::jlimit (0.0, (double) data.getLength() - 1.0, p.reverse ? pos + loopLength : pos - loopLength);
                    l += (hermite (dl, other) - l) * mix;
                    r += (hermite (dr, other) - r) * mix;
                }
            }
        }

        pos += speed * direction;

        if (looping)
        {
            const double loopLength = region.loopEnd - region.loopStart;

            if (p.loop == sampler::loopPingPong)
            {
                if (forward && pos >= region.loopEnd - 1.0)
                {
                    pos = juce::jmax (region.loopStart, 2.0 * (region.loopEnd - 1.0) - pos);
                    direction = -1.0;
                }
                else if (! forward && pos <= region.loopStart)
                {
                    pos = juce::jmin (region.loopEnd - 1.0, 2.0 * region.loopStart - pos);
                    direction = 1.0;
                }
            }
            else if (forward && pos >= region.loopEnd)
            {
                pos -= loopLength * std::ceil ((pos - region.loopEnd + 1.0e-9) / loopLength);
            }
            else if (! forward && pos < region.loopStart)
            {
                pos += loopLength * std::ceil ((region.loopStart - pos) / loopLength);
            }
        }
        else if ((forward && pos >= region.end) || (! forward && pos < region.start))
        {
            finished = true;
        }
    }

    void processGrains (const SamplerParams& p, const Region& region, double speed, float sampleRate, float& l, float& r) noexcept
    {
        const SampleData& data = *p.data;
        const float* dl = data.getChannel (0);
        const float* dr = data.getChannel (1);
        const double span = region.end - region.start;
        const double last = (double) data.getLength() - 1.0;

        const float size = juce::jlimit (2.0f, 1000.0f, p.grainMs) * 0.001f;
        const float density = juce::jlimit (0.5f, 200.0f, p.density);

        // The position travels on its own and wraps inside the played part.
        scanPos += (double) p.scan * data.getSampleRate() / ((double) sampleRate * span);
        scanPos -= std::floor (scanPos);

        spawn += density / sampleRate;
        if (spawn >= 1.0f)
        {
            // The next grain comes a little early or late, so a steady rate does not turn into a tone of its own.
            spawn -= 1.0f + (random() - 0.5f) * 0.3f * juce::jmin (1.0f, p.spray * 4.0f);

            for (auto& g : grains)
            {
                if (g.on)
                    continue;

                double where = (double) p.position + scanPos + (double) ((random() - 0.5f) * p.spray * 0.5f);
                where -= std::floor (where);
                const bool backwards = (random() < p.reverseChance) != p.reverse;
                const float detune = p.pitchSpray > 0.0f ? (random() * 2.0f - 1.0f) * p.pitchSpray : 0.0f;
                const float pan = (random() * 2.0f - 1.0f) * p.spread;

                g.on = true;
                g.inc = speed * (double) std::exp2 (detune / 12.0f) * (backwards ? -1.0 : 1.0);
                g.pos = region.start + where * span;
                g.phase = 0.0f;
                g.phaseInc = 1.0f / juce::jmax (8.0f, size * sampleRate);
                g.peak = juce::jlimit (0.03f, 0.97f, p.shape);
                g.gainL = juce::jmin (1.0f, 1.0f - pan);
                g.gainR = juce::jmin (1.0f, 1.0f + pan);

                // A backwards grain starts at its far end, so it covers the same audio.
                if (backwards)
                    g.pos += juce::jmin (span, (double) (size * sampleRate) * std::abs (g.inc));
                break;
            }
        }

        for (auto& g : grains)
        {
            if (! g.on)
                continue;

            if (g.pos >= 0.0 && g.pos < last)
            {
                const float t = g.phase < g.peak ? g.phase / g.peak : (1.0f - g.phase) / (1.0f - g.peak);
                const float window = t * t * (3.0f - 2.0f * t);
                l += linear (dl, g.pos) * window * g.gainL;
                r += linear (dr, g.pos) * window * g.gainR;
            }

            g.pos += g.inc;
            g.phase += g.phaseInc;
            if (g.phase >= 1.0f)
                g.on = false;
        }

        // Many overlapping grains add up; keep the level about the same at every density and size.
        const float overlap = density * size * 0.5f;
        const float gain = 1.0f / std::sqrt (juce::jmax (1.0f, overlap));
        l *= gain;
        r *= gain;
    }

    std::array<Grain, maxGrains> grains;
    double pos = 0.0, direction = 1.0, scanPos = 0.0;
    float spawn = 1.0f;
    bool finished = true;
    juce::uint32 rng = 1;
};
} // namespace amv
