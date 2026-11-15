#pragma once

#include <array>
#include <cmath>
#include <vector>

namespace amv
{
/** A scale as cents above the root. Quarter tones are written directly, e.g. 350. */
struct ScaleDef
{
    const char* name;
    std::vector<float> cents;
};

inline const std::vector<ScaleDef>& getScales()
{
    static const std::vector<ScaleDef> scales {
        { "Chromatic", { 0, 100, 200, 300, 400, 500, 600, 700, 800, 900, 1000, 1100 } },
        { "Hijaz", { 0, 100, 400, 500, 700, 800, 1000 } },
        { "Hijazkar", { 0, 100, 400, 500, 700, 800, 1100 } },
        { "Nikriz", { 0, 200, 300, 600, 700, 900, 1000 } },
        { "Harmonic Minor", { 0, 200, 300, 500, 700, 800, 1100 } },
        { "Kurdi", { 0, 100, 300, 500, 700, 800, 1000 } },
        { "Rast", { 0, 200, 350, 500, 700, 900, 1050 } },
        { "Ussak", { 0, 150, 300, 500, 700, 800, 1000 } },
        { "Huseyni", { 0, 150, 300, 500, 700, 850, 1000 } },
        { "Saba", { 0, 150, 300, 400, 700, 800, 1000 } },
        { "Minor", { 0, 200, 300, 500, 700, 800, 1000 } },
        { "Major", { 0, 200, 400, 500, 700, 900, 1100 } },
        // All twelve keys, built from pure fifths (3:2): three fifths down and eight up from the root.
        { "Pythagorean", { 0.0f, 113.685f, 203.910f, 294.135f, 407.820f, 498.045f, 611.730f, 701.955f, 815.640f, 905.865f, 996.090f,
                           1109.775f } },
    };
    return scales;
}

inline const std::array<const char*, 12>& getNoteNames()
{
    static const std::array<const char*, 12> names { "C", "C#", "D", "Eb", "E", "F", "F#", "G", "Ab", "A", "Bb", "B" };
    return names;
}

/** Maps MIDI keys to pitches for the chosen root and scale.

    Each scale degree lives on the nearest key. A quarter tone sits on the natural key above it,
    lowered by 50 cents, which is how these scales are usually played on a keyboard.
    Keys outside the scale either stay in equal temperament or, with snap on, play the nearest
    scale degree. */
class Tuning
{
public:
    Tuning() { rebuild(); }

    void set (int newRoot, int newScale, bool newSnap)
    {
        newScale = newScale < 0 || newScale >= (int) getScales().size() ? 0 : newScale;
        newRoot = ((newRoot % 12) + 12) % 12;

        if (newRoot == root && newScale == scale && newSnap == snap)
            return;

        root = newRoot;
        scale = newScale;
        snap = newSnap;
        rebuild();
    }

    /** The pitch a key plays, as a fractional MIDI note number. */
    float pitchFor (int midiNote) const noexcept
    {
        const int rel = (((midiNote - root) % 12) + 12) % 12;
        return (float) (midiNote - rel) + keyCents[(size_t) rel] * 0.01f;
    }

    /** Distance in semitones from this key's pitch up to the next scale degree. */
    float stepAbove (int midiNote) const noexcept
    {
        const int rel = (((midiNote - root) % 12) + 12) % 12;
        const float current = keyCents[(size_t) rel];
        const auto& cents = getScales()[(size_t) scale].cents;

        for (float c : cents)
            if (c > current + 1.0f)
                return (c - current) * 0.01f;

        return (cents.front() + 1200.0f - current) * 0.01f;
    }

private:
    void rebuild()
    {
        const auto& cents = getScales()[(size_t) scale].cents;
        std::array<bool, 12> isDegree {};

        for (size_t key = 0; key < 12; ++key)
            keyCents[key] = (float) key * 100.0f;

        for (float c : cents)
        {
            const auto key = (size_t) ((int) std::floor (c / 100.0f + 0.5f) % 12);
            keyCents[key] = c;
            isDegree[key] = true;
        }

        if (! snap)
            return;

        for (size_t key = 0; key < 12; ++key)
        {
            if (isDegree[key])
                continue;

            const float keyPos = (float) key * 100.0f;
            float best = cents.front() + 1200.0f; // the octave, so the top keys can snap upwards
            float bestDistance = std::abs (best - keyPos);

            // Walk downwards so a tie goes to the lower degree.
            for (auto it = cents.rbegin(); it != cents.rend(); ++it)
            {
                const float distance = std::abs (*it - keyPos);
                if (distance <= bestDistance)
                {
                    best = *it;
                    bestDistance = distance;
                }
            }

            keyCents[key] = best;
        }
    }

    int root = 0, scale = 0;
    bool snap = false;
    std::array<float, 12> keyCents {};
};
} // namespace amv
