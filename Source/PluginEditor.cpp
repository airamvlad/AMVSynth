#include "PluginEditor.h"

namespace
{
constexpr int headerHeight = 52;
constexpr int keyboardHeight = 64;
constexpr int margin = 12;
constexpr int gap = 10;
constexpr int titleHeight = 28;

void drawKilim (juce::Graphics& g, juce::Rectangle<float> area, juce::Colour colour, int count)
{
    const float w = area.getWidth() / (float) count;
    const float cy = area.getCentreY();
    const float h = area.getHeight() * 0.5f;
    g.setColour (colour);

    for (int i = 0; i < count; ++i)
    {
        const float cx = area.getX() + w * ((float) i + 0.5f);
        juce::Path diamond;
        diamond.startNewSubPath (cx - w * 0.5f, cy);
        diamond.lineTo (cx, cy - h);
        diamond.lineTo (cx + w * 0.5f, cy);
        diamond.lineTo (cx, cy + h);
        diamond.closeSubPath();
        g.fillPath (diamond);
    }
}
} // namespace

namespace amv
{
LookAndFeel::LookAndFeel()
{
    setColour (juce::ResizableWindow::backgroundColourId, colour::background);
    setColour (juce::Label::textColourId, colour::dimText);
    setColour (juce::ToggleButton::textColourId, colour::text);
    setColour (juce::ToggleButton::tickColourId, colour::red);
    setColour (juce::ToggleButton::tickDisabledColourId, colour::dimText);
    setColour (juce::BubbleComponent::backgroundColourId, colour::knob);
    setColour (juce::BubbleComponent::outlineColourId, colour::panelBorder);
    setColour (juce::TooltipWindow::textColourId, colour::text);
    setColour (juce::ComboBox::backgroundColourId, colour::background);
    setColour (juce::ComboBox::textColourId, colour::text);
    setColour (juce::ComboBox::outlineColourId, colour::panelBorder);
    setColour (juce::ComboBox::arrowColourId, colour::dimText);
    setColour (juce::PopupMenu::backgroundColourId, colour::header);
    setColour (juce::PopupMenu::textColourId, colour::text);
    setColour (juce::PopupMenu::highlightedBackgroundColourId, colour::track);
    setColour (juce::PopupMenu::highlightedTextColourId, colour::text);
}

void LookAndFeel::drawToggleButton (juce::Graphics& g, juce::ToggleButton& button, bool highlighted, bool)
{
    // Drawn as a pill: filled with the accent colour when on, outlined when off.
    const auto area = button.getLocalBounds().toFloat().reduced (1.0f);
    const float radius = juce::jmin (area.getHeight() * 0.5f, 12.0f);
    const auto accent = button.findColour (juce::ToggleButton::tickColourId);
    const bool on = button.getToggleState();

    if (on)
    {
        g.setColour (highlighted ? accent.brighter (0.15f) : accent);
        g.fillRoundedRectangle (area, radius);
    }
    else
    {
        g.setColour (highlighted ? accent : accent.withAlpha (0.45f));
        g.drawRoundedRectangle (area, radius, 1.0f);
    }

    g.setColour (on ? colour::background : colour::text);
    g.setFont (juce::FontOptions (juce::jmin (12.0f, area.getHeight() * 0.55f), juce::Font::bold));
    g.drawText (button.getButtonText(), area, juce::Justification::centred);
}

void LookAndFeel::drawRotarySlider (juce::Graphics& g, int x, int y, int width, int height, float sliderPos,
                                    float startAngle, float endAngle, juce::Slider& slider)
{
    const auto area = juce::Rectangle<int> (x, y, width, height).toFloat().reduced (3.0f);
    const float size = juce::jmin (area.getWidth(), area.getHeight());
    const auto centre = area.getCentre();
    const float radius = size * 0.5f - 2.0f;
    const float angle = startAngle + sliderPos * (endAngle - startAngle);
    const auto accent = slider.findColour (juce::Slider::rotarySliderFillColourId);
    const juce::PathStrokeType stroke (3.5f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded);

    juce::Path trackArc;
    trackArc.addCentredArc (centre.x, centre.y, radius, radius, 0.0f, startAngle, endAngle, true);
    g.setColour (colour::track);
    g.strokePath (trackArc, stroke);

    juce::Path valueArc;
    valueArc.addCentredArc (centre.x, centre.y, radius, radius, 0.0f, startAngle, angle, true);
    g.setColour (accent);
    g.strokePath (valueArc, stroke);

    const float inner = radius - 6.0f;
    g.setColour (colour::knob);
    g.fillEllipse (centre.x - inner, centre.y - inner, inner * 2.0f, inner * 2.0f);

    juce::Path pointer;
    pointer.addRoundedRectangle (-1.25f, -inner + 2.0f, 2.5f, inner * 0.55f, 1.0f);
    g.setColour (colour::text);
    g.fillPath (pointer, juce::AffineTransform::rotation (angle).translated (centre.x, centre.y));
}
} // namespace amv

