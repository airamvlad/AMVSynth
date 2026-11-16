#include "StepEditor.h"
#include "PluginEditor.h"

namespace
{
constexpr float gutter = 76.0f;      // lane names and the value under the mouse
constexpr float numbersHeight = 16.0f;
constexpr float laneGap = 8.0f;
constexpr int noteRows = 2 * amv::SeqPattern::noteRange + 1;
} // namespace

StepEditor::StepEditor (AMVSynthAudioProcessor& p) : processor (p)
{
    lengthParam = processor.apvts.getRawParameterValue (amv::id::seqSteps);
    meterParam = processor.apvts.getRawParameterValue (amv::id::seqMeter);
    onParam = processor.apvts.getRawParameterValue (amv::id::seqOn);
    selectParam = processor.apvts.getRawParameterValue (amv::id::seqSelect);
    refresh();
}

void StepEditor::refresh()
{
    bool changed = false;

    // Another pattern can be chosen, and a pattern can be replaced from outside, by a loaded project or preset.
    const int newSelect = juce::jlimit (0, amv::SeqBank::numPatterns - 1, juce::roundToInt (selectParam->load()));
    if (dragLane == noLane && (processor.getSeqVersion() != shownVersion || newSelect != select))
    {
        select = newSelect;
        shownVersion = processor.getSeqVersion();
        pattern = processor.getSeqPattern (select);
        changed = true;
    }

    const int newLength = juce::jlimit (1, (int) amv::SeqPattern::maxSteps, juce::roundToInt (lengthParam->load()));
    const int newMeter = juce::roundToInt (meterParam->load());
    const bool newOn = onParam->load() > 0.5f;
    const int newPlaying = newOn && processor.seqPatternPlaying.load (std::memory_order_relaxed) == select ? processor.seqStep.load (std::memory_order_relaxed) : -1;

    changed = changed || newLength != length || newMeter != meter || newOn != on || newPlaying != playing;
    length = newLength;
    meter = newMeter;
    on = newOn;
    playing = newPlaying;

    if (changed && isVisible())
        repaint();
}

juce::Rectangle<float> StepEditor::grid() const
{
    return getLocalBounds().toFloat().withTrimmedLeft (gutter).withTrimmedTop (numbersHeight).reduced (0.0f, 2.0f);
}

juce::Rectangle<float> StepEditor::laneArea (int lane) const
{
    // The note roll takes about half of the height, the slide switches one thin row.
    static constexpr float shares[numLanes] { 0.54f, 0.19f, 0.19f, 0.08f };
    auto area = grid();
    const float usable = area.getHeight() - laneGap * (float) (numLanes - 1);
    float y = area.getY();

    for (int i = 0; i < lane; ++i)
        y += usable * shares[i] + laneGap;

    return { area.getX(), y, area.getWidth(), usable * shares[lane] };
}

float StepEditor::stepWidth() const
{
    return grid().getWidth() / (float) amv::SeqPattern::maxSteps;
}

int StepEditor::stepAt (float x) const
{
    const auto area = grid();
    if (x < area.getX() || x >= area.getRight())
        return -1;

    return juce::jlimit (0, (int) amv::SeqPattern::maxSteps - 1, (int) ((x - area.getX()) / stepWidth()));
}

int StepEditor::laneAt (float y) const
{
    for (int lane = 0; lane < numLanes; ++lane)
    {
        const auto area = laneArea (lane);
        if (y >= area.getY() - laneGap * 0.5f && y < area.getBottom() + laneGap * 0.5f)
            return lane;
    }

    return noLane;
}

bool StepEditor::isAccent (int step) const
{
    return amv::Arpeggiator::isAccent (meter, step % amv::Arpeggiator::patternLength (meter));
}

juce::String StepEditor::valueText (int lane, int step) const
{
    if (step < 0)
        return {};

    const auto& s = pattern.steps[(size_t) step];
    switch (lane)
    {
        case noteLane:     return s.gate <= 0.0f ? juce::String ("rest") : (s.note > 0 ? "+" : "") + juce::String (s.note) + " st";
        case velocityLane: return juce::String (juce::roundToInt (s.velocity * 100.0f)) + " %";
        case gateLane:     return s.gate <= 0.0f ? juce::String ("rest") : juce::String (juce::roundToInt (s.gate * 100.0f)) + " %";
        case slideLane:    return s.slide ? "slide" : "off";
        default:           return {};
    }
}

