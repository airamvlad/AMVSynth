#pragma once

#include "Arpeggiator.h"

#include <array>
#include <algorithm>
#include <cmath>
#include <vector>

namespace amv
{
namespace mod
{
enum Source
{
    srcNone = 0,
    srcLfo1,
    srcLfo2,
    srcFilterEnv,
    srcVelocity,
    srcModWheel,
    srcMacro1,
    srcMacro2,
    srcMacro3,
    srcMacro4,
    srcPressure,
    srcTimbre,
    srcLfo3, // added later; the order is saved in projects, so new sources go at the end
    srcLfo4,
    srcEnv3,
    numSources
};

enum Dest
{
    dstPitch = 0,
    dstCutoff,
    dstResonance,
    dstOscAPosition,
    dstOscBPosition,
    dstOscALevel,
    dstOscBLevel,
    dstEngineLevel,
    dstNoiseLevel,
    dstVolume,
    dstFilterEnv,
    dstSubLevel,
    dstVibrato,
    dstVibRate,
    dstEngine1, // the four controls of the selected Balkan model
    dstEngine2,
    dstEngine3,
    dstEngine4,
    dstOscADetune,
    dstOscBDetune,
    dstOscAPitch,
    dstOscBPitch,
    dstOscAWarp,
    dstOscBWarp,
    dstSamplerLevel,
    dstSamplerPitch,
    dstGrainPosition,
    dstGrainSize,
    dstGrainDensity,
    dstGrainSpray,
    dstGrainScan,
    dstAmpAttack,
    dstAmpDecay,
    dstAmpRelease,
    dstFltAttack,
    dstFltDecay,
    dstFltRelease,
    dstDistDrive, // from here on the effects, which are shared by all notes
    dstDistMix,
    dstGateDepth,
    dstDelayFeedback,
    dstDelayTone,
    dstDelayMix,
    dstReverbSize,
    dstReverbMix,
    numDests
};

inline constexpr int numSlots = 8;
inline constexpr int numLfos = 4;
inline constexpr int numMacros = 4;

/** How a slot reads its source: as the source is by nature, always upwards only (0..1), or
    always swinging both ways (-1..1). */
enum Polarity
{
    polarityAuto = 0,
    polarityUnipolar,
    polarityBipolar
};

/** The source of LFO 0 to 3. */
inline int lfoSource (int lfo) noexcept
{
    return lfo < 2 ? srcLfo1 + lfo : srcLfo3 + (lfo - 2);
}

/** LFOs and timbre swing both ways by nature; the other sources only go up from zero. */
inline bool isBipolarSource (int source) noexcept
{
    return source == srcLfo1 || source == srcLfo2 || source == srcLfo3 || source == srcLfo4 || source == srcTimbre;
}

/** The effects are not part of a voice: they follow the shared sources and, for the sources
    that belong to a note, the newest note. */
inline bool isGlobalDest (int dest) noexcept
{
    return dest >= dstDistDrive && dest <= dstReverbMix;
}

/** How far a full modulation moves an envelope time or a tone control, in octaves. */
inline constexpr float timeOctaves = 5.0f;

inline bool isBipolar (int source, int polarity) noexcept
{
    return polarity == polarityAuto ? isBipolarSource (source) : polarity == polarityBipolar;
}

/** The value of a source as a slot with this polarity sees it. */
inline float shaped (float value, int source, int polarity) noexcept
{
    if (polarity == polarityUnipolar && isBipolarSource (source))
        return 0.5f * (value + 1.0f);
    if (polarity == polarityBipolar && ! isBipolarSource (source))
        return 2.0f * value - 1.0f;
    return value;
}
} // namespace mod

inline const std::array<const char*, 3>& getModPolarities()
{
    static const std::array<const char*, 3> names { "Auto", "Unipolar", "Bipolar" };
    return names;
}

namespace warp
{
enum Mode
{
    off = 0,
    bendUp,
    bendDown,
    pwm,
    sync,
    mirror,
    numModes
};
} // namespace warp

inline const std::array<const char*, warp::numModes>& getWarpModes()
{
    static const std::array<const char*, warp::numModes> names { "Warp Off", "Bend +", "Bend -", "PWM", "Sync", "Mirror" };
    return names;
}

inline const std::array<const char*, mod::numSources>& getModSources()
{
    static const std::array<const char*, mod::numSources> names { "None", "LFO 1", "LFO 2", "Filter Env", "Velocity", "Mod Wheel",
                                                                  "Macro 1", "Macro 2", "Macro 3", "Macro 4", "Pressure", "Timbre",
                                                                  "LFO 3", "LFO 4", "Env 3" };
    return names;
}

inline const std::array<const char*, mod::numDests>& getModDests()
{
    static const std::array<const char*, mod::numDests> names { "Pitch", "Cutoff", "Resonance", "Osc A Position", "Osc B Position",
                                                                "Osc A Level", "Osc B Level", "Engine Level", "Noise Level", "Volume",
                                                                "Filter Env Amount", "Sub Level", "Vibrato Depth", "Vibrato Rate",
                                                                "Engine Knob 1", "Engine Knob 2", "Engine Knob 3", "Engine Knob 4",
                                                                "Osc A Detune", "Osc B Detune", "Osc A Pitch", "Osc B Pitch",
                                                                "Osc A Warp", "Osc B Warp", "Sampler Level", "Sampler Pitch", "Grain Position", "Grain Size",
                                                                "Grain Density", "Grain Spray", "Grain Scan",
                                                                "Amp Attack", "Amp Decay", "Amp Release", "Filter Attack", "Filter Decay", "Filter Release",
                                                                "Distortion Drive", "Distortion Mix", "Gate Depth", "Delay Feedback", "Delay Tone",
                                                                "Delay Mix", "Reverb Size", "Reverb Mix" };
    return names;
}

inline constexpr int lfoShapeCustom = 5;

inline const std::array<const char*, 6>& getLfoShapes()
{
    static const std::array<const char*, 6> names { "Sine", "Triangle", "Saw", "Square", "Random", "Custom" };
    return names;
}

namespace lfoMode
{
enum Mode
{
    free = 0,   // runs all the time and follows the host position when synced
    retrigger,  // starts again with every note
    oneShot,    // starts with every note, runs once and stays at its end, like an envelope
    numModes
};
} // namespace lfoMode

inline const std::array<const char*, lfoMode::numModes>& getLfoModes()
{
    static const std::array<const char*, lfoMode::numModes> names { "Free", "Retrigger", "One Shot" };
    return names;
}

/** A point of a drawn LFO shape. `curve` bends the line to the next point: 0 is straight. */
struct LfoPoint
{
    float x = 0.0f, y = 0.0f, curve = 0.0f; // 0..1, -1..1, -1..1
};

/** A drawn LFO shape: the points it was made from and one cycle as a table. */
class LfoShape
{
public:
    static constexpr int size = 512;
    static constexpr int maxPoints = 64;

