#include "PluginEditor.h"

namespace
{
constexpr int headerHeight = 52;
constexpr int keyboardHeight = 64;
constexpr int margin = 12;
constexpr int gap = 10;
constexpr int titleHeight = 28;
constexpr int sourceBarHeight = 30;

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
    // The outermost ring is left free: knobs that can be modulated draw their modulation there.
    const auto area = juce::Rectangle<int> (x, y, width, height).toFloat().reduced (2.0f);
    const float size = juce::jmin (area.getWidth(), area.getHeight());
    const auto centre = area.getCentre();
    const float radius = size * 0.5f - 6.5f;
    const float angle = startAngle + sliderPos * (endAngle - startAngle);
    const auto accent = slider.findColour (juce::Slider::rotarySliderFillColourId);
    const juce::PathStrokeType stroke (3.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded);

    juce::Path trackArc;
    trackArc.addCentredArc (centre.x, centre.y, radius, radius, 0.0f, startAngle, endAngle, true);
    g.setColour (colour::track);
    g.strokePath (trackArc, stroke);

    juce::Path valueArc;
    valueArc.addCentredArc (centre.x, centre.y, radius, radius, 0.0f, startAngle, angle, true);
    g.setColour (accent.withAlpha (0.25f));
    g.strokePath (valueArc, juce::PathStrokeType (6.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded)); // soft glow
    g.setColour (accent);
    g.strokePath (valueArc, stroke);

    const float body = radius - 5.0f;
    g.setGradientFill (juce::ColourGradient (colour::knob.brighter (0.22f), centre.x, centre.y - body,
                                             colour::knob.darker (0.35f), centre.x, centre.y + body, false));
    g.fillEllipse (centre.x - body, centre.y - body, body * 2.0f, body * 2.0f);
    g.setColour (juce::Colours::white.withAlpha (0.07f));
    g.drawEllipse (centre.x - body + 0.5f, centre.y - body + 0.5f, body * 2.0f - 1.0f, body * 2.0f - 1.0f, 1.0f);

    juce::Path pointer;
    pointer.addRoundedRectangle (-1.1f, -body + 2.5f, 2.2f, body * 0.5f, 1.1f);
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
        modelKnobs.push_back (makeKnobs (true, { { id::zurPressure, "PRESSURE" }, { id::zurBite, "BITE" }, { id::zurGrowl, "GROWL" }, { id::zurNasal, "NASAL" } }));
        modelKnobs.push_back (makeKnobs (true, { { id::tamPluck, "PLUCK" }, { id::tamDecay, "DECAY" }, { id::tamDouble, "DOUBLE" }, { id::tamTone, "TONE" } }));
        modelKnobs.push_back (makeKnobs (true, { { id::tapThump, "THUMP" }, { id::tapSnap, "SNAP" }, { id::tapDecay, "DECAY" }, { id::tapRing, "RING" } }));
        modelKnobs.push_back (makeKnobs (true, { { id::dvoBreath, "BREATH" }, { id::dvoSecond, "SECOND" }, { id::dvoInterval, "INTERVAL" }, { id::dvoBeat, "BEATING" } }));
        modelKnobs.push_back (makeKnobs (true, { { id::claBreath, "BREATH" }, { id::claReed, "REED" }, { id::claGrowl, "GROWL" }, { id::claBody, "BODY" } }));
        modelKnobs.push_back (makeKnobs (true, { { id::truBlow, "BLOW" }, { id::truBlare, "BLARE" }, { id::truRasp, "RASP" }, { id::truMute, "MUTE" } }));
        modelKnobs.push_back (makeKnobs (true, { { id::vioBow, "BOW" }, { id::vioPosition, "POSITION" }, { id::vioScratch, "SCRATCH" }, { id::vioBody, "BODY" } }));
        modelKnobs.push_back (makeKnobs (true, { { id::sarPluck, "PLUCK" }, { id::sarDecay, "DECAY" }, { id::sarDrone, "DRONE" }, { id::sarTone, "TONE" } }));
        modelKnobs.push_back (makeKnobs (true, { { id::kanPluck, "PLUCK" }, { id::kanDecay, "DECAY" }, { id::kanShimmer, "SHIMMER" }, { id::kanTremolo, "TREMOLO" } }));
        modelKnobs.push_back (makeKnobs (true, { { id::darStrike, "STRIKE" }, { id::darSnap, "SNAP" }, { id::darDecay, "DECAY" }, { id::darRing, "RING" } }));
        modelKnobs.push_back (makeKnobs (true, { { id::voxVowel, "VOWEL" }, { id::voxBreath, "BREATH" }, { id::voxSize, "SIZE" }, { id::voxTension, "TENSION" } }));
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
        // The reference pitch is a number box, so an exact value such as 432 can be typed in.
        tuneBox.setSliderStyle (juce::Slider::IncDecButtons);
        tuneBox.setIncDecButtonsMode (juce::Slider::incDecButtonsDraggable_Vertical);
        tuneBox.setTextBoxStyle (juce::Slider::TextBoxLeft, false, 78, 22);
        tuneBox.setColour (juce::Slider::textBoxTextColourId, colour::text);
        tuneBox.setColour (juce::Slider::textBoxBackgroundColourId, colour::background);
        tuneBox.setColour (juce::Slider::textBoxOutlineColourId, colour::panelBorder);
        tuneBox.setColour (juce::Slider::textBoxHighlightColourId, colour::track);
        tuneBox.setColour (juce::TextButton::buttonColourId, colour::knob);
        tuneBox.setColour (juce::TextButton::textColourOffId, colour::text);
        tuneBox.setTooltip ("Frequency of the A above middle C. Click the number to type a value, for example 432.");
        tuneAttachment = std::make_unique<APVTS::SliderAttachment> (processor.apvts, id::tuneA4, tuneBox);
        tuneBox.setTextValueSuffix (" Hz");
        addChildComponent (tuneBox);

        // Tuning and MPE are set once per project, so they live on the SETUP page.
        buildingPage = 4;
        auto& s = addSection ("TUNING A4 / MPE", false, 0, 3.0f);
        s.columnWidth = 132;
        s.column.push_back (&tuneBox);
        s.column.push_back (&makeToggle (id::mpe, "MPE", false)->button);
        s.knobs = makeKnobs (false, { { id::mpeBend, "MPE BEND" } });

        helpDisplay.lines = { "Drag a source from the MOD SOURCES row onto a knob to modulate it.",
                              "Shift-drag a modulated knob up or down to set how far it moves.",
                              "Right-click a knob to add, invert or remove a modulation.",
                              "The MOD page lists every modulation and has the LFO and macro settings." };
        addSection ("MODULATION", false, 0, 7.0f).display = &helpDisplay;
        addChildComponent (helpDisplay);
        buildingPage = 0;
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

    // Row 2: the odd-meter arpeggiator and the macros.
    {
        auto& s = addSection ("ODD-METER ARP", true, 2, 6.6f);
        s.columnsInColumn = 2;
        s.columnWidth = 114;
        s.displayShare = 0.36f;
        s.column.push_back (&makeToggle (id::arpOn, "ARP ON", true)->button);
        s.column.push_back (&makeChoice (id::arpMeter, true)->box);
        s.column.push_back (&makeChoice (id::arpRate, true)->box);
        s.column.push_back (&makeChoice (id::arpMode, true)->box);
        s.column.push_back (&makeToggle (id::arpPolymeter, "POLYMETER", true)->button);
        s.display = &patternDisplay;
        s.knobs = makeKnobs (true, { { id::arpOctaves, "OCTAVES" }, { id::arpGate, "GATE" }, { id::arpAccent, "ACCENT" } });
        addChildComponent (patternDisplay);
    }
    {
        // The four macros, with the names the preset gave them. A double click on a name changes it.
        auto& s = addSection ("MACROS", false, 2, 4.4f);
        s.knobs = makeKnobs (false, { { id::macro (0), "MACRO 1" }, { id::macro (1), "MACRO 2" }, { id::macro (2), "MACRO 3" }, { id::macro (3), "MACRO 4" } });
        for (size_t i = 0; i < s.knobs.size(); ++i)
        {
            auto& label = s.knobs[i]->label;
            macroLabels[i] = &label;
            label.setEditable (false, true, false);
            label.setColour (juce::Label::textColourId, colour::forSource (mod::srcMacro1 + (int) i));
            label.setColour (juce::Label::textWhenEditingColourId, colour::text);
            label.setColour (juce::Label::backgroundWhenEditingColourId, colour::background);
            label.setTooltip ("Double-click to name this macro. Drag M" + juce::String (i + 1) + " from the MOD SOURCES row onto a knob to let the macro move it.");
            label.onTextChange = [this, i]
            {
                processor.setMacroName ((int) i, macroLabels[i]->getText());
                macroLabels[i]->setText (processor.getMacroName ((int) i), juce::dontSendNotification);
            };
        }
    }

    // Row 3: filter, envelopes, output.
    {
        auto& s = addSection ("FILTER", false, 3, 4.6f);
        s.columnWidth = 106;
        s.column.push_back (&makeChoice (id::filterType, false)->box);
        s.knobs = makeKnobs (false, { { id::cutoff, "CUTOFF" }, { id::resonance, "RES" }, { id::filterEnv, "ENV" } });
    }

    // The envelopes are drawn as curves with handles to drag.
    ampEnvDisplay.setParameters (processor.apvts, id::ampA, id::ampD, id::ampS, id::ampR);
    filterEnvDisplay.setParameters (processor.apvts, id::fltA, id::fltD, id::fltS, id::fltR);
    // The graph and the knobs show the same four values; either one can be used.
    {
        auto& s = addSection ("AMP ENV", false, 3, 4.9f);
        s.display = &ampEnvDisplay;
        s.displayShare = 0.38f;
        s.knobs = makeKnobs (false, { { id::ampA, "A" }, { id::ampD, "D" }, { id::ampS, "S" }, { id::ampR, "R" } });
    }
    {
        auto& s = addSection ("FILTER ENV", false, 3, 4.9f);
        s.display = &filterEnvDisplay;
        s.displayShare = 0.38f;
        s.knobs = makeKnobs (false, { { id::fltA, "A" }, { id::fltD, "D" }, { id::fltS, "S" }, { id::fltR, "R" } });
    }
    addChildComponent (ampEnvDisplay);
    addChildComponent (filterEnvDisplay);