juce::String StepEditor::getTooltip()
{
    switch (hoverLane)
    {
        case noteLane:     return "The note of each step, in semitones from the key you hold. Click or drag to set it; right-click makes a step a rest.";
        case velocityLane: return "How hard each step is played. Drag across the steps to draw.";
        case gateLane:     return "How long each step sounds. Dragged all the way down the step is a rest.";
        case slideLane:    return "A slide holds the note into the next step: in Mono Legato the pitch glides, and the same note again is tied into one long note.";
        default:           return {};
    }
}

void StepEditor::paint (juce::Graphics& g)
{
    using namespace amv;
    const auto area = grid();
    const float width = stepWidth();
    const char* names[numLanes] { "NOTE", "VELOCITY", "GATE", "SLIDE" };

    // Lane backgrounds and names.
    for (int lane = 0; lane < numLanes; ++lane)
    {
        const auto r = laneArea (lane);
        g.setColour (colour::background);
        g.fillRoundedRectangle (r, 5.0f);

        auto label = juce::Rectangle<float> (0.0f, r.getY(), gutter - 10.0f, r.getHeight());
        g.setColour (lane == hoverLane ? colour::text : colour::dimText);
        g.setFont (juce::FontOptions (11.0f, juce::Font::bold));
        g.drawText (names[lane], lane == slideLane ? label : label.removeFromTop (18.0f), juce::Justification::centredLeft);

        if (lane == hoverLane && lane != slideLane && hoverStep >= 0)
        {
            g.setColour (colour::teal);
            g.setFont (juce::FontOptions (12.0f));
            g.drawText (juce::String (hoverStep + 1) + ":  " + valueText (lane, hoverStep), label.removeFromTop (18.0f), juce::Justification::centredLeft);
        }
    }

    // The note roll: a line for every semitone, stronger at the key and its octaves.
    const auto roll = laneArea (noteLane);
    const float rowHeight = roll.getHeight() / (float) noteRows;
    for (int row = 0; row < noteRows; ++row)
    {
        const int note = SeqPattern::noteRange - row;
        const float y = roll.getY() + rowHeight * (float) row;

        if (note % 12 == 0)
        {
            g.setColour ((note == 0 ? colour::red : colour::teal).withAlpha (note == 0 ? 0.16f : 0.08f));
            g.fillRect (roll.getX(), y, roll.getWidth(), rowHeight);
            g.setColour (colour::dimText);
            g.setFont (juce::FontOptions (10.0f));
            g.drawText (note == 0 ? "KEY" : (note > 0 ? "+12" : "-12"), juce::Rectangle<float> (0.0f, y - 6.0f, gutter - 8.0f, rowHeight + 12.0f),
                        juce::Justification::centredRight);
        }
        else if (row > 0)
        {
            g.setColour (colour::track.withAlpha (0.35f));
            g.fillRect (roll.getX(), y, roll.getWidth(), 1.0f);
        }
    }

    for (int step = 0; step < SeqPattern::maxSteps; ++step)
    {
        const auto& s = pattern.steps[(size_t) step];
        const float x = area.getX() + width * (float) step;
        const bool used = step < length, accent = isAccent (step), rest = s.gate <= 0.0f;
        const bool lit = step == playing && used;
        const float alpha = used ? 1.0f : 0.25f;

        // Step numbers, the first step of every group of the meter stands out.
        g.setFont (juce::FontOptions (10.0f, accent ? juce::Font::bold : juce::Font::plain));
        g.setColour ((lit ? colour::text : accent ? colour::red : colour::dimText).withAlpha (used ? 1.0f : 0.35f));
        g.drawText (juce::String (step + 1), juce::Rectangle<float> (x, 0.0f, width, numbersHeight), juce::Justification::centred);

        for (int lane = 0; lane < numLanes; ++lane)
        {
            const auto r = laneArea (lane);
            const auto cell = juce::Rectangle<float> (x, r.getY(), width, r.getHeight());

            if (lit)
            {
                g.setColour (colour::text.withAlpha (0.10f));
                g.fillRect (cell);
            }
            else if (accent && used)
            {
                g.setColour (colour::red.withAlpha (0.06f));
                g.fillRect (cell);
            }

            // A line before every group, a faint one between the steps.
            if (step > 0)
            {
                g.setColour (accent && used ? colour::balkanBorder : colour::track.withAlpha (0.45f));
                g.fillRect (x, r.getY(), accent && used ? 1.5f : 1.0f, r.getHeight());
            }

            const auto colour = (accent ? colour::red : colour::teal).withAlpha (alpha);
            const auto inner = cell.reduced (2.5f, 0.0f);

            if (lane == noteLane)
            {
                const float y = roll.getY() + rowHeight * (float) (SeqPattern::noteRange - s.note);
                const auto block = juce::Rectangle<float> (inner.getX(), y + 0.5f, inner.getWidth(), juce::jmax (4.0f, rowHeight - 1.0f));

                if (rest)
                {
                    g.setColour (colour::dimText.withAlpha (0.5f * alpha));
                    g.drawRoundedRectangle (block.reduced (0.5f), 2.0f, 1.0f);
                }
                else
                {
                    // The block is as long as the gate; a slide reaches the next step.
                    const float part = s.slide ? 1.0f : juce::jlimit (0.2f, 1.0f, s.gate);
                    g.setColour (colour.withMultipliedAlpha (0.45f + 0.55f * s.velocity));
                    g.fillRoundedRectangle (block.withWidth (s.slide ? width : inner.getWidth() * part), 2.0f);
                    if (lit)
                    {
                        g.setColour (colour::text);
                        g.drawRoundedRectangle (block.withWidth (s.slide ? width : inner.getWidth() * part), 2.0f, 1.2f);
                    }
                }
            }
            else if (lane == velocityLane || lane == gateLane)
            {
                const float value = lane == velocityLane ? s.velocity : s.gate;
                const float height = juce::jmax (1.5f, (cell.getHeight() - 4.0f) * value);
                g.setColour (colour.withMultipliedAlpha (rest ? 0.25f : (lane == hoverLane && step == hoverStep ? 1.0f : 0.8f)));
                g.fillRoundedRectangle (inner.getX(), cell.getBottom() - 2.0f - height, inner.getWidth(), height, 2.0f);
            }
            else
            {
                const auto box = inner.reduced (0.0f, 3.0f);
                if (s.slide)
                {
                    g.setColour (colour.withMultipliedAlpha (rest ? 0.3f : 1.0f));
                    g.fillRoundedRectangle (box, 2.5f);
                }
                else
                {
                    g.setColour (colour::track.withAlpha (alpha));
                    g.drawRoundedRectangle (box.reduced (0.5f), 2.5f, 1.0f);
                }
            }
        }
    }

    if (! on)
    {
        g.setColour (colour::dimText);
        g.setFont (juce::FontOptions (11.0f));
        g.drawText ("OFF", juce::Rectangle<float> (0.0f, 0.0f, gutter - 10.0f, numbersHeight), juce::Justification::centredLeft);
    }
}