    explicit LfoShape (std::vector<LfoPoint> newPoints) : points (std::move (newPoints))
    {
        // Keep the points in order, inside the cycle, with one at each end.
        for (auto& p : points)
        {
            p.x = std::isfinite (p.x) ? juce::jlimit (0.0f, 1.0f, p.x) : 0.0f;
            p.y = std::isfinite (p.y) ? juce::jlimit (-1.0f, 1.0f, p.y) : 0.0f;
            p.curve = std::isfinite (p.curve) ? juce::jlimit (-1.0f, 1.0f, p.curve) : 0.0f;
        }
        std::stable_sort (points.begin(), points.end(), [] (const LfoPoint& a, const LfoPoint& b) { return a.x < b.x; });
        if (points.size() > (size_t) maxPoints)
            points.resize ((size_t) maxPoints);
        if (points.empty())
            points = { { 0.0f, 1.0f, 0.0f }, { 1.0f, -1.0f, 0.0f } };
        if (points.front().x > 0.0f)
            points.insert (points.begin(), { 0.0f, points.front().y, 0.0f });
        if (points.back().x < 1.0f)
            points.push_back ({ 1.0f, points.back().y, 0.0f });

        for (int i = 0; i <= size; ++i)
            table[(size_t) i] = valueAt ((float) i / (float) size);
    }

    const std::vector<LfoPoint>& getPoints() const noexcept { return points; }

    /** The shape at a phase of 0..1, from the table. */
    float at (float phase) const noexcept
    {
        const float position = juce::jlimit (0.0f, 1.0f, phase) * (float) size;
        const int whole = juce::jmin (size - 1, (int) position);
        return table[(size_t) whole] + (table[(size_t) whole + 1] - table[(size_t) whole]) * (position - (float) whole);
    }

    /** The bend of a segment: `t` is 0..1 along it. */
    static float bend (float t, float curve) noexcept
    {
        if (std::abs (curve) < 1.0e-3f)
            return t;
        return curve > 0.0f ? std::pow (t, 1.0f + 4.0f * curve) : 1.0f - std::pow (1.0f - t, 1.0f - 4.0f * curve);
    }

    juce::String toString() const
    {
        juce::StringArray parts;
        for (const auto& p : points)
            parts.add (juce::String (p.x, 5) + "," + juce::String (p.y, 5) + "," + juce::String (p.curve, 4));
        return parts.joinIntoString (";");
    }