AMVSynthAudioProcessorEditor::AMVSynthAudioProcessorEditor (AMVSynthAudioProcessor& p)
    : AudioProcessorEditor (&p),
      processor (p),
      keyboard (p.keyboardState, juce::MidiKeyboardComponent::horizontalKeyboard)
{
    using namespace amv;
    setLookAndFeel (&lookAndFeel);

    // Row 0: the sound source and how it is played.
    {
        auto& s = addSection ("BALKAN ENGINE", true, 0, 5.0f);
        engineSection = sections.size() - 1;
        s.column.push_back (&makeChoice (id::model, true)->box);

        modelKnobs.push_back (makeKnobs (true, { { id::tone, "TONE" }, { id::pulseWidth, "WIDTH" }, { id::detune, "DETUNE" }, { id::sub, "SUB" } }));
        modelKnobs.push_back (makeKnobs (true, { { id::accBellows, "BELLOWS" }, { id::accMusette, "MUSETTE" }, { id::accRegister, "REGISTER" }, { id::accClick, "CLICK" } }));
        modelKnobs.push_back (makeKnobs (true, { { id::gusBow, "BOW" }, { id::gusPosition, "POSITION" }, { id::gusRasp, "RASP" }, { id::gusNasal, "NASAL" } }));
        modelKnobs.push_back (makeKnobs (true, { { id::fruBreath, "BREATH" }, { id::fruOverblow, "OVERBLOW" }, { id::fruNoise, "NOISE" }, { id::fruChiff, "CHIFF" } }));
        modelKnobs.push_back (makeKnobs (true, { { id::kavBreath, "BREATH" }, { id::kavOverblow, "OVERBLOW" }, { id::kavNoise, "NOISE" }, { id::kavThroat, "THROAT" } }));
        modelKnobs.push_back (makeKnobs (true, { { id::gajDrone, "DRONE" }, { id::gajDroneTune, "DRONE TUNE" }, { id::gajBag, "BAG" }, { id::gajEdge, "EDGE" } }));
    }
    {
        auto& s = addSection ("PERFORMANCE", false, 0, 6.0f);
        s.column.push_back (&makeToggle (id::mono, "MONO LEGATO", false)->button);
        s.knobs = makeKnobs (false, { { id::vibDepth, "VIBRATO" }, { id::vibRate, "VIB RATE" }, { id::vibDelay, "VIB DELAY" },
                                      { id::bendRange, "BEND" }, { id::glide, "GLIDE" } });
    }

    // Row 1: scale and ornaments.
    {
        auto& s = addSection ("SCALE", true, 1, 3.0f);
        s.column.push_back (&makeChoice (id::scaleRoot, true)->box);
        s.column.push_back (&makeChoice (id::scale, true)->box);
        s.column.push_back (&makeToggle (id::snap, "SNAP TO SCALE", true)->button);
    }
    {
        auto& s = addSection ("ORNAMENTS", true, 1, 8.0f);
        s.columnsInColumn = 2;
        s.column.push_back (&makeToggle (id::ornGrace, "GRACE", true)->button);
        s.column.push_back (&makeToggle (id::ornMordent, "MORDENT", true)->button);
        s.column.push_back (&makeToggle (id::ornTrill, "TRILL", true)->button);
        s.column.push_back (&makeToggle (id::ornFall, "FALL", true)->button);
        s.knobs = makeKnobs (true, { { id::ornSpeed, "SPEED" }, { id::ornAmount, "AMOUNT" }, { id::ornChance, "CHANCE" } });
    }

    // Row 2: the odd-meter arpeggiator.
    {
        auto& s = addSection ("ODD-METER ARP", true, 2, 1.0f);
        s.columnsInColumn = 3;
        s.column.push_back (&makeToggle (id::arpOn, "ARP ON", true)->button);
        s.column.push_back (&makeChoice (id::arpMeter, true)->box);
        s.column.push_back (&makeChoice (id::arpRate, true)->box);
        s.column.push_back (&makeToggle (id::arpPolymeter, "POLYMETER", true)->button);
        s.column.push_back (&makeChoice (id::arpMode, true)->box);
        s.display = &patternDisplay;
        s.knobs = makeKnobs (true, { { id::arpOctaves, "OCTAVES" }, { id::arpGate, "GATE" }, { id::arpAccent, "ACCENT" } });
        addAndMakeVisible (patternDisplay);
    }

    // Row 3: filter, envelopes, output.
    addSection ("FILTER", false, 3, 3.4f).knobs = makeKnobs (false, { { id::cutoff, "CUTOFF" }, { id::resonance, "RES" }, { id::filterEnv, "ENV" } });
    addSection ("AMP ENV", false, 3, 4.4f).knobs = makeKnobs (false, { { id::ampA, "A" }, { id::ampD, "D" }, { id::ampS, "S" }, { id::ampR, "R" } });
    addSection ("FILTER ENV", false, 3, 4.4f).knobs = makeKnobs (false, { { id::fltA, "A" }, { id::fltD, "D" }, { id::fltS, "S" }, { id::fltR, "R" } });
    addSection ("OUT", false, 3, 1.4f).knobs = makeKnobs (false, { { id::level, "LEVEL" } });

    keyboard.setAvailableRange (36, 96);
    keyboard.setColour (juce::MidiKeyboardComponent::keyDownOverlayColourId, colour::red);
    keyboard.setColour (juce::MidiKeyboardComponent::mouseOverKeyOverlayColourId, colour::red.withAlpha (0.4f));
    addAndMakeVisible (keyboard);

    showModel (juce::roundToInt (processor.apvts.getRawParameterValue (id::model)->load()));
    startTimerHz (20);

    setResizable (true, true);
    getConstrainer()->setFixedAspectRatio (1040.0 / 840.0);
    setResizeLimits (780, 630, 2080, 1680);
    setSize (1040, 840);
}