    // OSC page: two wavetable oscillators, sub, noise and the level of the Balkan engine.
    buildingPage = 1;
    for (size_t i = 0; i < importButtons.size(); ++i)
    {
        auto& button = importButtons[i];
        button.setButtonText ("EDIT");
        button.setTooltip ("Open the wavetable editor for this oscillator: draw frames, set harmonics, import and export tables.");
        button.setColour (juce::TextButton::buttonColourId, colour::knob);
        button.setColour (juce::TextButton::textColourOffId, colour::text);
        button.onClick = [this, i]
        {
            waveEditor.setOscillator ((int) i);
            setPage (5);
        };
        addChildComponent (button);
    }
    {
        auto& s = addSection ("OSC A", false, 0, 1.0f);
        s.column.push_back (&makeChoice (id::oscATable, false)->box);
        s.column.push_back (&importButtons[0]);
        s.column.push_back (&makeChoice (id::oscAWarp, false)->box);
        s.column.push_back (&makeChoice (id::oscAXmodType, false)->box);
        s.column.push_back (&makeChoice (id::oscAXmodSource, false)->box);
        s.columnsInColumn = 2;
        s.columnWidth = 112;
        s.displayShare = 0.3f;
        s.display = &waveA;
        s.knobs = makeKnobs (false, { { id::oscAPos, "POSITION" }, { id::oscAWarpAmount, "WARP" }, { id::oscAXmodAmount, "FM/RING" }, { id::oscAUnison, "UNISON" }, { id::oscADetune, "DETUNE" }, { id::oscAWidth, "WIDTH" },
                                      { id::oscAPitch, "PITCH" }, { id::oscAFine, "FINE" }, { id::oscALevel, "LEVEL" } });
        addChildComponent (waveA);
    }
    {
        auto& s = addSection ("OSC B", false, 1, 1.0f);
        s.column.push_back (&makeChoice (id::oscBTable, false)->box);
        s.column.push_back (&importButtons[1]);
        s.column.push_back (&makeChoice (id::oscBWarp, false)->box);
        s.column.push_back (&makeChoice (id::oscBXmodType, false)->box);
        s.column.push_back (&makeChoice (id::oscBXmodSource, false)->box);
        s.columnsInColumn = 2;
        s.columnWidth = 112;
        s.displayShare = 0.3f;
        s.display = &waveB;
        s.knobs = makeKnobs (false, { { id::oscBPos, "POSITION" }, { id::oscBWarpAmount, "WARP" }, { id::oscBXmodAmount, "FM/RING" }, { id::oscBUnison, "UNISON" }, { id::oscBDetune, "DETUNE" }, { id::oscBWidth, "WIDTH" },
                                      { id::oscBPitch, "PITCH" }, { id::oscBFine, "FINE" }, { id::oscBLevel, "LEVEL" } });
        addChildComponent (waveB);
    }
    {
        auto& s = addSection ("SUB", false, 2, 3.4f);
        s.column.push_back (&makeChoice (id::subOctave, false)->box);
        s.knobs = makeKnobs (false, { { id::subShape, "SHAPE" }, { id::subLevel, "LEVEL" } });
    }
    addSection ("NOISE", false, 2, 2.4f).knobs = makeKnobs (false, { { id::noiseColor, "COLOR" }, { id::noiseLevel, "LEVEL" } });
    {
        // The model can be stacked: a detuned unison of it, or the notes of a chord from one key.
        auto& s = addSection ("BALKAN ENGINE", true, 2, 5.0f);
        s.columnWidth = 104;
        s.column.push_back (&makeChoice (id::engineStackMode, true)->box);
        s.knobs = makeKnobs (true, { { id::engineStack, "STACK" }, { id::engineStackDetune, "DETUNE" }, { id::engineStackSpread, "SPREAD" }, { id::engineLevel, "LEVEL" } });
    }
    {
        // The second filter and which filter each source runs through.
        auto& s = addSection ("FILTER 2", false, 3, 4.0f);
        s.columnWidth = 128;
        s.column.push_back (&makeToggle (id::filter2On, "FILTER 2 ON", false)->button);
        s.column.push_back (&makeChoice (id::filter2Type, false)->box);
        s.column.push_back (&makeChoice (id::filterRouting, false)->box);
        s.knobs = makeKnobs (false, { { id::filter2Cutoff, "CUTOFF" }, { id::filter2Res, "RES" }, { id::filter2Env, "ENV" } });
    }
    {
        auto& s = addSection ("FILTER ROUTING", false, 3, 5.0f);
        s.columnsInColumn = 3;
        const std::pair<const char*, const char*> routes[] { { id::routeEngine, "ENGINE" }, { id::routeOscA, "OSC A" }, { id::routeOscB, "OSC B" },
                                                             { id::routeSub, "SUB" }, { id::routeNoise, "NOISE" }, { id::routeSampler, "SAMPLER" } };
        for (const auto& [paramId, name] : routes)
        {
            auto* choice = makeChoice (paramId, false);
            // Each box shows its source in front of the filter it goes to.
            if (auto* param = dynamic_cast<juce::AudioParameterChoice*> (processor.apvts.getParameter (paramId)))
            {
                const int selected = choice->box.getSelectedId();
                choice->box.clear (juce::dontSendNotification);
                for (int i = 0; i < param->choices.size(); ++i)
                    choice->box.addItem (juce::String (name) + ":  " + param->choices[i], i + 1);
                choice->box.setSelectedId (selected, juce::dontSendNotification);
            }
            choice->box.setTooltip ("The filter this source runs through when FILTER 2 is on. In Serial mode filter 1 runs into filter 2.");
            s.column.push_back (&choice->box);
        }
    }

    // SAMPLE page: the sampler with its classic and granular modes.
    buildingPage = 6;
    {
        const char* names[numSampleTools] { "LOAD", "NORMALIZE", "CROP", "FADE EDGES", "FIND ROOT", "TO WAVE A", "TO WAVE B", "CLEAR" };
        const char* tips[numSampleTools] { "Load an audio file (WAV, AIFF, FLAC, MP3 or OGG). The first 60 seconds are used and saved with the project.",
                                           "Bring the loudest peak of the sample to full level.",
                                           "Keep only the part between START and END.",
                                           "Fade the first and last few milliseconds, so the sample starts and ends without a click.",
                                           "Find the pitch of the sample and set ROOT to it.",
                                           "Cut 16 single cycles between START and END into the User wavetable of oscillator A. ROOT has to match the pitch.",
                                           "Cut 16 single cycles between START and END into the User wavetable of oscillator B. ROOT has to match the pitch.",
                                           "Remove the sample from the project." };
        for (int i = 0; i < numSampleTools; ++i)
        {
            auto& button = sampleButtons[(size_t) i];
            button.setButtonText (names[i]);
            button.setTooltip (tips[i]);
            button.setColour (juce::TextButton::buttonColourId, colour::knob);
            button.setColour (juce::TextButton::textColourOffId, colour::text);
            button.onClick = [this, i] { runSampleTool (i); };
            addChildComponent (button);
        }

        sampleDisplay.onFileDropped = [this] (bool ok)
        {
            if (ok)
            {
                setParameterValue (id::smpStart, 0.0f);
                setParameterValue (id::smpEnd, 1.0f);
                setParameterValue (id::smpLoopStart, 0.0f);
                setParameterValue (id::smpLoopEnd, 1.0f);
                if (processor.apvts.getRawParameterValue (id::smpLevel)->load() <= 0.0f)
                    setParameterValue (id::smpLevel, 0.8f);
            }
            showStatus (ok ? "Sample loaded." : "This file could not be read as audio.");
        };

        auto& s = addSection ("SAMPLE", false, 0, 1.0f);
        s.column.push_back (&makeChoice (id::smpMode, false)->box);
        s.column.push_back (&makeChoice (id::smpLoop, false)->box);
        s.column.push_back (&makeToggle (id::smpKeyTrack, "KEY TRACK", false)->button);
        s.column.push_back (&makeToggle (id::smpReverse, "REVERSE", false)->button);
        s.columnWidth = 126;
        s.display = &sampleDisplay;
        s.displayShare = 0.88f;
        s.knobs = makeKnobs (false, { { id::smpLevel, "LEVEL" } });
        addChildComponent (sampleDisplay);
    }
    addSection ("PLAYBACK", false, 1, 1.0f).knobs = makeKnobs (false, { { id::smpStart, "START" }, { id::smpEnd, "END" }, { id::smpLoopStart, "LOOP START" },
                                                                        { id::smpLoopEnd, "LOOP END" }, { id::smpCrossfade, "XFADE" }, { id::smpRoot, "ROOT" },
                                                                        { id::smpPitch, "PITCH" }, { id::smpFine, "FINE" }, { id::smpPan, "PAN" } });
    addSection ("GRAINS", false, 2, 1.0f).knobs = makeKnobs (false, { { id::smpPosition, "POSITION" }, { id::smpGrainSize, "SIZE" }, { id::smpDensity, "DENSITY" },
                                                                      { id::smpSpray, "SPRAY" }, { id::smpPitchSpray, "PITCH SPRAY" }, { id::smpScan, "SCAN" },
                                                                      { id::smpShape, "SHAPE" }, { id::smpSpread, "SPREAD" }, { id::smpRevChance, "REVERSE" } });
    {
        auto& s = addSection ("TOOLS", false, 3, 5.0f);
        for (auto& button : sampleButtons)
            s.column.push_back (&button);
        s.columnsInColumn = 4;
        addSection ("INFO", false, 3, 4.0f).display = &sampleInfo;
        addChildComponent (sampleInfo);
        updateSampleInfo();
    }

    // SEQ page: the step grid over three rows, its settings and tools below.
    buildingPage = seqPage;
    {
        addSection ("ODD-METER STEP SEQUENCER", true, 0, 1.0f).display = &stepEditor;
        addChildComponent (stepEditor);

        auto& s = addSection ("SEQUENCER", true, 3, 5.0f);
        s.column.push_back (&makeToggle (id::seqOn, "SEQ ON", true)->button);
        s.column.push_back (&makeChoice (id::seqMeter, true)->box);
        s.column.push_back (&makeToggle (id::seqLatch, "LATCH", true)->button);
        s.column.push_back (&makeChoice (id::seqRate, true)->box);
        s.column.push_back (&makeToggle (id::seqFreeCycle, "FREE CYCLE", true)->button);
        s.columnsInColumn = 2;
        s.columnWidth = 122;
        s.knobs = makeKnobs (true, { { id::seqSteps, "STEPS" }, { id::seqGate, "GATE" }, { id::seqSwing, "SWING" } });

        addSection ("PATTERNS", true, 3, 3.1f).display = &patternBar;
        addChildComponent (patternBar);

        const char* names[numSeqTools] { "1 BAR", "2 BARS", "ACCENTS", "RANDOM", "SHIFT <", "SHIFT >", "REVERSE", "CLEAR", "COPY", "PASTE" };
        const char* tips[numSeqTools] { "Set STEPS to one bar of the chosen meter.",
                                        "Set STEPS to two bars of the chosen meter.",
                                        "Set the velocities from the meter: the first step of every group is loud.",
                                        "Fill the pattern with random notes from the scale on the SYNTH page.",
                                        "Move the pattern one step to the left.",
                                        "Move the pattern one step to the right.",
                                        "Play the pattern backwards.",
                                        "Put every step back to the key, without slides.",
                                        "Remember the pattern that is shown.",
                                        "Put the remembered pattern into the one that is shown, for example to make B a variation of A." };
        for (int i = 0; i < numSeqTools; ++i)
        {
            auto& button = seqButtons[(size_t) i];
            button.setButtonText (names[i]);
            button.setTooltip (tips[i]);
            button.setColour (juce::TextButton::buttonColourId, colour::knob);
            button.setColour (juce::TextButton::textColourOffId, colour::text);
            button.onClick = [this, i]
            {
                switch (i)
                {
                    case seqOneBar:     stepEditor.fitToMeter (1); break;
                    case seqTwoBars:    stepEditor.fitToMeter (2); break;
                    case seqAccents:    stepEditor.accentsFromMeter(); break;
                    case seqRandom:     stepEditor.randomise(); break;
                    case seqShiftLeft:  stepEditor.shift (-1); break;
                    case seqShiftRight: stepEditor.shift (1); break;
                    case seqReverse:    stepEditor.reverse(); break;
                    case seqClear:      stepEditor.clear(); break;
                    case seqCopy:       stepEditor.copy(); break;
                    case seqPaste:      stepEditor.paste(); break;
                    default: break;
                }
            };
            addChildComponent (button);
        }

        auto& tools = addSection ("TOOLS", true, 3, 3.6f);
        for (auto& button : seqButtons)
            tools.column.push_back (&button);
        tools.columnsInColumn = 4;
    }

    // MOD page: four LFOs with a shape editor, a free envelope, the modulation matrix and the macros.
    buildingPage = 2;
    {
        lfoSection = sections.size();
        auto& s = addSection ("LFO", false, 0, 6.2f);
        s.columnWidth = 92;
        s.columnsInColumn = 2;
        s.display = &lfoDisplay;
        s.displayShare = 0.8f;
        lfoDisplay.owner = this;
        addChildComponent (lfoDisplay);

        for (int i = 0; i < mod::numLfos; ++i)
        {
            auto& button = lfoSelect[(size_t) i];
            button.setButtonText ("LFO " + juce::String (i + 1));
            button.setTooltip ("Show the settings of this LFO.");
            button.setColour (juce::TextButton::buttonColourId, colour::knob);
            button.setColour (juce::TextButton::buttonOnColourId, colour::forSource (mod::lfoSource (i)).withAlpha (0.55f));
            button.setColour (juce::TextButton::textColourOffId, colour::dimText);
            button.setColour (juce::TextButton::textColourOnId, colour::text);
            button.onClick = [this, i] { showLfo (i); };
            addChildComponent (button);

            auto& column = lfoColumns[(size_t) i];
            for (auto& select : lfoSelect)
                column.push_back (&select);
            column.push_back (&makeChoice (id::lfo (i, "Shape"), false)->box);
            column.push_back (&makeChoice (id::lfo (i, "Mode"), false)->box);
            column.push_back (&makeToggle (id::lfo (i, "Sync"), "SYNC", false)->button);
            column.push_back (&makeChoice (id::lfo (i, "Division"), false)->box);
            lfoKnobs[(size_t) i] = makeKnobs (false, { { id::lfo (i, "Rate"), "RATE" } });
        }
        showLfo (0);
    }
    {
        auto& s = addSection ("ENV 3", false, 0, 3.8f);
        env3Display.setParameters (processor.apvts, id::env3A, id::env3D, id::env3S, id::env3R);
        s.display = &env3Display;
        s.displayShare = 0.4f;
        s.knobs = makeKnobs (false, { { id::env3A, "A" }, { id::env3D, "D" }, { id::env3S, "S" }, { id::env3R, "R" } });
        addChildComponent (env3Display);
    }
    addSection ("MACROS", false, 3, 4.0f).knobs = makeKnobs (false, { { id::macro (0), "MACRO 1" }, { id::macro (1), "MACRO 2" },
                                                                      { id::macro (2), "MACRO 3" }, { id::macro (3), "MACRO 4" } });
    lfoHelp.lines = { "Set an LFO to Custom to draw it: drag points, double-click to add or remove one.",
                      "The small handle between two points bends the line. Hold Alt to move without the grid.",
                      "Right-click the LFO for shapes to start from, such as stairs or a sidechain pump.",
                      "Retrigger starts the LFO with every note; One Shot runs it once, like an envelope." };
    addSection ("LFO EDITOR", false, 3, 6.0f).display = &lfoHelp;
    addChildComponent (lfoHelp);
    for (int i = 0; i < mod::numSlots; ++i)
    {
        auto& s = addSection ("MOD " + juce::String (i + 1), false, 1 + i / 4, 1.0f);
        s.columnWidth = 150;
        slotParams[(size_t) i] = { processor.apvts.getParameter (id::slot (i, "Source")), processor.apvts.getParameter (id::slot (i, "Dest")),
                                   processor.apvts.getParameter (id::slot (i, "Amount")), processor.apvts.getParameter (id::slot (i, "Polarity")) };
        s.column.push_back (&makeChoice (id::slot (i, "Source"), false)->box);
        s.column.push_back (&makeChoice (id::slot (i, "Dest"), false)->box);
        s.column.push_back (&makeChoice (id::slot (i, "Polarity"), false)->box);
        s.knobs = makeKnobs (false, { { id::slot (i, "Amount"), "AMOUNT" } });
    }