void StepEditor::edit (int lane, int step, float y)
{
    if (step < 0 || lane == noLane)
        return;

    auto& s = pattern.steps[(size_t) step];
    const auto area = laneArea (lane);
    const float value = juce::jlimit (0.0f, 1.0f, 1.0f - (y - area.getY() - 2.0f) / (area.getHeight() - 4.0f));

    switch (lane)
    {
        case noteLane:
        {
            const int row = juce::jlimit (0, noteRows - 1, (int) ((y - area.getY()) / (area.getHeight() / (float) noteRows)));
            s.note = amv::SeqPattern::noteRange - row;
            if (s.gate <= 0.0f)
                s.gate = 0.5f; // a note drawn on a rest plays again
            break;
        }
        case velocityLane: s.velocity = juce::jmax (0.02f, value); break;
        case gateLane:     s.gate = value < 0.04f ? 0.0f : value; break;
        case slideLane:    s.slide = slideValue; break;
        default: break;
    }

    commit (false);
}

void StepEditor::commit (bool save)
{
    processor.setSeqPattern (select, pattern, save);
    shownVersion = processor.getSeqVersion();
    repaint();
}

void StepEditor::mouseMove (const juce::MouseEvent& e)
{
    const int lane = laneAt (e.position.y), step = stepAt (e.position.x);
    if (lane != hoverLane || step != hoverStep)
    {
        hoverLane = lane;
        hoverStep = step;
        repaint();
    }
}