AMVSynthAudioProcessorEditor::~AMVSynthAudioProcessorEditor()
{
    stopTimer();
    setLookAndFeel (nullptr);
}

std::vector<AMVSynthAudioProcessorEditor::Knob*> AMVSynthAudioProcessorEditor::makeKnobs (bool balkan, ParamList params)
{
    std::vector<Knob*> result;

    for (const auto& [paramId, name] : params)
    {
        auto knob = std::make_unique<Knob>();
        knob->slider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
        knob->slider.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
        knob->slider.setPopupDisplayEnabled (true, true, this);
        knob->slider.setColour (juce::Slider::rotarySliderFillColourId, balkan ? amv::colour::red : amv::colour::teal);
        knob->label.setText (name, juce::dontSendNotification);
        knob->label.setJustificationType (juce::Justification::centred);
        knob->label.setFont (juce::FontOptions (11.0f));
        knob->attachment = std::make_unique<APVTS::SliderAttachment> (processor.apvts, paramId, knob->slider);

        addChildComponent (knob->slider);
        addChildComponent (knob->label);
        result.push_back (knob.get());
        knobs.push_back (std::move (knob));
    }

    return result;
}

AMVSynthAudioProcessorEditor::Toggle* AMVSynthAudioProcessorEditor::makeToggle (const char* paramId, const juce::String& text, bool balkan)
{
    auto toggle = std::make_unique<Toggle>();
    toggle->button.setButtonText (text);
    toggle->button.setColour (juce::ToggleButton::tickColourId, balkan ? amv::colour::red : amv::colour::teal);
    toggle->attachment = std::make_unique<APVTS::ButtonAttachment> (processor.apvts, paramId, toggle->button);
    addAndMakeVisible (toggle->button);
    toggles.push_back (std::move (toggle));
    return toggles.back().get();
}