    // FX page, from the top in the order the sound runs through it.
    buildingPage = 3;
    {
        auto& s = addSection ("COMPRESSOR", false, 0, 6.0f);
        s.column.push_back (&makeToggle (id::compOn, "ON", false)->button);
        s.columnWidth = 80;
        s.knobs = makeKnobs (false, { { id::compThreshold, "THRESHOLD" }, { id::compRatio, "RATIO" }, { id::compAttack, "ATTACK" },
                                      { id::compRelease, "RELEASE" }, { id::compMakeup, "MAKEUP" } });
    }
    {
        auto& s = addSection ("BITCRUSHER", false, 0, 4.2f);
        s.column.push_back (&makeToggle (id::crushOn, "ON", false)->button);
        s.columnWidth = 80;
        s.knobs = makeKnobs (false, { { id::crushBits, "BITS" }, { id::crushRate, "RATE" }, { id::crushMix, "MIX" } });
    }
    {
        auto& s = addSection ("DISTORTION", false, 1, 4.0f);
        s.column.push_back (&makeToggle (id::distOn, "ON", false)->button);
        s.column.push_back (&makeChoice (id::distType, false)->box);
        s.knobs = makeKnobs (false, { { id::distDrive, "DRIVE" }, { id::distMix, "MIX" } });
    }
    {
        auto& s = addSection ("TRANCE GATE", false, 1, 6.0f);
        s.column.push_back (&makeToggle (id::gateOn, "ON", false)->button);
        s.column.push_back (&makeChoice (id::gateRate, false)->box);
        s.column.push_back (&makeChoice (id::gatePattern, false)->box);
        s.columnsInColumn = 2;
        s.columnWidth = 118;
        s.knobs = makeKnobs (false, { { id::gateDepth, "DEPTH" }, { id::gateLength, "LENGTH" }, { id::gateSmooth, "SMOOTH" } });
    }
    {
        auto& s = addSection ("CHORUS", false, 2, 3.6f);
        s.column.push_back (&makeToggle (id::chorusOn, "ON", false)->button);
        s.columnWidth = 70;
        s.knobs = makeKnobs (false, { { id::chorusRate, "RATE" }, { id::chorusDepth, "DEPTH" }, { id::chorusMix, "MIX" } });
    }
    {
        auto& s = addSection ("FLANGER", false, 2, 4.2f);
        s.column.push_back (&makeToggle (id::flangerOn, "ON", false)->button);
        s.columnWidth = 70;
        s.knobs = makeKnobs (false, { { id::flangerRate, "RATE" }, { id::flangerDepth, "DEPTH" }, { id::flangerFeedback, "FEEDBACK" }, { id::flangerMix, "MIX" } });
    }
    {
        auto& s = addSection ("PHASER", false, 2, 4.2f);
        s.column.push_back (&makeToggle (id::phaserOn, "ON", false)->button);
        s.columnWidth = 70;
        s.knobs = makeKnobs (false, { { id::phaserRate, "RATE" }, { id::phaserDepth, "DEPTH" }, { id::phaserFeedback, "FEEDBACK" }, { id::phaserMix, "MIX" } });
    }
    {
        auto& s = addSection ("DELAY", false, 3, 6.0f);
        s.column.push_back (&makeToggle (id::delayOn, "ON", false)->button);
        s.column.push_back (&makeChoice (id::delayTime, false)->box);
        s.column.push_back (&makeToggle (id::delayPingPong, "PING-PONG", false)->button);
        s.columnsInColumn = 2;
        s.columnWidth = 104;
        s.knobs = makeKnobs (false, { { id::delayFeedback, "FEEDBACK" }, { id::delayTone, "TONE" }, { id::delayMix, "MIX" } });
    }
    {
        auto& s = addSection ("REVERB", false, 3, 5.0f);
        s.column.push_back (&makeToggle (id::reverbOn, "ON", false)->button);
        s.knobs = makeKnobs (false, { { id::reverbSize, "SIZE" }, { id::reverbDamp, "DAMP" }, { id::reverbMix, "MIX" } });
    }
    {
        auto& s = addSection ("EQ", false, 4, 5.0f);
        s.column.push_back (&makeToggle (id::eqOn, "ON", false)->button);
        s.columnWidth = 70;
        s.knobs = makeKnobs (false, { { id::eqLow, "LOW" }, { id::eqMid, "MID" }, { id::eqMidFreq, "MID FREQ" }, { id::eqHigh, "HIGH" } });
    }
    {
        auto& s = addSection ("KICK DUCK", false, 4, 3.4f);
        s.column.push_back (&makeToggle (id::duckOn, "ON", false)->button);
        s.columnWidth = 70;
        s.knobs = makeKnobs (false, { { id::duckDepth, "DEPTH" }, { id::duckRelease, "RELEASE" } });
    }
    {
        auto& s = addSection ("ROUTING", false, 4, 3.4f);
        s.column.push_back (&makeChoice (id::fxOrder, false)->box);
    }
    {
        auto& s = addSection ("OUTPUT", false, 4, 3.4f);
        s.display = &levelMeter;
        s.knobs = makeKnobs (false, { { id::level, "LEVEL" } });
        addChildComponent (levelMeter);
    }
    buildingPage = 0;

    sourceBar.owner = this;
    addAndMakeVisible (sourceBar);
    statusLabel.setFont (juce::FontOptions (11.5f));
    statusLabel.setColour (juce::Label::textColourId, colour::text);
    statusLabel.setJustificationType (juce::Justification::centredRight);
    statusLabel.setInterceptsMouseClicks (false, false);
    addAndMakeVisible (statusLabel);

    addChildComponent (waveEditor);

    const char* tabNames[] { "SYNTH", "OSC", "WAVE", "SAMPLE", "SEQ", "MOD", "FX", "SETUP" };
    for (size_t i = 0; i < tabs.size(); ++i)
    {
        auto& tab = tabs[i];
        tab.setButtonText (tabNames[i]);
        tab.setClickingTogglesState (false);
        tab.setColour (juce::TextButton::buttonColourId, colour::header);
        tab.setColour (juce::TextButton::buttonOnColourId, colour::track);
        tab.setColour (juce::TextButton::textColourOffId, colour::dimText);
        tab.setColour (juce::TextButton::textColourOnId, colour::text);
        tab.onClick = [this, i] { setPage (tabPages[i]); };
        addAndMakeVisible (tab);
    }
    tabs[0].setToggleState (true, juce::dontSendNotification);

    // Preset bar in the header.
    for (int i = 0; i < processor.getNumPrograms(); ++i)
        presetBox.addItem (processor.getProgramName (i), i + 1);
    presetBox.setTextWhenNothingSelected ("User preset");
    presetBox.onChange = [this]
    {
        const int index = presetBox.getSelectedId() - 1;
        if (index >= 0 && index != shownPreset)
            selectPreset (index);
    };
    prevPreset.onClick = [this] { selectPreset ((juce::jmax (0, shownPreset) + processor.getNumPrograms() - 1) % processor.getNumPrograms()); };
    nextPreset.onClick = [this] { selectPreset ((shownPreset + 1) % processor.getNumPrograms()); };

    const auto presetFolder = PresetBrowser::userFolder();
    savePreset.onClick = [this, presetFolder]
    {
        presetFolder.createDirectory();
        fileChooser = std::make_unique<juce::FileChooser> ("Save preset", presetFolder, "*.amvpreset");
        fileChooser->launchAsync (juce::FileBrowserComponent::saveMode | juce::FileBrowserComponent::canSelectFiles | juce::FileBrowserComponent::warnAboutOverwriting,
                                  [this] (const juce::FileChooser& chooser)
                                  {
                                      const auto file = chooser.getResult();
                                      if (file != juce::File() && processor.savePresetFile (file.withFileExtension ("amvpreset")))
                                      {
                                          // The saved preset is the one that is loaded now.
                                          userPreset = file.withFileExtension ("amvpreset");
                                          shownPreset = -1;
                                          presetBox.setSelectedId (0, juce::dontSendNotification);
                                          presetBox.setTextWhenNothingSelected (userPreset.getFileNameWithoutExtension());
                                          browser.rescan();
                                      }
                                  });
    };
    browsePreset.setTooltip ("Open the preset browser: categories, search, favourites and your own presets.");
    browsePreset.onClick = [this] { showBrowser (! browser.isVisible()); };
    browser.onClose = [this] { showBrowser (false); };
    browser.onChoose = [this] (const PresetBrowser::Entry& entry)
    {
        if (entry.program >= 0)
            selectPreset (entry.program);
        else
            loadUserPreset (entry.file);
    };
    browser.onLoadFile = [this, presetFolder]
    {
        fileChooser = std::make_unique<juce::FileChooser> ("Load preset", presetFolder, "*.amvpreset");
        fileChooser->launchAsync (juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles,
                                  [this] (const juce::FileChooser& chooser) { loadUserPreset (chooser.getResult()); });
    };

    presetBox.setColour (juce::ComboBox::outlineColourId, colour::panelBorder);
    addAndMakeVisible (presetBox);
    for (auto* button : { &prevPreset, &nextPreset, &savePreset, &browsePreset })
    {
        button->setColour (juce::TextButton::buttonColourId, colour::header);
        button->setColour (juce::TextButton::textColourOffId, colour::dimText);
        addAndMakeVisible (*button);
    }
    shownPreset = processor.getCurrentProgram();
    presetBox.setSelectedId (shownPreset + 1, juce::dontSendNotification);

    keyboard.setAvailableRange (36, 96);
    keyboard.setColour (juce::MidiKeyboardComponent::keyDownOverlayColourId, colour::red);
    keyboard.setColour (juce::MidiKeyboardComponent::mouseOverKeyOverlayColourId, colour::red.withAlpha (0.4f));
    addAndMakeVisible (keyboard);

    browsePreset.setColour (juce::TextButton::buttonOnColourId, colour::track);
    browsePreset.setColour (juce::TextButton::textColourOnId, colour::text);
    addChildComponent (browser);

    showModel (juce::roundToInt (processor.apvts.getRawParameterValue (id::model)->load()));
    updateVisibility();
    startTimerHz (30);

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
        knob->slider.owner = this;
        knob->slider.param = processor.apvts.getParameter (paramId);
        knob->slider.dest = amv::modDestFor (paramId);

        addChildComponent (knob->slider);
        addChildComponent (knob->label);
        result.push_back (knob.get());
        knobs.push_back (std::move (knob));
    }

    return result;
}

AMVSynthAudioProcessorEditor::Toggle* AMVSynthAudioProcessorEditor::makeToggle (const juce::String& paramId, const juce::String& text, bool balkan)
{
    auto toggle = std::make_unique<Toggle>();
    toggle->button.setButtonText (text);
    toggle->button.setColour (juce::ToggleButton::tickColourId, balkan ? amv::colour::red : amv::colour::teal);
    toggle->attachment = std::make_unique<APVTS::ButtonAttachment> (processor.apvts, paramId, toggle->button);
    addChildComponent (toggle->button);
    toggles.push_back (std::move (toggle));
    return toggles.back().get();
}

AMVSynthAudioProcessorEditor::Choice* AMVSynthAudioProcessorEditor::makeChoice (const juce::String& paramId, bool balkan)
{
    auto choice = std::make_unique<Choice>();

    if (auto* param = dynamic_cast<juce::AudioParameterChoice*> (processor.apvts.getParameter (paramId)))
        choice->box.addItemList (param->choices, 1);

    choice->box.setColour (juce::ComboBox::outlineColourId, balkan ? amv::colour::balkanBorder : amv::colour::panelBorder);
    choice->attachment = std::make_unique<APVTS::ComboBoxAttachment> (processor.apvts, paramId, choice->box);
    addChildComponent (choice->box);
    choices.push_back (std::move (choice));
    return choices.back().get();
}

