#pragma once

#include "Arpeggiator.h"

#include <array>
#include <cmath>

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
    numDests
};

inline constexpr int numSlots = 6;
inline constexpr int numLfos = 2;
inline constexpr int numMacros = 4;
} // namespace mod

inline const std::array<const char*, mod::numSources>& getModSources()
{
    static const std::array<const char*, mod::numSources> names { "None", "LFO 1", "LFO 2", "Filter Env", "Velocity", "Mod Wheel",
                                                                  "Macro 1", "Macro 2", "Macro 3", "Macro 4", "Pressure", "Timbre" };
    return names;
}

inline const std::array<const char*, mod::numDests>& getModDests()
{
    static const std::array<const char*, mod::numDests> names { "Pitch", "Cutoff", "Resonance", "Osc A Position", "Osc B Position",
                                                                "Osc A Level", "Osc B Level", "Engine Level", "Noise Level", "Volume" };
    return names;
}

inline const std::array<const char*, 5>& getLfoShapes()
{
    static const std::array<const char*, 5> names { "Sine", "Triangle", "Saw", "Square", "Random" };
    return names;
}

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
};

struct ModSlot
{
    int source = mod::srcNone;
    int dest = mod::dstPitch;
    float amount = 0.0f;  // -1..1
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

    /** Returns the value at the current position, then moves on by `numSamples`.
        `ppq` is the host position at this point; it is followed while the host plays. */
    float next (int numSamples, double sampleRate, const LfoParams& p, const TransportInfo& transport, double ppq) noexcept
    {
        const auto& division = getLfoDivisions()[(size_t) juce::jlimit (0, (int) getLfoDivisions().size() - 1, p.division)];
        const double hz = p.sync ? transport.bpm / 60.0 / division.beats : (double) p.rateHz;

        if (p.sync && transport.playing)
        {
            const double cycles = ppq / division.beats;
            const double whole = std::floor (cycles);
            phase = cycles - whole;
            cycle = (juce::int64) whole;
        }

        const float value = shapeAt (p.shape);

        phase += hz * (double) numSamples / sampleRate;
        while (phase >= 1.0)
        {
            phase -= 1.0;
            ++cycle;
        }

        return value;
    }

private:
    float shapeAt (int shape) const noexcept
    {
        const auto ph = (float) phase;

        switch (shape)
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