AMVSynthAudioProcessorEditor::Choice* AMVSynthAudioProcessorEditor::makeChoice (const char* paramId, bool balkan)
{
    auto choice = std::make_unique<Choice>();

    if (auto* param = dynamic_cast<juce::AudioParameterChoice*> (processor.apvts.getParameter (paramId)))
        choice->box.addItemList (param->choices, 1);

    choice->box.setColour (juce::ComboBox::outlineColourId, balkan ? amv::colour::balkanBorder : amv::colour::panelBorder);
    choice->attachment = std::make_unique<APVTS::ComboBoxAttachment> (processor.apvts, paramId, choice->box);
    addAndMakeVisible (choice->box);
    choices.push_back (std::move (choice));
    return choices.back().get();
}

AMVSynthAudioProcessorEditor::Section& AMVSynthAudioProcessorEditor::addSection (const juce::String& title, bool balkan, int row, float weight)
{
    Section section;
    section.title = title;
    section.balkan = balkan;
    section.row = row;
    section.weight = weight;
    sections.push_back (std::move (section));
    return sections.back();
}

void AMVSynthAudioProcessorEditor::showModel (int index)
{
    index = juce::jlimit (0, (int) modelKnobs.size() - 1, index);
    if (index == shownModel)
        return;

    shownModel = index;

    for (size_t m = 0; m < modelKnobs.size(); ++m)
        for (auto* knob : modelKnobs[m])
        {
            knob->slider.setVisible ((int) m == index);
            knob->label.setVisible ((int) m == index);
        }

    sections[engineSection].knobs = modelKnobs[(size_t) index];
    layoutSection (sections[engineSection]);
}

void AMVSynthAudioProcessorEditor::timerCallback()
{
    // The model can also change from host automation or a loaded preset.
    showModel (juce::roundToInt (processor.apvts.getRawParameterValue (amv::id::model)->load()));

    const int meter = juce::roundToInt (processor.apvts.getRawParameterValue (amv::id::arpMeter)->load());
    if (meter != patternDisplay.meter)
    {
        patternDisplay.meter = meter;
        patternDisplay.repaint();
    }
}

void AMVSynthAudioProcessorEditor::PatternDisplay::paint (juce::Graphics& g)
{
    using namespace amv;
    auto area = getLocalBounds().toFloat();
    g.setColour (colour::background);
    g.fillRoundedRectangle (area, 6.0f);
    area.reduce (8.0f, 8.0f);

    const auto& groups = getMeters()[(size_t) juce::jlimit (0, (int) getMeters().size() - 1, meter)].groups;
    const int steps = Arpeggiator::patternLength (meter);
    const float groupGap = 8.0f, stepGap = 3.0f;
    const float stepWidth = (area.getWidth() - groupGap * (float) (groups.size() - 1) - stepGap * (float) (steps - (int) groups.size())) / (float) steps;
    float x = area.getX();

    for (int group : groups)
    {
        for (int i = 0; i < group; ++i)
        {
            const float height = area.getHeight() * (i == 0 ? 1.0f : 0.55f);
            g.setColour (i == 0 ? colour::red : colour::teal.withAlpha (0.55f));
            g.fillRoundedRectangle (x, area.getBottom() - height, stepWidth, height, 2.5f);
            x += stepWidth + stepGap;
        }

        x += groupGap - stepGap;
    }
}

void AMVSynthAudioProcessorEditor::paint (juce::Graphics& g)
{
    using namespace amv;
    g.fillAll (colour::background);

    auto header = getLocalBounds().removeFromTop (headerHeight);
    g.setColour (colour::header);
    g.fillRect (header);
    g.setColour (colour::panelBorder);
    g.fillRect (header.removeFromBottom (1));

    auto logo = header.reduced (16, 0);
    drawKilim (g, logo.removeFromLeft (17).toFloat().withSizeKeepingCentre (17.0f, 20.0f), colour::red, 1);
    drawKilim (g, logo.removeFromLeft (17).toFloat().withSizeKeepingCentre (17.0f, 20.0f), colour::teal, 1);
    logo.removeFromLeft (10);
    g.setColour (colour::text);
    g.setFont (juce::FontOptions (19.0f, juce::Font::bold));
    g.drawText ("AMVSYNTH", logo.removeFromLeft (140), juce::Justification::centredLeft);
    g.setColour (colour::dimText);
    g.setFont (juce::FontOptions (11.0f));
    g.drawText ("v" JucePlugin_VersionString "  Balkan psy synthesizer", logo.removeFromLeft (260), juce::Justification::centredLeft);

    for (const auto& s : sections)
    {
        const auto r = s.bounds.toFloat();
        g.setColour (s.balkan ? colour::balkanPanel : colour::panel);
        g.fillRoundedRectangle (r, 8.0f);
        g.setColour (s.balkan ? colour::balkanBorder : colour::panelBorder);
        g.drawRoundedRectangle (r.reduced (0.5f), 8.0f, 1.0f);

        auto titleArea = s.bounds.reduced (12, 0).removeFromTop (titleHeight + 4);
        if (s.balkan)
        {
            drawKilim (g, titleArea.removeFromLeft (30).toFloat().withSizeKeepingCentre (30.0f, 10.0f), colour::red, 3);
            titleArea.removeFromLeft (8);
        }

        g.setColour (s.balkan ? colour::red : colour::teal);
        g.setFont (juce::FontOptions (12.5f, juce::Font::bold));
        g.drawText (s.title, titleArea, juce::Justification::centredLeft);
    }
}