AMVSynthAudioProcessorEditor::Section& AMVSynthAudioProcessorEditor::addSection (const juce::String& title, bool balkan, int row, float weight)
{
    Section section;
    section.title = title;
    section.balkan = balkan;
    section.page = buildingPage;
    section.row = row;
    section.weight = weight;
    sections.push_back (std::move (section));
    return sections.back();
}

void AMVSynthAudioProcessorEditor::loadUserPreset (const juce::File& file)
{
    if (! file.existsAsFile())
        return;

    if (! processor.loadPresetFile (file))
    {
        showStatus ("This file is not an AMVSynth preset.");
        return;
    }

    userPreset = file;
    shownPreset = -1;
    presetBox.setSelectedId (0, juce::dontSendNotification);
    presetBox.setTextWhenNothingSelected (file.getFileNameWithoutExtension());
    browser.setCurrent (-1, userPreset);
}

void AMVSynthAudioProcessorEditor::showBrowser (bool show)
{
    browsePreset.setToggleState (show, juce::dontSendNotification);
    browser.setCurrent (shownPreset, userPreset);
    browser.setVisible (show);
    if (show)
        browser.toFront (true);
}

void AMVSynthAudioProcessorEditor::selectPreset (int index)
{
    userPreset = juce::File();
    shownPreset = index;
    processor.setCurrentProgram (index);
    presetBox.setSelectedId (index + 1, juce::dontSendNotification);
}

void AMVSynthAudioProcessorEditor::showModel (int index)
{
    index = juce::jlimit (0, (int) modelKnobs.size() - 1, index);
    if (index == shownModel)
        return;

    shownModel = index;
    sections[engineSection].knobs = modelKnobs[(size_t) index];
    layoutSection (sections[engineSection]);
    updateVisibility();
}

void AMVSynthAudioProcessorEditor::setPage (int page)
{
    currentPage = page;
    showBrowser (false);
    for (size_t i = 0; i < tabs.size(); ++i)
        tabs[i].setToggleState (tabPages[i] == page, juce::dontSendNotification);
    resized();
    updateVisibility();
    repaint();
}

void AMVSynthAudioProcessorEditor::updateVisibility()
{
    for (auto& k : knobs)
    {
        k->slider.setVisible (false);
        k->label.setVisible (false);
    }
    for (auto& t : toggles)
        t->button.setVisible (false);
    for (auto& c : choices)
        c->box.setVisible (false);
    tuneBox.setVisible (false);
    sourceBar.setVisible (hasSourceBar());
    statusLabel.setVisible (hasSourceBar());
    waveEditor.setVisible (currentPage == 5);
    for (auto& button : importButtons)
        button.setVisible (false);
    for (auto& button : sampleButtons)
        button.setVisible (false);
    for (auto& button : lfoSelect)
        button.setVisible (false);
    for (auto& button : seqButtons)
        button.setVisible (false);
    for (auto& s : sections)
        if (s.display != nullptr)
            s.display->setVisible (false);

    for (auto& s : sections)
    {
        if (s.page != currentPage)
            continue;

        for (auto* k : s.knobs)
        {
            k->slider.setVisible (true);
            k->label.setVisible (true);
        }
        for (auto* c : s.column)
            c->setVisible (true);
        if (s.display != nullptr)
            s.display->setVisible (true);
    }
}

void AMVSynthAudioProcessorEditor::timerCallback()
{
    // The host can change the program too.
    if (shownPreset >= 0 && processor.getCurrentProgram() != shownPreset)
    {
        shownPreset = processor.getCurrentProgram();
        presetBox.setSelectedId (shownPreset + 1, juce::dontSendNotification);
        userPreset = juce::File();
    }

    if (browser.isVisible())
        browser.setCurrent (shownPreset, userPreset);

    // The model can also change from host automation or a loaded preset.
    showModel (juce::roundToInt (processor.apvts.getRawParameterValue (amv::id::model)->load()));

    const int meter = juce::roundToInt (processor.apvts.getRawParameterValue (amv::id::arpMeter)->load());
    if (meter != patternDisplay.meter)
    {
        patternDisplay.meter = meter;
        patternDisplay.repaint();
    }

    // Output meter: jump up to new peaks, fall back slowly.
    bool meterChanged = false;
    for (size_t ch = 0; ch < 2; ++ch)
    {
        const float peak = processor.outputPeak[ch].exchange (0.0f, std::memory_order_relaxed);
        const float next = juce::jmax (peak, levelMeter.level[ch] * 0.86f);
        meterChanged = meterChanged || std::abs (next - levelMeter.level[ch]) > 1.0e-4f;
        levelMeter.level[ch] = next < 1.0e-4f ? 0.0f : next;

        if (peak >= 0.99f)
        {
            levelMeter.clipHold = 30; // one second at the timer rate
            meterChanged = true;
        }
    }
    if (levelMeter.clipHold > 0 && --levelMeter.clipHold == 0)
        meterChanged = true;
    ampEnvDisplay.refresh();
    filterEnvDisplay.refresh();
    updateModulationDisplay();
    sampleDisplay.refresh();
    stepEditor.refresh();
    patternBar.refresh();

    // A preset or a loaded project can rename the macros.
    for (size_t i = 0; i < macroLabels.size(); ++i)
        if (macroLabels[i] != nullptr && ! macroLabels[i]->isBeingEdited() && macroLabels[i]->getText() != processor.getMacroName ((int) i))
            macroLabels[i]->setText (processor.getMacroName ((int) i), juce::dontSendNotification);
    updateSampleInfo();

    if (meterChanged && levelMeter.isVisible())
        levelMeter.repaint();

    const auto param = [this] (const char* paramId) { return processor.apvts.getRawParameterValue (paramId)->load(); };
    const auto sync = [] (WaveDisplay& d, int table, float position, float level, const amv::UserWavetable* user)
    {
        if (table != d.table || std::abs (position - d.position) > 1.0e-4f || (level > 0.0f) != d.active || user != d.user)
        {
            d.user = user;
            d.table = table;
            d.position = position;
            d.active = level > 0.0f;
            d.repaint();
        }
    };
    sync (waveA, juce::roundToInt (param (amv::id::oscATable)), param (amv::id::oscAPos), param (amv::id::oscALevel), processor.getUserWavetable (0));
    sync (waveB, juce::roundToInt (param (amv::id::oscBTable)), param (amv::id::oscBPos), param (amv::id::oscBLevel), processor.getUserWavetable (1));
}

void AMVSynthAudioProcessorEditor::LevelMeter::paint (juce::Graphics& g)
{
    using namespace amv;
    auto area = getLocalBounds().toFloat();
    g.setColour (colour::background);
    g.fillRoundedRectangle (area, 6.0f);
    area.reduce (10.0f, 10.0f);

    // The clip light on the right, then one bar per channel from -48 dB to 0 dB.
    auto light = area.removeFromRight (10.0f);
    area.removeFromRight (6.0f);
    g.setColour (clipHold > 0 ? colour::red : colour::track);
    g.fillRoundedRectangle (light, 3.0f);

    const float barHeight = juce::jmin (10.0f, (area.getHeight() - 6.0f) * 0.5f);
    auto bars = area.withSizeKeepingCentre (area.getWidth(), barHeight * 2.0f + 6.0f);

    for (size_t ch = 0; ch < 2; ++ch)
    {
        auto bar = bars.removeFromTop (barHeight);
        bars.removeFromTop (6.0f);
        g.setColour (colour::track);
        g.fillRoundedRectangle (bar, 3.0f);

        const float db = juce::Decibels::gainToDecibels (level[ch], -48.0f);
        const float fraction = juce::jlimit (0.0f, 1.0f, (db + 48.0f) / 48.0f);
        if (fraction > 0.0f)
        {
            g.setColour (db > -3.0f ? colour::red : colour::teal);
            g.fillRoundedRectangle (bar.withWidth (juce::jmax (6.0f, bar.getWidth() * fraction)), 3.0f);
        }
    }
}

void AMVSynthAudioProcessorEditor::WaveDisplay::paint (juce::Graphics& g)
{
    using namespace amv;
    auto area = getLocalBounds().toFloat();
    g.setColour (colour::background);
    g.fillRoundedRectangle (area, 6.0f);
    area.reduce (10.0f, 8.0f);

    // All frames of the table in depth, with the shape that is playing drawn bright.
    const auto& bank = WavetableBank::get();
    const bool isUser = table >= WavetableBank::numTables;
    const auto frameAt = [&] (int f) -> const float*
    {
        if (isUser)
            return user != nullptr ? user->wave (f, 2) : bank.wave (0, 0, 2); // a sine sounds when nothing was made yet
        return bank.wave (juce::jmax (0, table), f, 2);
    };

    // An oscillator that is turned down is drawn dimmed.
    drawWavetableStack (g, area, frameAt, WavetableBank::numFrames, position, colour::teal.withAlpha (active ? 1.0f : 0.45f));

    if (isUser)
    {
        g.setColour (colour::dimText);
        g.setFont (juce::FontOptions (10.5f));
        g.drawText (user != nullptr ? user->getName() : juce::String ("Empty: press EDIT to make a table"), area.withHeight (12.0f),
                    juce::Justification::topLeft);
    }
}

void AMVSynthAudioProcessorEditor::EnvelopeDisplay::setParameters (APVTS& apvts, const char* attack, const char* decay,
                                                                   const char* sustain, const char* release)
{
    params = { apvts.getParameter (attack), apvts.getParameter (decay), apvts.getParameter (sustain), apvts.getParameter (release) };
}

juce::Rectangle<float> AMVSynthAudioProcessorEditor::EnvelopeDisplay::graphArea() const
{
    // The bottom strip holds the values as text, when there is room for them.
    return getLocalBounds().toFloat().withTrimmedBottom (showsValues() ? 18.0f : 0.0f).reduced (9.0f, 8.0f);
}

float AMVSynthAudioProcessorEditor::EnvelopeDisplay::segmentWidth() const
{
    // Attack, decay and release each get up to this much room; the held part has a fixed width.
    return graphArea().getWidth() * 0.29f;
}

std::array<juce::Point<float>, 5> AMVSynthAudioProcessorEditor::EnvelopeDisplay::points() const
{
    const auto area = graphArea();
    const float seg = segmentWidth(), hold = area.getWidth() - 3.0f * seg;
    const float a = params[0]->getValue(), d = params[1]->getValue(), s = params[2]->getValue(), r = params[3]->getValue();
    const float sustainY = area.getBottom() - s * area.getHeight();

    std::array<juce::Point<float>, 5> p;
    p[0] = { area.getX(), area.getBottom() };
    p[1] = { p[0].x + a * seg, area.getY() };
    p[2] = { p[1].x + d * seg, sustainY };
    p[3] = { p[2].x + hold, sustainY };
    p[4] = { p[3].x + r * seg, area.getBottom() };
    return p;
}

int AMVSynthAudioProcessorEditor::EnvelopeDisplay::handleAt (juce::Point<float> position) const
{
    const auto p = points();
    const juce::Point<float> handles[3] { p[1], p[2], p[4] };
    int best = none;
    float bestDistance = 14.0f;

    for (int i = 0; i < 3; ++i)
    {
        const float distance = handles[i].getDistanceFrom (position);
        if (distance < bestDistance)
        {
            best = i;
            bestDistance = distance;
        }
    }

    return best;
}

void AMVSynthAudioProcessorEditor::EnvelopeDisplay::set (juce::RangedAudioParameter* param, float normalised)
{
    param->setValueNotifyingHost (juce::jlimit (0.0f, 1.0f, normalised));
}

void AMVSynthAudioProcessorEditor::EnvelopeDisplay::refresh()
{
    bool changed = false;
    for (size_t i = 0; i < params.size(); ++i)
    {
        const float v = params[i]->getValue();
        changed = changed || std::abs (v - shown[i]) > 1.0e-6f;
        shown[i] = v;
    }

    if (changed)
        repaint();
}