void StepEditor::mouseExit (const juce::MouseEvent&)
{
    hoverLane = noLane;
    hoverStep = -1;
    repaint();
}

void StepEditor::mouseDown (const juce::MouseEvent& e)
{
    const int lane = laneAt (e.position.y), step = stepAt (e.position.x);
    if (lane == noLane || step < 0)
        return;

    hoverLane = lane;
    hoverStep = step;
    auto& s = pattern.steps[(size_t) step];

    if (e.mods.isPopupMenu())
    {
        // A right click switches between a rest and a note; dragging on does the same to more steps.
        restoring = s.gate <= 0.0f;
        s.gate = restoring ? 0.5f : 0.0f;
        dragLane = numLanes;
        commit (false);
        return;
    }

    dragLane = lane;
    slideValue = ! s.slide;
    edit (lane, step, e.position.y);
}

void StepEditor::mouseDrag (const juce::MouseEvent& e)
{
    const int step = stepAt (e.position.x);
    if (dragLane == noLane || step < 0)
        return;

    hoverStep = step;

    if (dragLane == numLanes)
    {
        auto& s = pattern.steps[(size_t) step];
        if (restoring != (s.gate > 0.0f))
        {
            s.gate = restoring ? 0.5f : 0.0f;
            commit (false);
        }

        return;
    }

    edit (dragLane, step, e.position.y);
}

void StepEditor::mouseUp (const juce::MouseEvent&)
{
    if (dragLane != noLane)
    {
        dragLane = noLane;
        commit (true);
    }
}

void StepEditor::fitToMeter (int bars)
{
    const int steps = juce::jlimit (1, (int) amv::SeqPattern::maxSteps, amv::Arpeggiator::patternLength (meter) * bars);
    if (auto* param = processor.apvts.getParameter (amv::id::seqSteps))
        param->setValueNotifyingHost (param->convertTo0to1 ((float) steps));
}

void StepEditor::accentsFromMeter()
{
    for (int i = 0; i < amv::SeqPattern::maxSteps; ++i)
        pattern.steps[(size_t) i].velocity = isAccent (i) ? 1.0f : 0.55f;

    commit (true);
}

void StepEditor::randomise()
{
    // Notes come from the scale that is chosen on the SYNTH page, taking the held key as its root.
    const int scale = juce::jlimit (0, (int) amv::getScales().size() - 1, juce::roundToInt (processor.apvts.getRawParameterValue (amv::id::scale)->load()));
    std::array<bool, 12> inScale {};
    for (float cents : amv::getScales()[(size_t) scale].cents)
        inScale[(size_t) (juce::roundToInt (cents / 100.0f) % 12)] = true;

    std::vector<int> notes;
    for (int n = -amv::SeqPattern::noteRange; n <= amv::SeqPattern::noteRange; ++n)
        if (inScale[(size_t) (((n % 12) + 12) % 12)])
            notes.push_back (n);

    juce::Random random;
    for (int i = 0; i < amv::SeqPattern::maxSteps; ++i)
    {
        auto& s = pattern.steps[(size_t) i];
        const bool accent = isAccent (i);

        // The first step of a group tends to stay on the key, the others wander.
        s.note = accent && random.nextFloat() < 0.6f ? 0 : notes[(size_t) random.nextInt ((int) notes.size())];
        s.velocity = accent ? 1.0f : 0.45f + 0.35f * random.nextFloat();
        s.gate = ! accent && random.nextFloat() < 0.15f ? 0.0f : 0.3f + 0.5f * random.nextFloat();
        s.slide = random.nextFloat() < 0.15f;
    }

    commit (true);
}

void StepEditor::shift (int by)
{
    // Only the steps that play move round.
    const auto old = pattern;
    for (int i = 0; i < length; ++i)
        pattern.steps[(size_t) (((i + by) % length + length) % length)] = old.steps[(size_t) i];

    commit (true);
}