void AMVSynthAudioProcessorEditor::layoutSection (Section& s)
{
    if (s.bounds.isEmpty())
        return;

    auto inner = s.bounds.reduced (12, 8);
    inner.removeFromTop (titleHeight);

    if (! s.column.empty())
    {
        // Choices and toggles sit in a grid on the left, or fill the panel when there are no knobs.
        const int cols = s.columnsInColumn;
        const int rows = ((int) s.column.size() + cols - 1) / cols;
        const int cellWidth = s.knobs.empty() ? inner.getWidth() / cols : juce::jmin (150, inner.getWidth() / 3);
        auto columnArea = inner.removeFromLeft (cellWidth * cols);
        inner.removeFromLeft (s.knobs.empty() ? 0 : 10);

        const int cellHeight = juce::jmin (34, columnArea.getHeight() / rows);
        auto grid = columnArea.withSizeKeepingCentre (columnArea.getWidth(), cellHeight * rows);

        for (size_t i = 0; i < s.column.size(); ++i)
        {
            const int col = (int) i % cols, row = (int) i / cols;
            s.column[i]->setBounds (juce::Rectangle<int> (grid.getX() + col * cellWidth, grid.getY() + row * cellHeight, cellWidth, cellHeight)
                                        .reduced (3, 4));
        }
    }

    if (s.display != nullptr)
    {
        s.display->setBounds (inner.removeFromLeft (inner.getWidth() * 2 / 5).reduced (0, 6));
        inner.removeFromLeft (10);
    }

    if (s.knobs.empty())
        return;

    const int knobWidth = inner.getWidth() / (int) s.knobs.size();

    for (auto* knob : s.knobs)
    {
        auto cell = inner.removeFromLeft (knobWidth);
        const int size = juce::jmin (cell.getWidth(), cell.getHeight() - 20, 80);
        auto group = cell.withSizeKeepingCentre (cell.getWidth(), size + 20);
        knob->slider.setBounds (group.removeFromTop (size).withSizeKeepingCentre (size, size));
        knob->label.setBounds (group);
        knob->slider.setVisible (true);
        knob->label.setVisible (true);
    }
}

void AMVSynthAudioProcessorEditor::resized()
{
    auto area = getLocalBounds();
    area.removeFromTop (headerHeight);

    keyboard.setBounds (area.removeFromBottom (keyboardHeight));
    keyboard.setKeyWidth ((float) keyboard.getWidth() / 36.0f); // 36 white keys from C2 to C7
    area.reduce (margin, margin);

    constexpr int numRows = 4;
    const int rowHeight = (area.getHeight() - gap * (numRows - 1)) / numRows;

    for (int row = 0; row < numRows; ++row)
    {
        auto rowArea = area.removeFromTop (rowHeight);
        area.removeFromTop (gap);

        float totalWeight = 0.0f;
        int count = 0;
        for (const auto& s : sections)
            if (s.row == row)
            {
                totalWeight += s.weight;
                ++count;
            }

        if (count == 0)
            continue;

        const float available = (float) (rowArea.getWidth() - gap * (count - 1));
        int index = 0;

        for (auto& s : sections)
        {
            if (s.row != row)
                continue;

            ++index;
            const int width = index == count ? rowArea.getWidth()
                                             : juce::roundToInt (available * s.weight / totalWeight);
            s.bounds = rowArea.removeFromLeft (width);
            rowArea.removeFromLeft (gap);
            layoutSection (s);
        }
    }
}