    static std::vector<LfoPoint> pointsFromString (const juce::String& text)
    {
        std::vector<LfoPoint> result;
        for (const auto& part : juce::StringArray::fromTokens (text, ";", ""))
        {
            const auto numbers = juce::StringArray::fromTokens (part, ",", "");
            if (numbers.size() == 3)
                result.push_back ({ numbers[0].getFloatValue(), numbers[1].getFloatValue(), numbers[2].getFloatValue() });
        }
        return result;
    }

private:
    float valueAt (float x) const noexcept
    {
        // The last segment that starts at or before x; two points at the same x make a step.
        size_t i = 0;
        while (i + 2 < points.size() && points[i + 1].x <= x)
            ++i;

        const auto& a = points[i];
        const auto& b = points[i + 1];
        const float width = b.x - a.x;
        if (width < 1.0e-6f)
            return b.y;
        return a.y + (b.y - a.y) * bend (juce::jlimit (0.0f, 1.0f, (x - a.x) / width), a.curve);
    }

    std::vector<LfoPoint> points;
    std::array<float, size + 1> table {};
};

struct LfoDivision
{
    const char* name;
    double beats; // length of one cycle in quarter notes
};

inline const std::array<LfoDivision, 11>& getLfoDivisions()
{
    static const std::array<LfoDivision, 11> divisions { { { "4 Bars", 16.0 }, { "2 Bars", 8.0 }, { "1 Bar", 4.0 }, { "1/2", 2.0 },
                                                          { "1/4.", 1.5 }, { "1/4", 1.0 }, { "1/8.", 0.75 }, { "1/8", 0.5 },
                                                          { "1/8T", 1.0 / 3.0 }, { "1/16", 0.25 }, { "1/16T", 1.0 / 6.0 } } };
    return divisions;
}

struct LfoParams
{
    int shape = 0;        // index into getLfoShapes()
    float rateHz = 2.0f;  // used when not synced
    bool sync = false;
    int division = 5;     // index into getLfoDivisions()
    int mode = lfoMode::free;
    const LfoShape* custom = nullptr; // the drawn shape, used when shape is Custom; not owned
};

struct ModSlot
{
    int source = mod::srcNone;
    int dest = mod::dstPitch;
    float amount = 0.0f;  // -1..1
    int polarity = mod::polarityAuto;
};

/** A free-running or tempo-synced LFO, shared by all voices. Output is -1..1. */
class Lfo
{
public:
    void reset() noexcept
    {
        phase = 0.0;
        cycle = 0;
    }

    /** A note started: the retriggered modes begin their cycle again. */
    void noteStarted (const LfoParams& p) noexcept
    {
        if (p.mode != lfoMode::free)
            reset();
    }

    /** Where the LFO is in its cycle, 0..1, for the display. */
    float getPhase() const noexcept { return (float) phase; }

    /** Returns the value at the current position, then moves on by `numSamples`.
        `ppq` is the host position at this point; it is followed while the host plays. */
    float next (int numSamples, double sampleRate, const LfoParams& p, const TransportInfo& transport, double ppq) noexcept
    {
        const auto& division = getLfoDivisions()[(size_t) juce::jlimit (0, (int) getLfoDivisions().size() - 1, p.division)];
        const double hz = p.sync ? transport.bpm / 60.0 / division.beats : (double) p.rateHz;

        if (p.sync && transport.playing && p.mode == lfoMode::free)
        {
            const double cycles = ppq / division.beats;
            const double whole = std::floor (cycles);
            phase = cycles - whole;
            cycle = (juce::int64) whole;
        }

        const float value = shapeAt (p);

        phase += hz * (double) numSamples / sampleRate;
        if (p.mode == lfoMode::oneShot)
        {
            // Stays at the end of its cycle until the next note.
            phase = juce::jmin (phase, 1.0);
        }
        else
        {
            while (phase >= 1.0)
            {
                phase -= 1.0;
                ++cycle;
            }
        }

        return value;
    }

private:
    float shapeAt (const LfoParams& p) const noexcept
    {
        const auto ph = (float) phase;

        if (p.shape == lfoShapeCustom)
            return p.custom != nullptr ? p.custom->at (ph) : 0.0f;

        switch (p.shape)
        {
            case 1:  return ph < 0.5f ? 4.0f * ph - 1.0f : 3.0f - 4.0f * ph;
            case 2:  return 1.0f - 2.0f * ph;
            case 3:  return ph < 0.5f ? 1.0f : -1.0f;
            case 4:  return randomFor (cycle);
            default: return std::sin (ph * juce::MathConstants<float>::twoPi);
        }
    }

    /** A repeatable random value per cycle, so a synced LFO gives the same pattern every time. */
    static float randomFor (juce::int64 n) noexcept
    {
        auto x = (juce::uint32) (n * 2654435761LL + 0x9e3779b9LL);
        x ^= x >> 15;
        x *= 0x2c1b3c6dU;
        x ^= x >> 12;
        x *= 0x297a2d39U;
        x ^= x >> 15;
        return (float) (x & 0xffffff) / (float) 0x7fffff - 1.0f;
    }

    double phase = 0.0;
    juce::int64 cycle = 0;
};
} // namespace amv
