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
        addChildComponent (patternDisplay);
    }

    // Row 3: filter, envelopes, output.
    addSection ("FILTER", false, 3, 3.4f).knobs = makeKnobs (false, { { id::cutoff, "CUTOFF" }, { id::resonance, "RES" }, { id::filterEnv, "ENV" } });
    addSection ("AMP ENV", false, 3, 4.4f).knobs = makeKnobs (false, { { id::ampA, "A" }, { id::ampD, "D" }, { id::ampS, "S" }, { id::ampR, "R" } });
    addSection ("FILTER ENV", false, 3, 4.4f).knobs = makeKnobs (false, { { id::fltA, "A" }, { id::fltD, "D" }, { id::fltS, "S" }, { id::fltR, "R" } });
    addSection ("OUT", false, 3, 1.4f).knobs = makeKnobs (false, { { id::level, "LEVEL" } });

    // OSC page: two wavetable oscillators, sub, noise and the level of the Balkan engine.
    buildingPage = 1;
    {
        auto& s = addSection ("OSC A", false, 0, 1.0f);
        s.column.push_back (&makeChoice (id::oscATable, false)->box);
        s.display = &waveA;
        s.knobs = makeKnobs (false, { { id::oscAPos, "POSITION" }, { id::oscAUnison, "UNISON" }, { id::oscADetune, "DETUNE" }, { id::oscAWidth, "WIDTH" },
                                      { id::oscAPitch, "PITCH" }, { id::oscAFine, "FINE" }, { id::oscALevel, "LEVEL" } });
        addChildComponent (waveA);
    }
    {
        auto& s = addSection ("OSC B", false, 1, 1.0f);
        s.column.push_back (&makeChoice (id::oscBTable, false)->box);
        s.display = &waveB;
        s.knobs = makeKnobs (false, { { id::oscBPos, "POSITION" }, { id::oscBUnison, "UNISON" }, { id::oscBDetune, "DETUNE" }, { id::oscBWidth, "WIDTH" },
                                      { id::oscBPitch, "PITCH" }, { id::oscBFine, "FINE" }, { id::oscBLevel, "LEVEL" } });
        addChildComponent (waveB);
    }
    {
        auto& s = addSection ("SUB", false, 2, 4.0f);
        s.column.push_back (&makeChoice (id::subOctave, false)->box);
        s.knobs = makeKnobs (false, { { id::subShape, "SHAPE" }, { id::subLevel, "LEVEL" } });
    }
    addSection ("NOISE", false, 2, 3.0f).knobs = makeKnobs (false, { { id::noiseColor, "COLOR" }, { id::noiseLevel, "LEVEL" } });
    addSection ("BALKAN ENGINE", true, 2, 2.0f).knobs = makeKnobs (true, { { id::engineLevel, "LEVEL" } });

    // MOD page: two LFOs, four macros and the modulation matrix.
    buildingPage = 2;
    for (int i = 0; i < mod::numLfos; ++i)
    {
        auto& s = addSection ("LFO " + juce::String (i + 1), false, 0, 3.0f);
        s.columnWidth = 130;
        s.column.push_back (&makeChoice (id::lfo (i, "Shape"), false)->box);
        s.column.push_back (&makeToggle (id::lfo (i, "Sync"), "TEMPO SYNC", false)->button);
        s.column.push_back (&makeChoice (id::lfo (i, "Division"), false)->box);
        s.knobs = makeKnobs (false, { { id::lfo (i, "Rate"), "RATE" } });
    }
    addSection ("MACROS", false, 0, 5.0f).knobs = makeKnobs (false, { { id::macro (0), "MACRO 1" }, { id::macro (1), "MACRO 2" },
                                                                      { id::macro (2), "MACRO 3" }, { id::macro (3), "MACRO 4" } });
    for (int i = 0; i < mod::numSlots; ++i)
    {
        auto& s = addSection ("MOD " + juce::String (i + 1), false, 1 + i / 3, 1.0f);
        s.columnWidth = 175;
        s.column.push_back (&makeChoice (id::slot (i, "Source"), false)->box);
        s.column.push_back (&makeChoice (id::slot (i, "Dest"), false)->box);
        s.knobs = makeKnobs (false, { { id::slot (i, "Amount"), "AMOUNT" } });
    }

    // FX page.
    buildingPage = 3;
    {
        auto& s = addSection ("DISTORTION", false, 0, 4.0f);
        s.column.push_back (&makeToggle (id::distOn, "ON", false)->button);
        s.column.push_back (&makeChoice (id::distType, false)->box);
        s.knobs = makeKnobs (false, { { id::distDrive, "DRIVE" }, { id::distMix, "MIX" } });
    }
    {
        auto& s = addSection ("TRANCE GATE", false, 0, 6.0f);
        s.column.push_back (&makeToggle (id::gateOn, "ON", false)->button);
        s.column.push_back (&makeChoice (id::gateRate, false)->box);
        s.column.push_back (&makeChoice (id::gatePattern, false)->box);
        s.knobs = makeKnobs (false, { { id::gateDepth, "DEPTH" }, { id::gateLength, "LENGTH" }, { id::gateSmooth, "SMOOTH" } });
    }
    {
        auto& s = addSection ("DELAY", false, 1, 6.0f);
        s.column.push_back (&makeToggle (id::delayOn, "ON", false)->button);
        s.column.push_back (&makeChoice (id::delayTime, false)->box);
        s.column.push_back (&makeToggle (id::delayPingPong, "PING-PONG", false)->button);
        s.knobs = makeKnobs (false, { { id::delayFeedback, "FEEDBACK" }, { id::delayTone, "TONE" }, { id::delayMix, "MIX" } });
    }
    {
        auto& s = addSection ("REVERB", false, 1, 5.0f);
        s.column.push_back (&makeToggle (id::reverbOn, "ON", false)->button);
        s.knobs = makeKnobs (false, { { id::reverbSize, "SIZE" }, { id::reverbDamp, "DAMP" }, { id::reverbMix, "MIX" } });
    }
    {
        auto& s = addSection ("KICK DUCK", false, 2, 4.0f);
        s.column.push_back (&makeToggle (id::duckOn, "ON", false)->button);
        s.knobs = makeKnobs (false, { { id::duckDepth, "DEPTH" }, { id::duckRelease, "RELEASE" } });
    }
    {
        auto& s = addSection ("ROUTING", false, 2, 3.0f);
        s.column.push_back (&makeChoice (id::fxOrder, false)->box);
    }
    {
        auto& s = addSection ("OUTPUT", false, 2, 4.0f);
        s.display = &levelMeter;
        s.knobs = makeKnobs (false, { { id::level, "LEVEL" } });
        addChildComponent (levelMeter);
    }
    buildingPage = 0;

    const char* tabNames[] { "SYNTH", "OSC", "MOD", "FX" };
    for (size_t i = 0; i < tabs.size(); ++i)
    {
        auto& tab = tabs[i];
        tab.setButtonText (tabNames[i]);
        tab.setClickingTogglesState (false);
        tab.setColour (juce::TextButton::buttonColourId, colour::header);
        tab.setColour (juce::TextButton::buttonOnColourId, colour::track);
        tab.setColour (juce::TextButton::textColourOffId, colour::dimText);
        tab.setColour (juce::TextButton::textColourOnId, colour::text);
        tab.onClick = [this, i] { setPage ((int) i); };
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

    const auto presetFolder = juce::File::getSpecialLocation (juce::File::userDocumentsDirectory).getChildFile ("AMVSynth Presets");
    savePreset.onClick = [this, presetFolder]
    {
        presetFolder.createDirectory();
        fileChooser = std::make_unique<juce::FileChooser> ("Save preset", presetFolder, "*.amvpreset");
        fileChooser->launchAsync (juce::FileBrowserComponent::saveMode | juce::FileBrowserComponent::canSelectFiles | juce::FileBrowserComponent::warnAboutOverwriting,
                                  [this] (const juce::FileChooser& chooser)
                                  {
                                      const auto file = chooser.getResult();
                                      if (file != juce::File())
                                          processor.savePresetFile (file.withFileExtension ("amvpreset"));
                                  });
    };
    loadPreset.onClick = [this, presetFolder]
    {
        fileChooser = std::make_unique<juce::FileChooser> ("Load preset", presetFolder, "*.amvpreset");
        fileChooser->launchAsync (juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles,
                                  [this] (const juce::FileChooser& chooser)
                                  {
                                      const auto file = chooser.getResult();
                                      if (file.existsAsFile() && processor.loadPresetFile (file))
                                      {
                                          shownPreset = -1;
                                          presetBox.setSelectedId (0, juce::dontSendNotification);
                                          presetBox.setTextWhenNothingSelected (file.getFileNameWithoutExtension());
                                      }
                                  });
    };

    presetBox.setColour (juce::ComboBox::outlineColourId, colour::panelBorder);
    addAndMakeVisible (presetBox);
    for (auto* button : { &prevPreset, &nextPreset, &savePreset, &loadPreset })
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

    showModel (juce::roundToInt (processor.apvts.getRawParameterValue (id::model)->load()));
    updateVisibility();
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

void AMVSynthAudioProcessorEditor::selectPreset (int index)
{
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
    for (size_t i = 0; i < tabs.size(); ++i)
        tabs[i].setToggleState ((int) i == page, juce::dontSendNotification);
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
    }

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
        const float next = juce::jmax (peak, levelMeter.level[ch] * 0.8f);
        meterChanged = meterChanged || std::abs (next - levelMeter.level[ch]) > 1.0e-4f;
        levelMeter.level[ch] = next < 1.0e-4f ? 0.0f : next;

        if (peak >= 0.99f)
        {
            levelMeter.clipHold = 20; // one second at the timer rate
            meterChanged = true;
        }
    }
    if (levelMeter.clipHold > 0 && --levelMeter.clipHold == 0)
        meterChanged = true;
    if (meterChanged && levelMeter.isVisible())
        levelMeter.repaint();

    const auto param = [this] (const char* paramId) { return processor.apvts.getRawParameterValue (paramId)->load(); };
    const auto sync = [] (WaveDisplay& d, int table, float position, float level)
    {
        if (table != d.table || std::abs (position - d.position) > 1.0e-4f || (level > 0.0f) != d.active)
        {
            d.table = table;
            d.position = position;
            d.active = level > 0.0f;
            d.repaint();
        }
    };
    sync (waveA, juce::roundToInt (param (amv::id::oscATable)), param (amv::id::oscAPos), param (amv::id::oscALevel));
    sync (waveB, juce::roundToInt (param (amv::id::oscBTable)), param (amv::id::oscBPos), param (amv::id::oscBLevel));
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
    area.reduce (10.0f, 10.0f);

    g.setColour (colour::track);
    g.fillRect (area.getX(), area.getCentreY() - 0.5f, area.getWidth(), 1.0f);

    const auto& bank = WavetableBank::get();
    const int t = juce::jlimit (0, WavetableBank::numTables - 1, table);
    const float framePos = juce::jlimit (0.0f, 1.0f, position) * (float) (WavetableBank::numFrames - 1);
    const int frame = juce::jmin ((int) framePos, WavetableBank::numFrames - 2);
    const float mix = framePos - (float) frame;
    const float* a = bank.wave (t, frame, 2);
    const float* b = bank.wave (t, frame + 1, 2);

    juce::Path path;
    const int points = juce::jmax (2, (int) area.getWidth());
    for (int i = 0; i < points; ++i)
    {
        const int index = i * WavetableBank::tableSize / (points - 1);
        const float v = a[index] + (b[index] - a[index]) * mix;
        const float x = area.getX() + (float) i, y = area.getCentreY() - v * area.getHeight() * 0.47f;
        if (i == 0)
            path.startNewSubPath (x, y);
        else
            path.lineTo (x, y);
    }

    // An oscillator that is turned down is drawn dimmed.
    g.setColour (colour::teal.withAlpha (active ? 1.0f : 0.4f));
    g.strokePath (path, juce::PathStrokeType (1.6f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
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
    g.drawText ("v" JucePlugin_VersionString, logo.removeFromLeft (50), juce::Justification::centredLeft);

    for (const auto& s : sections)
    {
        if (s.page != currentPage)
            continue;

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
    }
}

void AMVSynthAudioProcessorEditor::resized()
{
    auto area = getLocalBounds();
    auto headerArea = area.removeFromTop (headerHeight).reduced (0, 12);
    headerArea.removeFromRight (16);
    for (size_t i = tabs.size(); i-- > 0;)
    {
        tabs[i].setBounds (headerArea.removeFromRight (70));
        headerArea.removeFromRight (6);
    }

    // The preset bar fills the space between the logo and the tabs.
    headerArea.removeFromLeft (250);
    headerArea.removeFromRight (14);
    loadPreset.setBounds (headerArea.removeFromRight (50));
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

    int numRows = 1;
    for (const auto& s : sections)
        if (s.page == currentPage)
            numRows = juce::jmax (numRows, s.row + 1);

    // The FX page has fewer rows; keep them the same height as on the main page.
    const int layoutRows = juce::jmax (numRows, 4);
    const int rowHeight = (area.getHeight() - gap * (layoutRows - 1)) / layoutRows;

    for (int row = 0; row < numRows; ++row)
    {
        auto rowArea = area.removeFromTop (rowHeight);
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