void AMVSynthAudioProcessorEditor::EnvelopeDisplay::paint (juce::Graphics& g)
{
    using namespace amv;
    const auto bounds = getLocalBounds().toFloat();
    g.setColour (colour::background);
    g.fillRoundedRectangle (bounds, 6.0f);

    const auto area = graphArea();
    const auto p = points();

    g.setColour (colour::track.withAlpha (0.6f));
    for (int i = 1; i < 4; ++i)
        g.drawHorizontalLine (juce::roundToInt (area.getY() + area.getHeight() * (float) i / 4.0f), area.getX(), area.getRight());

    juce::Path curve;
    curve.startNewSubPath (p[0]);
    for (size_t i = 1; i < p.size(); ++i)
        curve.lineTo (p[i]);

    juce::Path fill (curve);
    fill.lineTo (p[0]);
    fill.closeSubPath();
    g.setGradientFill (juce::ColourGradient (colour::teal.withAlpha (0.32f), 0.0f, area.getY(),
                                             colour::teal.withAlpha (0.03f), 0.0f, area.getBottom(), false));
    g.fillPath (fill);

    g.setColour (colour::teal);
    g.strokePath (curve, juce::PathStrokeType (1.8f, juce::PathStrokeType::mitered, juce::PathStrokeType::rounded));

    // The held part is not a setting of its own, so it has no handle.
    const juce::Point<float> handles[3] { p[1], p[2], p[4] };
    for (int i = 0; i < 3; ++i)
    {
        const bool active = i == hover || i == dragging;
        const float radius = active ? 5.5f : 4.0f;
        g.setColour (colour::background);
        g.fillEllipse (handles[i].x - radius, handles[i].y - radius, radius * 2.0f, radius * 2.0f);
        g.setColour (active ? colour::text : colour::teal);
        g.drawEllipse (handles[i].x - radius, handles[i].y - radius, radius * 2.0f, radius * 2.0f, 1.8f);
    }

    if (! showsValues())
        return;

    // Values along the bottom; the one being changed is highlighted.
    static const char* names[4] { "A", "D", "S", "R" };
    auto textArea = bounds.withTop (bounds.getBottom() - 19.0f).reduced (6.0f, 0.0f);
    const float cell = textArea.getWidth() / 4.0f;
    const int active = dragging != none ? dragging : hover;
    g.setFont (juce::FontOptions (10.5f));

    for (int i = 0; i < 4; ++i)
    {
        const bool lit = (active == attackHandle && i == 0) || (active == decayHandle && (i == 1 || i == 2))
                         || (active == releaseHandle && i == 3);
        juce::String value;
        if (i == 2)
            value = juce::String (juce::roundToInt (params[2]->getValue() * 100.0f)) + "%";
        else
        {
            const float ms = params[(size_t) i]->convertFrom0to1 (params[(size_t) i]->getValue());
            value = ms < 10.0f ? juce::String (ms, 1) + " ms" : ms < 1000.0f ? juce::String (juce::roundToInt (ms)) + " ms"
                                                                           : juce::String (ms * 0.001f, 2) + " s";
        }

        g.setColour (lit ? colour::text : colour::dimText);
        g.drawText (juce::String (names[i]) + " " + value, textArea.removeFromLeft (cell), juce::Justification::centred);
    }
}

void AMVSynthAudioProcessorEditor::EnvelopeDisplay::mouseMove (const juce::MouseEvent& e)
{
    const int handle = handleAt (e.position);
    if (handle != hover)
    {
        hover = handle;
        setMouseCursor (handle == none ? juce::MouseCursor::NormalCursor : juce::MouseCursor::DraggingHandCursor);
        repaint();
    }
}

void AMVSynthAudioProcessorEditor::EnvelopeDisplay::mouseExit (const juce::MouseEvent&)
{
    if (hover != none)
    {
        hover = none;
        repaint();
    }
}

void AMVSynthAudioProcessorEditor::EnvelopeDisplay::mouseDown (const juce::MouseEvent& e)
{
    dragging = handleAt (e.position);

    if (dragging == attackHandle)
        params[0]->beginChangeGesture();
    else if (dragging == decayHandle)
    {
        params[1]->beginChangeGesture();
        params[2]->beginChangeGesture();
    }
    else if (dragging == releaseHandle)
        params[3]->beginChangeGesture();

    repaint();
}

void AMVSynthAudioProcessorEditor::EnvelopeDisplay::mouseDrag (const juce::MouseEvent& e)
{
    if (dragging == none)
        return;

    const auto area = graphArea();
    const auto p = points();
    const float seg = segmentWidth();

    // Each handle moves relative to the point before it, like the curve itself.
    if (dragging == attackHandle)
        set (params[0], (e.position.x - p[0].x) / seg);
    else if (dragging == decayHandle)
    {
        set (params[1], (e.position.x - p[1].x) / seg);
        set (params[2], (area.getBottom() - e.position.y) / area.getHeight());
    }
    else
        set (params[3], (e.position.x - p[3].x) / seg);

    refresh();
}

void AMVSynthAudioProcessorEditor::EnvelopeDisplay::mouseUp (const juce::MouseEvent&)
{
    if (dragging == attackHandle)
        params[0]->endChangeGesture();
    else if (dragging == decayHandle)
    {
        params[1]->endChangeGesture();
        params[2]->endChangeGesture();
    }
    else if (dragging == releaseHandle)
        params[3]->endChangeGesture();

    dragging = none;
    repaint();
}

void AMVSynthAudioProcessorEditor::EnvelopeDisplay::mouseDoubleClick (const juce::MouseEvent& e)
{
    const int handle = handleAt (e.position);
    const auto reset = [] (juce::RangedAudioParameter* param)
    {
        param->beginChangeGesture();
        param->setValueNotifyingHost (param->getDefaultValue());
        param->endChangeGesture();
    };

    if (handle == attackHandle)
        reset (params[0]);
    else if (handle == decayHandle)
    {
        reset (params[1]);
        reset (params[2]);
    }
    else if (handle == releaseHandle)
        reset (params[3]);

    refresh();
}

//==============================================================================
// Modulation by drag and drop

namespace
{
const char* const sourceTag = "mod:";
const char* const chipNames[amv::mod::numSources] { "", "LFO 1", "LFO 2", "F.ENV", "VEL", "WHEEL", "M1", "M2", "M3", "M4", "PRESS", "TIMBRE", "LFO 3", "LFO 4", "ENV 3" };

/** The order of the chips in the source bar: modulators first, then what the player does. */
constexpr int chipOrder[amv::mod::numSources - 1] { amv::mod::srcLfo1, amv::mod::srcLfo2, amv::mod::srcLfo3, amv::mod::srcLfo4, amv::mod::srcFilterEnv,
                                                    amv::mod::srcEnv3, amv::mod::srcVelocity, amv::mod::srcModWheel, amv::mod::srcMacro1,
                                                    amv::mod::srcMacro2, amv::mod::srcMacro3, amv::mod::srcMacro4, amv::mod::srcPressure, amv::mod::srcTimbre };

/** LFOs and timbre swing both ways around the value of the knob; the others only add. */
bool isBipolar (int source)
{
    return amv::mod::isBipolarSource (source);
}

void setParameter (juce::RangedAudioParameter* param, float realValue)
{
    param->beginChangeGesture();
    param->setValueNotifyingHost (param->convertTo0to1 (realValue));
    param->endChangeGesture();
}

int realValue (const juce::RangedAudioParameter* param)
{
    return juce::roundToInt (param->convertFrom0to1 (param->getValue()));
}
} // namespace

float AMVSynthAudioProcessorEditor::ModKnob::proportionWith (float x) const
{
    const float base = param->getValue();
    const auto range = param->getNormalisableRange();

    // These two are not moved in a straight line, so follow what the engine does with them.
    if (dest == amv::mod::dstCutoff || dest == amv::mod::dstCutoff2)
        return param->convertTo0to1 (juce::jlimit (range.start, range.end, param->convertFrom0to1 (base) * std::exp2 (x * 5.0f)));

    if (dest == amv::mod::dstVolume)
    {
        const float gain = juce::jlimit (0.0f, 2.0f, 1.0f + x);
        const float db = gain < 1.0e-4f ? range.start : param->convertFrom0to1 (base) + juce::Decibels::gainToDecibels (gain);
        return param->convertTo0to1 (juce::jlimit (range.start, range.end, db));
    }

    // Envelope times, the delay tone and the crush rate move by octaves.
    if ((dest >= amv::mod::dstAmpAttack && dest <= amv::mod::dstFltRelease) || dest == amv::mod::dstDelayTone || dest == amv::mod::dstCrushRate)
        return param->convertTo0to1 (juce::jlimit (range.start, range.end, param->convertFrom0to1 (base) * std::exp2 (x * amv::mod::timeOctaves)));

    return juce::jlimit (0.0f, 1.0f, base + x);
}