void StepEditor::reverse()
{
    std::reverse (pattern.steps.begin(), pattern.steps.begin() + length);
    commit (true);
}

void StepEditor::clear()
{
    pattern = {};
    commit (true);
}

void StepEditor::copy()
{
    clipboard = pattern;
    hasClipboard = true;
}

void StepEditor::paste()
{
    if (! hasClipboard)
        return;

    pattern = clipboard;
    commit (true);
}

//==============================================================================
PatternBar::PatternBar (AMVSynthAudioProcessor& p) : processor (p)
{
    refresh();
}

void PatternBar::refresh()
{
    const auto value = [this] (const char* paramId) { return processor.apvts.getRawParameterValue (paramId)->load(); };
    const int newSelect = juce::roundToInt (value (amv::id::seqSelect));
    const bool newKeySwitch = value (amv::id::seqKeySwitch) > 0.5f, newChain = value (amv::id::seqChainOn) > 0.5f;
    const int newPlaying = value (amv::id::seqOn) > 0.5f && processor.seqStep.load (std::memory_order_relaxed) >= 0
                               ? processor.seqPatternPlaying.load (std::memory_order_relaxed) : -1;

    bool changed = newSelect != select || newKeySwitch != keySwitchOn || newChain != chainIsOn || newPlaying != playing;
    select = newSelect;
    keySwitchOn = newKeySwitch;
    chainIsOn = newChain;
    playing = newPlaying;

    if (processor.getSeqVersion() != version)
    {
        version = processor.getSeqVersion();
        chainShown = processor.getSeqChain();
        changed = true;
    }

    if (changed && isVisible())
        repaint();
}

juce::Rectangle<float> PatternBar::row (int index) const
{
    const auto area = getLocalBounds().toFloat();
    const float height = juce::jmin (32.0f, area.getHeight() / 3.0f);
    const float top = area.getCentreY() - height * 1.5f;
    return { area.getX(), top + height * (float) index, area.getWidth(), height };
}

juce::Rectangle<float> PatternBar::cell (int rowIndex, int index, int count) const
{
    const auto r = row (rowIndex);
    const float width = r.getWidth() / (float) count;
    return juce::Rectangle<float> (r.getX() + width * (float) index, r.getY(), width, r.getHeight()).reduced (2.5f, 3.5f);
}

int PatternBar::partAt (juce::Point<float> p, int& index) const
{
    for (int i = 0; i < amv::SeqBank::numPatterns; ++i)
        if (cell (0, i, amv::SeqBank::numPatterns).contains (p))
        {
            index = i;
            return letters;
        }

    for (int i = 0; i < 2; ++i)
        if (cell (1, i, 2).contains (p))
        {
            index = 0;
            return i == 0 ? keySwitch : chainOn;
        }

    for (int i = 0; i < amv::SeqBank::maxChain; ++i)
        if (cell (2, i, amv::SeqBank::maxChain).contains (p))
        {
            index = i;
            return chain;
        }

    index = -1;
    return none;
}

void PatternBar::setParam (const char* paramId, float value)
{
    if (auto* param = processor.apvts.getParameter (paramId))
        param->setValueNotifyingHost (param->convertTo0to1 (value));
}

juce::String PatternBar::getTooltip()
{
    switch (hoverPart)
    {
        case letters:   return "The pattern that is shown and played. The dot marks the one that is playing now.";
        case keySwitch: return "The MIDI keys C0, C#0, D0 and D#0 choose pattern A, B, C or D while you play. They make no sound.";
        case chainOn:   return "Play the patterns in the boxes below one after the other. Each one plays once through, or for one bar when FREE CYCLE is off.";
        case chain:     return "The chain. Click a box to step it through A to D, click the first empty box to add one, right-click to remove one.";
        default:        return {};
    }
}

