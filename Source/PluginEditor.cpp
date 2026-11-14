#include "PluginEditor.h"

namespace
{
constexpr int headerHeight = 52;
constexpr int keyboardHeight = 70;
constexpr int margin = 12;
constexpr int gap = 10;

juce::String utf8 (const char* text)
{
    return juce::String (juce::CharPointer_UTF8 (text));
}

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

    addSection (utf8 ("JU\xc5\xbdNI VETAR LEAD"), true, 0,
                { { id::tone, "TONE" }, { id::pulseWidth, "WIDTH" }, { id::detune, "DETUNE" },
                  { id::vibDepth, "VIBRATO" }, { id::vibRate, "VIB RATE" }, { id::vibDelay, "VIB DELAY" },
                  { id::bendRange, "BEND" }, { id::glide, "GLIDE" } });

    addSection ("FILTER", false, 1, { { id::cutoff, "CUTOFF" }, { id::resonance, "RES" }, { id::filterEnv, "ENV" } });
    addSection ("AMP ENV", false, 1, { { id::ampA, "A" }, { id::ampD, "D" }, { id::ampS, "S" }, { id::ampR, "R" } });
    addSection ("FILTER ENV", false, 1, { { id::fltA, "A" }, { id::fltD, "D" }, { id::fltS, "S" }, { id::fltR, "R" } });
    addSection ("OUT", false, 1, { { id::level, "LEVEL" } });

    monoAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (processor.apvts, id::mono, monoButton);
    addAndMakeVisible (monoButton);

    keyboard.setAvailableRange (36, 96);
    keyboard.setColour (juce::MidiKeyboardComponent::keyDownOverlayColourId, colour::red);
    keyboard.setColour (juce::MidiKeyboardComponent::mouseOverKeyOverlayColourId, colour::red.withAlpha (0.4f));
    addAndMakeVisible (keyboard);

    setResizable (true, true);
    getConstrainer()->setFixedAspectRatio (960.0 / 560.0);
    setResizeLimits (720, 420, 1920, 1120);
    setSize (960, 560);
}

AMVSynthAudioProcessorEditor::~AMVSynthAudioProcessorEditor()
{
    setLookAndFeel (nullptr);
}

AMVSynthAudioProcessorEditor::Section& AMVSynthAudioProcessorEditor::addSection (
    const juce::String& title, bool balkan, int row, std::initializer_list<std::pair<const char*, const char*>> params)
{
    Section section;
    section.title = title;
    section.balkan = balkan;
    section.row = row;
    section.weight = (float) params.size() + 0.4f;

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
        knob->attachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (processor.apvts, paramId, knob->slider);

        addAndMakeVisible (knob->slider);
        addAndMakeVisible (knob->label);
        section.knobs.push_back (knob.get());
        knobs.push_back (std::move (knob));
    }

    sections.push_back (std::move (section));
    return sections.back();
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

        auto titleArea = s.bounds.reduced (12, 0).removeFromTop (30);
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

void AMVSynthAudioProcessorEditor::resized()
{
    auto area = getLocalBounds();
    auto header = area.removeFromTop (headerHeight);
    monoButton.setBounds (header.removeFromRight (150).reduced (8, 12));

    keyboard.setBounds (area.removeFromBottom (keyboardHeight));
    keyboard.setKeyWidth ((float) keyboard.getWidth() / 36.0f); // 36 white keys from C2 to C7
    area.reduce (margin, margin);

    const int rowHeight = (area.getHeight() - gap) / 2;

    for (int row = 0; row < 2; ++row)
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

            auto inner = s.bounds.reduced (10, 8);
            inner.removeFromTop (26);
            const int knobWidth = inner.getWidth() / juce::jmax (1, (int) s.knobs.size());

            for (auto* knob : s.knobs)
            {
                auto cell = inner.removeFromLeft (knobWidth);
                const int size = juce::jmin (cell.getWidth(), cell.getHeight() - 20, 84);
                auto group = cell.withSizeKeepingCentre (cell.getWidth(), size + 20);
                knob->slider.setBounds (group.removeFromTop (size).withSizeKeepingCentre (size, size));
                knob->label.setBounds (group);
            }
        }
    }
}