void AMVSynthAudioProcessorEditor::ModKnob::paint (juce::Graphics& g)
{
    juce::Slider::paint (g);

    if (dest < 0 || param == nullptr)
        return;

    const auto area = getLocalBounds().toFloat().reduced (2.0f);
    const float size = juce::jmin (area.getWidth(), area.getHeight());
    const auto centre = area.getCentre();
    const float radius = size * 0.5f - 1.5f;
    const auto rotary = getRotaryParameters();
    const auto angleFor = [&] (float proportion)
    {
        return rotary.startAngleRadians + proportion * (rotary.endAngleRadians - rotary.startAngleRadians);
    };
    const auto arc = [&] (float from, float to)
    {
        juce::Path path;
        path.addCentredArc (centre.x, centre.y, radius, radius, 0.0f, angleFor (juce::jmin (from, to)), angleFor (juce::jmax (from, to)), true);
        return path;
    };

    // While a source is being dragged, every knob that can take it shows a ring.
    if (showAsTarget || dropHover)
    {
        g.setColour (dropHover ? amv::colour::text : amv::colour::text.withAlpha (0.22f));
        g.strokePath (arc (0.0f, 1.0f), juce::PathStrokeType (dropHover ? 2.2f : 1.4f));
    }

    if (assignments.empty())
        return;

    // One arc per modulation: how far that source can move the knob.
    for (const auto& a : assignments)
    {
        const float farEnd = proportionWith (a.amount);
        const float nearEnd = amv::mod::isBipolar (a.source, a.polarity) ? proportionWith (-a.amount) : proportionWith (0.0f);
        if (std::abs (farEnd - nearEnd) < 0.004f)
            continue;

        g.setColour (amv::colour::forSource (a.source));
        g.strokePath (arc (nearEnd, farEnd), juce::PathStrokeType (2.4f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    }

    // The dot is where the modulated value is right now.
    const float now = angleFor (proportionWith (live));
    const juce::Point<float> dot (centre.x + radius * std::sin (now), centre.y - radius * std::cos (now));
    g.setColour (amv::colour::background);
    g.fillEllipse (dot.x - 4.0f, dot.y - 4.0f, 8.0f, 8.0f);
    g.setColour (amv::colour::text);
    g.fillEllipse (dot.x - 2.6f, dot.y - 2.6f, 5.2f, 5.2f);
}

void AMVSynthAudioProcessorEditor::ModKnob::mouseDown (const juce::MouseEvent& e)
{
    if (dest >= 0 && owner != nullptr && e.mods.isPopupMenu())
    {
        owner->showModMenu (*this);
        return;
    }

    // Shift-drag sets the depth of the modulation that was added last.
    if (dest >= 0 && owner != nullptr && e.mods.isShiftDown() && ! assignments.empty())
    {
        depthDrag = true;
        depthStart = assignments.back().amount;
        owner->slotParams[(size_t) assignments.back().slot].amount->beginChangeGesture();
        return;
    }

    juce::Slider::mouseDown (e);
}

void AMVSynthAudioProcessorEditor::ModKnob::mouseDrag (const juce::MouseEvent& e)
{
    if (! depthDrag)
    {
        juce::Slider::mouseDrag (e);
        return;
    }

    if (! assignments.empty())
        owner->setSlotAmount (assignments.back().slot, depthStart - (float) e.getDistanceFromDragStartY() / 140.0f, false);
}

void AMVSynthAudioProcessorEditor::ModKnob::mouseUp (const juce::MouseEvent& e)
{
    if (! depthDrag)
    {
        juce::Slider::mouseUp (e);
        return;
    }

    depthDrag = false;
    if (! assignments.empty())
        owner->slotParams[(size_t) assignments.back().slot].amount->endChangeGesture();
}

bool AMVSynthAudioProcessorEditor::ModKnob::isInterestedInDragSource (const SourceDetails& details)
{
    return dest >= 0 && details.description.toString().startsWith (sourceTag);
}

void AMVSynthAudioProcessorEditor::ModKnob::itemDragEnter (const SourceDetails&)
{
    dropHover = true;
    repaint();
}

void AMVSynthAudioProcessorEditor::ModKnob::itemDragExit (const SourceDetails&)
{
    dropHover = false;
    repaint();
}

void AMVSynthAudioProcessorEditor::ModKnob::itemDropped (const SourceDetails& details)
{
    dropHover = false;
    if (owner != nullptr)
        owner->assignModulation (details.description.toString().fromFirstOccurrenceOf (sourceTag, false, false).getIntValue(), dest);
    repaint();
}

std::vector<AMVSynthAudioProcessorEditor::Assignment> AMVSynthAudioProcessorEditor::assignmentsFor (int dest) const
{
    std::vector<Assignment> result;
    for (int i = 0; i < amv::mod::numSlots; ++i)
    {
        const auto& slot = slotParams[(size_t) i];
        const int source = realValue (slot.source);
        if (source != amv::mod::srcNone && realValue (slot.dest) == dest)
            result.push_back ({ i, source, slot.amount->convertFrom0to1 (slot.amount->getValue()), realValue (slot.polarity) });
    }

    return result;
}

void AMVSynthAudioProcessorEditor::assignModulation (int source, int dest)
{
    if (source <= amv::mod::srcNone || source >= amv::mod::numSources || dest < 0 || dest >= amv::mod::numDests)
        return;

    const juce::String route = juce::String (amv::getModSources()[(size_t) source]) + " > " + amv::getModDests()[(size_t) dest];

    for (const auto& a : assignmentsFor (dest))
        if (a.source == source)
        {
            showStatus (route + " is already set. Shift-drag the knob for depth.");
            return;
        }

    for (int i = 0; i < amv::mod::numSlots; ++i)
    {
        const auto& slot = slotParams[(size_t) i];
        if (realValue (slot.source) != amv::mod::srcNone)
            continue;

        setParameter (slot.dest, (float) dest);
        setParameter (slot.polarity, (float) amv::mod::polarityAuto);
        setParameter (slot.amount, 0.35f);
        setParameter (slot.source, (float) source);
        showStatus (route + "  (MOD " + juce::String (i + 1) + ")");
        return;
    }

    showStatus ("All " + juce::String (amv::mod::numSlots) + " modulation slots are in use.");
}

void AMVSynthAudioProcessorEditor::setSlotAmount (int slot, float amount, bool gesture)
{
    auto* param = slotParams[(size_t) slot].amount;
    if (gesture)
        param->beginChangeGesture();
    param->setValueNotifyingHost (param->convertTo0to1 (juce::jlimit (-1.0f, 1.0f, amount)));
    if (gesture)
        param->endChangeGesture();

    showStatus (juce::String (amv::getModSources()[(size_t) realValue (slotParams[(size_t) slot].source)]) + " depth "
                + juce::String (juce::roundToInt (juce::jlimit (-1.0f, 1.0f, amount) * 100.0f)) + "%");
}

void AMVSynthAudioProcessorEditor::clearSlot (int slot)
{
    setParameter (slotParams[(size_t) slot].source, (float) amv::mod::srcNone);
    setParameter (slotParams[(size_t) slot].amount, 0.0f);
}

void AMVSynthAudioProcessorEditor::showModMenu (ModKnob& knob)
{
    juce::PopupMenu menu, add;
    const auto current = assignmentsFor (knob.dest);

    for (int source = 1; source < amv::mod::numSources; ++source)
    {
        bool used = false;
        for (const auto& a : current)
            used = used || a.source == source;
        add.addItem (100 + source, amv::getModSources()[(size_t) source], ! used, used);
    }

    menu.addSectionHeader (juce::String ("Modulate ") + amv::getModDests()[(size_t) knob.dest]);
    menu.addSubMenu ("Add modulation from", add);

    for (const auto& a : current)
    {
        const juce::String name = amv::getModSources()[(size_t) a.source];
        menu.addSeparator();
        menu.addItem (300 + a.slot, "Invert " + name + "  (" + juce::String (juce::roundToInt (a.amount * 100.0f)) + "%)");
        const bool bipolar = amv::mod::isBipolar (a.source, a.polarity);
        menu.addItem (400 + a.slot, name + ": unipolar (one way)", true, ! bipolar);
        menu.addItem (500 + a.slot, name + ": bipolar (both ways)", true, bipolar);
        menu.addItem (200 + a.slot, "Remove " + name);
    }

    const int dest = knob.dest;
    menu.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (&knob),
                        [safe = juce::Component::SafePointer<AMVSynthAudioProcessorEditor> (this), dest] (int result)
                        {
                            if (safe == nullptr || result == 0)
                                return;

                            if (result >= 400)
                            {
                                const bool bipolar = result >= 500;
                                setParameter (safe->slotParams[(size_t) (result - (bipolar ? 500 : 400))].polarity,
                                              (float) (bipolar ? amv::mod::polarityBipolar : amv::mod::polarityUnipolar));
                            }
                            else if (result >= 300)
                            {
                                const auto* amount = safe->slotParams[(size_t) (result - 300)].amount;
                                safe->setSlotAmount (result - 300, -amount->convertFrom0to1 (amount->getValue()), true);
                            }
                            else if (result >= 200)
                                safe->clearSlot (result - 200);
                            else
                                safe->assignModulation (result - 100, dest);
                        });
}

void AMVSynthAudioProcessorEditor::setParameterValue (const char* paramId, float value)
{
    if (auto* param = processor.apvts.getParameter (paramId))
        param->setValueNotifyingHost (param->convertTo0to1 (value));
}

void AMVSynthAudioProcessorEditor::updateSampleInfo()
{
    const auto* data = processor.getSample();
    if (data == infoFor && infoShown)
        return;

    infoFor = data;
    infoShown = true;

    if (data == nullptr)
        sampleInfo.lines = { "No sample loaded. Press LOAD or drop an audio file on the wave.",
                             "Classic plays the sample across the keyboard, with a loop if you want one.",
                             "Granular builds the sound from short grains around POSITION.",
                             "SCAN moves the position on its own: 1 plays at the sample's speed at any pitch." };
    else
        sampleInfo.lines = { data->getName(),
                             juce::String ((double) data->getLength() / data->getSampleRate(), 2) + " s,  " + juce::String (juce::roundToInt (data->getSampleRate()))
                                 + " Hz,  " + (data->isStereo() ? "stereo" : "mono"),
                             "Drag START, END, LOOP and GRAIN in the wave. Turn LEVEL up to hear it.",
                             "The sample is saved inside the project and in presets you save." };
    sampleInfo.repaint();
}

void AMVSynthAudioProcessorEditor::runSampleTool (int tool)
{
    using namespace amv;
    const auto resetMarkers = [this]
    {
        setParameterValue (id::smpStart, 0.0f);
        setParameterValue (id::smpEnd, 1.0f);
        setParameterValue (id::smpLoopStart, 0.0f);
        setParameterValue (id::smpLoopEnd, 1.0f);
    };

    if (tool == toolLoad)
    {
        const auto folder = juce::File::getSpecialLocation (juce::File::userMusicDirectory);
        fileChooser = std::make_unique<juce::FileChooser> ("Load sample", folder, "*.wav;*.aif;*.aiff;*.flac;*.mp3;*.ogg");
        fileChooser->launchAsync (juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles,
                                  [this, resetMarkers] (const juce::FileChooser& chooser)
                                  {
                                      const auto file = chooser.getResult();
                                      if (file == juce::File())
                                          return;

                                      if (! processor.loadSample (file))
                                      {
                                          showStatus ("This file could not be read as audio.");
                                          return;
                                      }

                                      resetMarkers();
                                      if (processor.apvts.getRawParameterValue (id::smpLevel)->load() <= 0.0f)
                                          setParameterValue (id::smpLevel, 0.8f);
                                      showStatus ("Sample loaded.");
                                  });
        return;
    }

    const auto* data = processor.getSample();
    if (data == nullptr)
    {
        showStatus ("Load a sample first.");
        return;
    }

    const auto value = [this] (const char* paramId) { return processor.apvts.getRawParameterValue (paramId)->load(); };
    const float start = value (id::smpStart), end = juce::jmax (start, value (id::smpEnd));
    auto audio = data->toBuffer();
    const int length = audio.getNumSamples();

    switch (tool)
    {
        case toolNormalise:
        {
            const float peak = audio.getMagnitude (0, length);
            if (peak < 1.0e-6f)
                return;
            audio.applyGain (1.0f / peak);
            processor.setSample (SampleData::fromBuffer (audio, data->getSampleRate(), data->getName()));
            showStatus ("Normalized, gain " + juce::String (juce::Decibels::gainToDecibels (1.0f / peak), 1) + " dB.");
            break;
        }
        case toolCrop:
        {
            const int from = juce::jlimit (0, length - 16, (int) (start * (float) length));
            const int count = juce::jlimit (16, length - from, (int) ((end - start) * (float) length));
            juce::AudioBuffer<float> part (audio.getNumChannels(), count);
            for (int ch = 0; ch < part.getNumChannels(); ++ch)
                part.copyFrom (ch, 0, audio, ch, from, count);
            processor.setSample (SampleData::fromBuffer (part, data->getSampleRate(), data->getName()));
            resetMarkers();
            showStatus ("Cropped to " + juce::String ((double) count / data->getSampleRate(), 2) + " s.");
            break;
        }
        case toolFade:
        {
            const int fade = juce::jmin (length / 2, (int) (0.005 * data->getSampleRate()));
            audio.applyGainRamp (0, fade, 0.0f, 1.0f);
            audio.applyGainRamp (length - fade, fade, 1.0f, 0.0f);
            processor.setSample (SampleData::fromBuffer (audio, data->getSampleRate(), data->getName()));
            showStatus ("Edges faded.");
            break;
        }
        case toolFindRoot:
        {
            const int note = detectRootNote (*data);
            if (note < 0)
            {
                showStatus ("No clear pitch found; set ROOT by ear.");
                return;
            }
            setParameterValue (id::smpRoot, (float) note);
            showStatus ("Root set to " + juce::MidiMessage::getMidiNoteName (note, true, true, 3) + ".");
            break;
        }
        case toolToWaveA:
        case toolToWaveB:
        {
            const int osc = tool == toolToWaveA ? 0 : 1;
            processor.setUserWavetableFrames (osc, sampleToWavetable (*data, start, end, juce::roundToInt (value (id::smpRoot)),
                                                                      UserWavetable::numFrames, UserWavetable::tableSize),
                                              data->getName(), true);
            if (auto* param = dynamic_cast<juce::AudioParameterChoice*> (processor.apvts.getParameter (osc == 0 ? id::oscATable : id::oscBTable)))
                param->setValueNotifyingHost (param->convertTo0to1 ((float) (param->choices.size() - 1)));
            showStatus (juce::String ("Wavetable of oscillator ") + (osc == 0 ? "A" : "B") + " made from the sample.");
            break;
        }
        case toolClear:
            processor.setSample (nullptr);
            showStatus ("Sample removed.");
            break;
        default: break;
    }
}

void AMVSynthAudioProcessorEditor::showStatus (const juce::String& text)
{
    statusLabel.setText (text, juce::dontSendNotification);
    statusTicks = 120; // four seconds at the timer rate
}

void AMVSynthAudioProcessorEditor::updateModulationDisplay()
{
    if (statusTicks > 0 && --statusTicks == 0)
        statusLabel.setText ({}, juce::dontSendNotification);

    std::array<float, amv::mod::numSources> values {};
    bool changed = false;
    for (size_t i = 0; i < values.size(); ++i)
    {
        values[i] = processor.modSourceValues[i].load (std::memory_order_relaxed);
        changed = changed || std::abs (values[i] - sourceBar.values[i]) > 0.004f;
    }

    if (changed)
    {
        sourceBar.values = values;
        if (sourceBar.isVisible())
            sourceBar.repaint();
    }

    lfoDisplay.refresh();
    env3Display.refresh();

    // Collect the modulations once, then hand each knob its own.
    std::array<std::vector<Assignment>, amv::mod::numDests> byDest;
    for (int i = 0; i < amv::mod::numSlots; ++i)
    {
        const auto& slot = slotParams[(size_t) i];
        const int source = realValue (slot.source), dest = realValue (slot.dest);
        if (source != amv::mod::srcNone && dest >= 0 && dest < amv::mod::numDests)
            byDest[(size_t) dest].push_back ({ i, source, slot.amount->convertFrom0to1 (slot.amount->getValue()), realValue (slot.polarity) });
    }

    const bool dragging = isDragAndDropActive();
    for (auto& k : knobs)
    {
        auto& knob = k->slider;
        if (knob.dest < 0)
            continue;

        const auto& mine = byDest[(size_t) knob.dest];
        float live = 0.0f;
        for (const auto& a : mine)
            live += amv::mod::shaped (values[(size_t) a.source], a.source, a.polarity) * a.amount;

        const bool differs = mine != knob.assignments || std::abs (live - knob.live) > 0.003f || dragging != knob.showAsTarget
                             || (! dragging && knob.dropHover);
        if (! differs)
            continue;

        knob.assignments = mine;
        knob.live = live;
        knob.showAsTarget = dragging;
        knob.dropHover = knob.dropHover && dragging;
        if (knob.isVisible())
            knob.repaint();
    }
}

juce::Rectangle<float> AMVSynthAudioProcessorEditor::ModSourceBar::chipBounds (int source) const
{
    const auto area = getLocalBounds().toFloat().withTrimmedLeft (96.0f);
    const float width = area.getWidth() / (float) (amv::mod::numSources - 1);
    int place = 0;
    for (int i = 0; i < amv::mod::numSources - 1; ++i)
        if (chipOrder[i] == source)
            place = i;
    return juce::Rectangle<float> (area.getX() + width * (float) place, area.getY(), width, area.getHeight()).reduced (2.0f, 2.0f);
}

int AMVSynthAudioProcessorEditor::ModSourceBar::chipAt (juce::Point<float> position) const
{
    for (int source = 1; source < amv::mod::numSources; ++source)
        if (chipBounds (source).contains (position))
            return source;

    return 0;
}

void AMVSynthAudioProcessorEditor::ModSourceBar::paint (juce::Graphics& g)
{
    using namespace amv;
    g.setColour (colour::dimText);
    g.setFont (juce::FontOptions (10.5f, juce::Font::bold));
    g.drawText ("MOD SOURCES", getLocalBounds().withWidth (92), juce::Justification::centredLeft);

    for (int source = 1; source < mod::numSources; ++source)
    {
        const auto chip = chipBounds (source);
        const auto tint = colour::forSource (source);
        const bool lit = source == hover || source == pressed;

        g.setColour (lit ? tint.withAlpha (0.22f) : colour::panel);
        g.fillRoundedRectangle (chip, 6.0f);
        g.setColour (tint.withAlpha (lit ? 1.0f : 0.55f));
        g.drawRoundedRectangle (chip.reduced (0.5f), 6.0f, 1.0f);

        g.setColour (lit ? colour::text : tint);
        g.setFont (juce::FontOptions (10.5f, juce::Font::bold));
        g.drawText (chipNames[source], chip.withTrimmedBottom (5.0f), juce::Justification::centred);

        // A small meter along the bottom shows what the source is doing.
        const auto meter = chip.reduced (7.0f, 0.0f).withTop (chip.getBottom() - 6.5f).withHeight (2.5f);
        g.setColour (colour::track);
        g.fillRoundedRectangle (meter, 1.2f);

        const float v = juce::jlimit (-1.0f, 1.0f, values[(size_t) source]);
        g.setColour (tint);
        if (isBipolar (source))
        {
            const float half = meter.getWidth() * 0.5f, x = meter.getCentreX() + juce::jmin (0.0f, v) * half;
            g.fillRoundedRectangle (x, meter.getY(), juce::jmax (1.5f, std::abs (v) * half), meter.getHeight(), 1.2f);
        }
        else
            g.fillRoundedRectangle (meter.withWidth (juce::jmax (0.0f, v) * meter.getWidth()), 1.2f);
    }
}

void AMVSynthAudioProcessorEditor::ModSourceBar::mouseMove (const juce::MouseEvent& e)
{
    const int source = chipAt (e.position);
    if (source != hover)
    {
        hover = source;
        setMouseCursor (source != 0 ? juce::MouseCursor::DraggingHandCursor : juce::MouseCursor::NormalCursor);
        repaint();
    }
}

void AMVSynthAudioProcessorEditor::ModSourceBar::mouseExit (const juce::MouseEvent&)
{
    hover = 0;
    pressed = 0;
    repaint();
}

void AMVSynthAudioProcessorEditor::ModSourceBar::mouseDown (const juce::MouseEvent& e)
{
    pressed = chipAt (e.position);
    if (pressed != 0 && owner != nullptr)
        owner->showStatus (juce::String ("Drag ") + amv::getModSources()[(size_t) pressed] + " onto a knob");
    repaint();
}

void AMVSynthAudioProcessorEditor::ModSourceBar::mouseDrag (const juce::MouseEvent& e)
{
    if (pressed == 0 || owner == nullptr || owner->isDragAndDropActive() || e.getDistanceFromDragStart() < 4)
        return;

    const auto chip = chipBounds (pressed).getSmallestIntegerContainer();
    owner->startDragging (juce::String (sourceTag) + juce::String (pressed), this, juce::ScaledImage (createComponentSnapshot (chip)));
    pressed = 0;
}

void AMVSynthAudioProcessorEditor::showLfo (int index)
{
    index = juce::jlimit (0, amv::mod::numLfos - 1, index);
    for (int i = 0; i < amv::mod::numLfos; ++i)
        lfoSelect[(size_t) i].setToggleState (i == index, juce::dontSendNotification);

    lfoDisplay.lfo = index;
    auto& s = sections[lfoSection];
    s.title = "LFO " + juce::String (index + 1);
    s.column = lfoColumns[(size_t) index];
    s.knobs = lfoKnobs[(size_t) index];
    layoutSection (s);
    updateVisibility();
    lfoDisplay.refresh();
    repaint();
}

juce::Rectangle<float> AMVSynthAudioProcessorEditor::LfoDisplay::graph() const
{
    return getLocalBounds().toFloat().reduced (10.0f, 10.0f);
}

juce::Point<float> AMVSynthAudioProcessorEditor::LfoDisplay::toScreen (float x, float y) const
{
    const auto area = graph();
    return { area.getX() + x * area.getWidth(), area.getCentreY() - y * area.getHeight() * 0.5f };
}

juce::Point<float> AMVSynthAudioProcessorEditor::LfoDisplay::handleFor (size_t segment) const
{
    const auto& a = points[segment];
    const auto& b = points[segment + 1];
    return toScreen (0.5f * (a.x + b.x), a.y + (b.y - a.y) * amv::LfoShape::bend (0.5f, a.curve));
}

int AMVSynthAudioProcessorEditor::LfoDisplay::pointAt (juce::Point<float> p) const
{
    int best = -1;
    float bestDistance = 9.0f;
    for (size_t i = 0; i < points.size(); ++i)
    {
        const float distance = p.getDistanceFrom (toScreen (points[i].x, points[i].y));
        if (distance < bestDistance)
        {
            bestDistance = distance;
            best = (int) i;
        }
    }
    return best;
}

int AMVSynthAudioProcessorEditor::LfoDisplay::handleAt (juce::Point<float> p) const
{
    for (size_t i = 0; i + 1 < points.size(); ++i)
        if (points[i + 1].x - points[i].x > 0.02f && std::abs (points[i + 1].y - points[i].y) > 0.02f && p.getDistanceFrom (handleFor (i)) < 7.0f)
            return (int) i;
    return -1;
}

float AMVSynthAudioProcessorEditor::LfoDisplay::valueAt (float ph) const
{
    static const float steps[6] { 0.5f, -0.7f, 0.9f, -0.2f, 0.35f, -0.9f };

    switch (shape)
    {
        case 1:  return ph < 0.5f ? 4.0f * ph - 1.0f : 3.0f - 4.0f * ph;
        case 2:  return 1.0f - 2.0f * ph;
        case 3:  return ph < 0.5f ? 1.0f : -1.0f;
        case 4:  return steps[juce::jmin (5, (int) (ph * 6.0f))]; // random is drawn as steps
        case amv::lfoShapeCustom: break;
        default: return std::sin (ph * juce::MathConstants<float>::twoPi);
    }

    size_t i = 0;
    while (i + 2 < points.size() && points[i + 1].x <= ph)
        ++i;
    if (points.size() < 2)
        return 0.0f;
    const auto& a = points[i];
    const auto& b = points[i + 1];
    const float width = b.x - a.x;
    return width < 1.0e-6f ? b.y : a.y + (b.y - a.y) * amv::LfoShape::bend (juce::jlimit (0.0f, 1.0f, (ph - a.x) / width), a.curve);
}

void AMVSynthAudioProcessorEditor::LfoDisplay::refresh()
{
    if (owner == nullptr)
        return;

    auto& processor = owner->processor;
    const auto raw = [&] (const char* suffix) { return juce::roundToInt (processor.apvts.getRawParameterValue (amv::id::lfo (lfo, suffix))->load()); };
    const int newShape = raw ("Shape"), newMode = raw ("Mode");
    const float newPhase = processor.lfoPhases[(size_t) lfo].load (std::memory_order_relaxed);
    bool changed = newShape != shape || newMode != mode || lfo != shownLfo || std::abs (newPhase - phase) > 0.002f;

    // The points come from the processor, except while one is being dragged here.
    const auto& stored = processor.getLfoPoints (lfo);
    if (dragPoint < 0 && dragHandle < 0 && (&stored != pointsFrom || lfo != shownLfo))
    {
        points = stored;
        pointsFrom = &stored;
        changed = true;
    }

    shape = newShape;
    mode = newMode;
    phase = newPhase;
    shownLfo = lfo;

    if (changed && isVisible())
        repaint();
}

void AMVSynthAudioProcessorEditor::LfoDisplay::setPoints (std::vector<amv::LfoPoint> newPoints, bool finished)
{
    auto& processor = owner->processor;
    processor.setLfoShape (lfo, std::move (newPoints), finished);
    points = processor.getLfoPoints (lfo);
    pointsFrom = &processor.getLfoPoints (lfo);

    if (shape != amv::lfoShapeCustom)
        if (auto* param = processor.apvts.getParameter (amv::id::lfo (lfo, "Shape")))
        {
            param->setValueNotifyingHost (param->convertTo0to1 ((float) amv::lfoShapeCustom));
            shape = amv::lfoShapeCustom;
        }

    repaint();
}

void AMVSynthAudioProcessorEditor::LfoDisplay::paint (juce::Graphics& g)
{
    using namespace amv;
    g.setColour (colour::background);
    g.fillRoundedRectangle (getLocalBounds().toFloat(), 6.0f);

    const auto area = graph();
    const auto tint = colour::forSource (mod::lfoSource (lfo));
    const bool custom = shape == lfoShapeCustom;

    // A grid of sixteen steps, with the quarters a little brighter.
    for (int i = 0; i <= 16; ++i)
    {
        g.setColour (colour::track.withAlpha (i % 4 == 0 ? 0.9f : 0.35f));
        g.fillRect (area.getX() + area.getWidth() * (float) i / 16.0f - 0.5f, area.getY(), 1.0f, area.getHeight());
    }
    g.setColour (colour::track);
    g.fillRect (area.getX(), area.getCentreY() - 0.5f, area.getWidth(), 1.0f);

    juce::Path path;
    const int count = juce::jmax (2, (int) area.getWidth());
    for (int i = 0; i <= count; ++i)
    {
        const float ph = (float) i / (float) count;
        const auto p = toScreen (ph, valueAt (juce::jmin (ph, 0.99999f)));
        if (i == 0)
            path.startNewSubPath (p);
        else
            path.lineTo (p);
    }

    auto fill = path;
    fill.lineTo (area.getRight(), area.getCentreY());
    fill.lineTo (area.getX(), area.getCentreY());
    fill.closeSubPath();
    g.setColour (tint.withAlpha (0.12f));
    g.fillPath (fill);
    g.setColour (tint.withAlpha (0.9f));
    g.strokePath (path, juce::PathStrokeType (1.7f, juce::PathStrokeType::mitered, juce::PathStrokeType::rounded));

    if (custom)
    {
        for (size_t i = 0; i + 1 < points.size(); ++i)
            if (points[i + 1].x - points[i].x > 0.02f && std::abs (points[i + 1].y - points[i].y) > 0.02f)
            {
                const auto h = handleFor (i);
                const bool lit = hoverHandle == (int) i || dragHandle == (int) i;
                g.setColour (colour::background);
                g.fillEllipse (h.x - 3.5f, h.y - 3.5f, 7.0f, 7.0f);
                g.setColour (tint.withAlpha (lit ? 1.0f : 0.6f));
                g.drawEllipse (h.x - 3.5f, h.y - 3.5f, 7.0f, 7.0f, 1.3f);
            }

        for (size_t i = 0; i < points.size(); ++i)
        {
            const auto p = toScreen (points[i].x, points[i].y);
            const bool lit = hoverPoint == (int) i || dragPoint == (int) i;
            const float r = lit ? 5.5f : 4.5f;
            g.setColour (lit ? colour::text : tint);
            g.fillEllipse (p.x - r, p.y - r, 2.0f * r, 2.0f * r);
            g.setColour (colour::background);
            g.drawEllipse (p.x - r, p.y - r, 2.0f * r, 2.0f * r, 1.2f);
        }
    }

    // Where the LFO is now.
    const float ph = juce::jlimit (0.0f, 1.0f, phase);
    const auto now = toScreen (ph, shape == 4 ? 0.0f : valueAt (juce::jmin (ph, 0.99999f)));
    g.setColour (colour::text.withAlpha (0.45f));
    g.fillRect (now.x - 0.5f, area.getY(), 1.0f, area.getHeight());
    if (shape != 4)
    {
        g.setColour (colour::text);
        g.fillEllipse (now.x - 3.0f, now.y - 3.0f, 6.0f, 6.0f);
    }
}

void AMVSynthAudioProcessorEditor::LfoDisplay::mouseMove (const juce::MouseEvent& e)
{
    const int point = shape == amv::lfoShapeCustom ? pointAt (e.position) : -1;
    const int handle = shape == amv::lfoShapeCustom && point < 0 ? handleAt (e.position) : -1;
    if (point != hoverPoint || handle != hoverHandle)
    {
        hoverPoint = point;
        hoverHandle = handle;
        setMouseCursor (point >= 0 ? juce::MouseCursor::DraggingHandCursor
                                   : handle >= 0 ? juce::MouseCursor::UpDownResizeCursor : juce::MouseCursor::NormalCursor);
        repaint();
    }
}

void AMVSynthAudioProcessorEditor::LfoDisplay::mouseExit (const juce::MouseEvent&)
{
    hoverPoint = hoverHandle = -1;
    repaint();
}

void AMVSynthAudioProcessorEditor::LfoDisplay::mouseDown (const juce::MouseEvent& e)
{
    if (e.mods.isPopupMenu())
    {
        showMenu();
        return;
    }

    if (shape != amv::lfoShapeCustom)
        return;

    dragPoint = pointAt (e.position);
    dragHandle = dragPoint < 0 ? handleAt (e.position) : -1;
    if (dragHandle >= 0)
        curveStart = points[(size_t) dragHandle].curve;
}

void AMVSynthAudioProcessorEditor::LfoDisplay::mouseDrag (const juce::MouseEvent& e)
{
    const auto area = graph();

    if (dragPoint >= 0 && dragPoint < (int) points.size())
    {
        const auto index = (size_t) dragPoint;
        float x = (e.position.x - area.getX()) / juce::jmax (1.0f, area.getWidth());
        float y = (area.getCentreY() - e.position.y) / juce::jmax (1.0f, area.getHeight() * 0.5f);

        // On the grid unless Alt is held: sixteen steps across, eight up and down.
        if (! e.mods.isAltDown())
        {
            x = std::round (x * 16.0f) / 16.0f;
            y = std::round (y * 8.0f) / 8.0f;
        }

        // The first and last point stay at the ends; the others stay between their neighbours.
        if (index == 0)
            x = 0.0f;
        else if (index + 1 == points.size())
            x = 1.0f;
        else
            x = juce::jlimit (points[index - 1].x, points[index + 1].x, x);

        auto edited = points;
        edited[index].x = x;
        edited[index].y = juce::jlimit (-1.0f, 1.0f, y);
        owner->processor.setLfoShape (lfo, edited, false);
        points = edited;
        repaint();
    }
    else if (dragHandle >= 0 && dragHandle + 1 < (int) points.size())
    {
        const auto index = (size_t) dragHandle;
        const float rising = points[index + 1].y > points[index].y ? 1.0f : -1.0f;
        const float moved = (float) -e.getDistanceFromDragStartY() / juce::jmax (1.0f, area.getHeight() * 0.5f);
        auto edited = points;
        edited[index].curve = juce::jlimit (-1.0f, 1.0f, curveStart - moved * rising * 1.5f);
        owner->processor.setLfoShape (lfo, edited, false);
        points = edited;
        repaint();
    }
}

void AMVSynthAudioProcessorEditor::LfoDisplay::mouseUp (const juce::MouseEvent&)
{
    if (dragPoint >= 0 || dragHandle >= 0)
    {
        dragPoint = dragHandle = -1;
        setPoints (points, true);
    }
}

void AMVSynthAudioProcessorEditor::LfoDisplay::mouseDoubleClick (const juce::MouseEvent& e)
{
    if (shape != amv::lfoShapeCustom)
        return;

    auto edited = points;
    const int point = pointAt (e.position);

    if (point > 0 && point + 1 < (int) edited.size())
    {
        edited.erase (edited.begin() + point);
    }
    else if (point < 0 && edited.size() < (size_t) amv::LfoShape::maxPoints)
    {
        const auto area = graph();
        float x = juce::jlimit (0.0f, 1.0f, (e.position.x - area.getX()) / juce::jmax (1.0f, area.getWidth()));
        float y = juce::jlimit (-1.0f, 1.0f, (area.getCentreY() - e.position.y) / juce::jmax (1.0f, area.getHeight() * 0.5f));
        if (! e.mods.isAltDown())
        {
            x = std::round (x * 16.0f) / 16.0f;
            y = std::round (y * 8.0f) / 8.0f;
        }
        edited.push_back ({ x, y, 0.0f });
    }
    else
    {
        return;
    }

    hoverPoint = hoverHandle = -1;
    setPoints (std::move (edited), true);
}

void AMVSynthAudioProcessorEditor::LfoDisplay::showMenu()
{
    using Points = std::vector<amv::LfoPoint>;
    const auto stairs = [] (int count, bool up)
    {
        Points result;
        for (int i = 0; i < count; ++i)
        {
            const float level = (float) i / (float) (count - 1) * 2.0f - 1.0f;
            const float y = up ? level : -level;
            result.push_back ({ (float) i / (float) count, y, 0.0f });
            result.push_back ({ (float) (i + 1) / (float) count, y, 0.0f });
        }
        return result;
    };

    juce::PopupMenu menu;
    menu.addSectionHeader ("Draw a custom shape from");
    const char* names[] { "Triangle", "Ramp up", "Ramp down", "Square", "Stairs up, 4 steps", "Stairs down, 8 steps", "Sidechain pump",
                          "Pluck", "Random, 8 steps", "Random, 16 steps" };
    for (int i = 0; i < (int) std::size (names); ++i)
        menu.addItem (i + 1, names[i]);

    juce::Component::SafePointer<LfoDisplay> safe (this);
    menu.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (this),
                        [safe, stairs] (int result)
                        {
                            if (safe == nullptr || result <= 0)
                                return;

                            Points made;
                            switch (result)
                            {
                                case 1:  made = { { 0.0f, -1.0f, 0.0f }, { 0.5f, 1.0f, 0.0f }, { 1.0f, -1.0f, 0.0f } }; break;
                                case 2:  made = { { 0.0f, -1.0f, 0.0f }, { 1.0f, 1.0f, 0.0f } }; break;
                                case 3:  made = { { 0.0f, 1.0f, 0.0f }, { 1.0f, -1.0f, 0.0f } }; break;
                                case 4:  made = { { 0.0f, 1.0f, 0.0f }, { 0.5f, 1.0f, 0.0f }, { 0.5f, -1.0f, 0.0f }, { 1.0f, -1.0f, 0.0f } }; break;
                                case 5:  made = stairs (4, true); break;
                                case 6:  made = stairs (8, false); break;
                                case 7:  made = { { 0.0f, -1.0f, -0.7f }, { 0.75f, 1.0f, 0.0f }, { 1.0f, 1.0f, 0.0f } }; break;
                                case 8:  made = { { 0.0f, 1.0f, -0.8f }, { 1.0f, -1.0f, 0.0f } }; break;
                                default:
                                {
                                    const int count = result == 9 ? 8 : 16;
                                    juce::Random random;
                                    for (int i = 0; i < count; ++i)
                                    {
                                        const float y = std::round ((random.nextFloat() * 2.0f - 1.0f) * 8.0f) / 8.0f;
                                        made.push_back ({ (float) i / (float) count, y, 0.0f });
                                        made.push_back ({ (float) (i + 1) / (float) count, y, 0.0f });
                                    }
                                    break;
                                }
                            }

                            safe->setPoints (std::move (made), true);
                        });
}