void PatternBar::paint (juce::Graphics& g)
{
    using namespace amv;
    g.setFont (juce::FontOptions (12.5f, juce::Font::bold));

    for (int i = 0; i < SeqBank::numPatterns; ++i)
    {
        const auto r = cell (0, i, SeqBank::numPatterns);
        const bool chosen = i == select, hover = hoverPart == letters && hoverIndex == i;
        g.setColour (chosen ? colour::red : colour::knob.brighter (hover ? 0.15f : 0.0f));
        g.fillRoundedRectangle (r, 5.0f);
        g.setColour (chosen ? colour::background : colour::text);
        g.drawText (juce::String::charToString ((juce::juce_wchar) ('A' + i)), r, juce::Justification::centred);

        if (i == playing)
        {
            g.setColour (chosen ? colour::background : colour::teal);
            g.fillEllipse (r.getRight() - 11.0f, r.getY() + 5.0f, 5.0f, 5.0f);
        }
    }

    const char* names[2] { "KEY SWITCH", "CHAIN" };
    for (int i = 0; i < 2; ++i)
    {
        const auto r = cell (1, i, 2);
        const bool isOn = i == 0 ? keySwitchOn : chainIsOn, hover = hoverPart == (i == 0 ? keySwitch : chainOn);
        g.setColour (isOn ? colour::red : colour::background);
        g.fillRoundedRectangle (r, r.getHeight() * 0.5f);
        g.setColour (isOn ? colour::red : colour::balkanBorder.brighter (hover ? 0.4f : 0.0f));
        g.drawRoundedRectangle (r.reduced (0.5f), r.getHeight() * 0.5f, 1.0f);
        g.setColour (isOn ? colour::background : colour::text);
        g.setFont (juce::FontOptions (11.0f, juce::Font::bold));
        g.drawText (names[i], r, juce::Justification::centred);
    }

    for (int i = 0; i < SeqBank::maxChain; ++i)
    {
        const auto r = cell (2, i, SeqBank::maxChain);
        const bool filled = i < (int) chainShown.size(), hover = hoverPart == chain && hoverIndex == i;
        const float alpha = chainIsOn ? 1.0f : 0.45f;

        if (filled)
        {
            g.setColour ((chainShown[(size_t) i] == playing && chainIsOn ? colour::teal : colour::track.brighter (hover ? 0.2f : 0.0f)).withAlpha (alpha));
            g.fillRoundedRectangle (r, 4.0f);
            g.setColour ((chainShown[(size_t) i] == playing && chainIsOn ? colour::background : colour::text).withAlpha (alpha));
            g.setFont (juce::FontOptions (12.0f, juce::Font::bold));
            g.drawText (juce::String::charToString ((juce::juce_wchar) ('A' + chainShown[(size_t) i])), r, juce::Justification::centred);
        }
        else
        {
            g.setColour (colour::track.withAlpha ((hover && i == (int) chainShown.size() ? 1.0f : 0.5f) * alpha));
            g.drawRoundedRectangle (r.reduced (0.5f), 4.0f, 1.0f);
        }
    }
}

void PatternBar::mouseDown (const juce::MouseEvent& e)
{
    int index = -1;
    switch (partAt (e.position, index))
    {
        case letters:   setParam (amv::id::seqSelect, (float) index); break;
        case keySwitch: setParam (amv::id::seqKeySwitch, keySwitchOn ? 0.0f : 1.0f); break;
        case chainOn:   setParam (amv::id::seqChainOn, chainIsOn ? 0.0f : 1.0f); break;
        case chain:
        {
            auto next = chainShown;
            if (e.mods.isPopupMenu())
            {
                if (index < (int) next.size() && next.size() > 1)
                    next.erase (next.begin() + index);
            }
            else if (index < (int) next.size())
                next[(size_t) index] = (next[(size_t) index] + 1) % amv::SeqBank::numPatterns;
            else if (next.size() < (size_t) amv::SeqBank::maxChain)
                next.push_back (0);

            processor.setSeqChain (next);
            break;
        }
        default: break;
    }

    refresh();
    repaint();
}

void PatternBar::mouseMove (const juce::MouseEvent& e)
{
    int index = -1;
    const int part = partAt (e.position, index);
    if (part != hoverPart || index != hoverIndex)
    {
        hoverPart = part;
        hoverIndex = index;
        repaint();
    }
}

void PatternBar::mouseExit (const juce::MouseEvent&)
{
    hoverPart = none;
    hoverIndex = -1;
    repaint();
}