void AMVSynthAudioProcessorEditor::TextDisplay::paint (juce::Graphics& g)
{
    auto area = getLocalBounds().toFloat().reduced (4.0f, 2.0f);
    const float lineHeight = juce::jmin (24.0f, area.getHeight() / (float) juce::jmax (1, lines.size()));
    g.setFont (juce::FontOptions (13.0f));

    for (const auto& line : lines)
    {
        auto row = area.removeFromTop (lineHeight);
        g.setColour (amv::colour::teal);
        g.fillEllipse (row.getX(), row.getCentreY() - 2.0f, 4.0f, 4.0f);
        g.setColour (amv::colour::text.withAlpha (0.85f));
        g.drawText (line, row.withTrimmedLeft (12.0f), juce::Justification::centredLeft);
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
    g.setGradientFill (juce::ColourGradient (colour::background.brighter (0.04f), 0.0f, 0.0f,
                                             colour::background.darker (0.25f), 0.0f, (float) getHeight(), false));
    g.fillAll();

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
    g.drawText ("v" JucePlugin_VersionString, logo.removeFromLeft (50), juce::Justification::centredLeft);

    for (const auto& s : sections)
    {
        if (s.page != currentPage)
            continue;

        // A soft shadow, a panel that is a little lighter at the top, and a hairline border.
        const auto r = s.bounds.toFloat();
        const auto base = s.balkan ? colour::balkanPanel : colour::panel;
        g.setColour (juce::Colours::black.withAlpha (0.28f));
        g.fillRoundedRectangle (r.translated (0.0f, 2.0f), 10.0f);
        g.setGradientFill (juce::ColourGradient (base.brighter (0.07f), 0.0f, r.getY(), base.darker (0.12f), 0.0f, r.getBottom(), false));
        g.fillRoundedRectangle (r, 10.0f);
        g.setColour ((s.balkan ? colour::balkanBorder : colour::panelBorder).withAlpha (0.8f));
        g.drawRoundedRectangle (r.reduced (0.5f), 10.0f, 1.0f);

        auto titleArea = s.bounds.reduced (12, 0).removeFromTop (titleHeight + 4);
        if (s.balkan)
        {
            drawKilim (g, titleArea.removeFromLeft (30).toFloat().withSizeKeepingCentre (30.0f, 10.0f), colour::red, 3);
            titleArea.removeFromLeft (8);
        }
        else
        {
            g.setColour (colour::teal);
            g.fillRoundedRectangle (titleArea.removeFromLeft (3).toFloat().withSizeKeepingCentre (3.0f, 12.0f), 1.5f);
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
        const int cellWidth = s.knobs.empty() ? inner.getWidth() / cols
                                              : s.columnWidth > 0 ? juce::jmin (s.columnWidth, inner.getWidth() * 3 / 5)
                                                                  : juce::jmin (150, inner.getWidth() / 3);
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
        // Without knobs the display takes the whole panel.
        if (s.knobs.empty())
            s.display->setBounds (inner.withTrimmedBottom (2));
        else
            s.display->setBounds (inner.removeFromLeft (juce::roundToInt ((float) inner.getWidth() * s.displayShare)).reduced (0, 6));
        inner.removeFromLeft (10);
    }

    if (s.knobs.empty())
        return;

    const int knobWidth = inner.getWidth() / (int) s.knobs.size();

    for (auto* knob : s.knobs)
    {
        auto cell = inner.removeFromLeft (knobWidth);
        const int size = juce::jmin (cell.getWidth(), cell.getHeight() - 20, 70);
        auto group = cell.withSizeKeepingCentre (cell.getWidth(), size + 20);
        knob->slider.setBounds (group.removeFromTop (size).withSizeKeepingCentre (size, size));
        knob->label.setBounds (group);
    }
}

void AMVSynthAudioProcessorEditor::resized()
{
    auto area = getLocalBounds();
    auto headerArea = area.removeFromTop (headerHeight).reduced (0, 12);
    headerArea.removeFromRight (16);
    for (size_t i = tabs.size(); i-- > 0;)
    {
        static constexpr int widths[] { 56, 46, 52, 66, 44, 46, 40, 56 };
        tabs[i].setBounds (headerArea.removeFromRight (widths[i]));
        headerArea.removeFromRight (3);
    }

    // The preset bar fills the space between the logo and the tabs.
    headerArea.removeFromLeft (250);
    headerArea.removeFromRight (14);
    browsePreset.setBounds (headerArea.removeFromRight (64));
    headerArea.removeFromRight (4);
    savePreset.setBounds (headerArea.removeFromRight (50));
    headerArea.removeFromRight (8);
    prevPreset.setBounds (headerArea.removeFromLeft (26));
    headerArea.removeFromLeft (4);
    nextPreset.setBounds (headerArea.removeFromRight (26));
    headerArea.removeFromRight (4);
    presetBox.setBounds (headerArea);

    keyboard.setBounds (area.removeFromBottom (keyboardHeight));
    keyboard.setKeyWidth ((float) keyboard.getWidth() / 36.0f); // 36 white keys from C2 to C7
    area.reduce (margin, margin);
    browser.setBounds (area);

    // The modulation sources sit above the panels on every page that has knobs to drop them on.
    // The wavetable editor lays out its own page.
    waveEditor.setBounds (area);

    if (hasSourceBar())
    {
        auto bar = area.removeFromTop (sourceBarHeight);
        area.removeFromTop (gap - 2);
        statusLabel.setBounds (bar.removeFromRight (190));
        sourceBar.setBounds (bar);
    }

    int numRows = 1;
    for (const auto& s : sections)
        if (s.page == currentPage)
            numRows = juce::jmax (numRows, s.row + 1);

    // The FX page has fewer rows; keep them the same height as on the main page.
    const int layoutRows = juce::jmax (numRows, 4);
    const int rowHeight = (area.getHeight() - gap * (layoutRows - 1)) / layoutRows;

    // On the SEQ page the step grid is three rows tall.
    const bool tallFirstRow = currentPage == seqPage;

    for (int row = 0; row < numRows; ++row)
    {
        if (tallFirstRow && (row == 1 || row == 2))
            continue;

        auto rowArea = area.removeFromTop (tallFirstRow && row == 0 ? rowHeight * 3 + gap * 2 : rowHeight);
        area.removeFromTop (gap);

        float totalWeight = 0.0f;
        int count = 0;
        for (const auto& s : sections)
            if (s.page == currentPage && s.row == row)
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
            if (s.page != currentPage || s.row != row)
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
