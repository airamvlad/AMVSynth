#pragma once

#include "PluginProcessor.h"
#include "PresetBrowser.h"
#include "SampleDisplay.h"
#include "StepEditor.h"
#include "WavetableEditor.h"

namespace amv
{
namespace colour
{
inline const juce::Colour background { 0xff0e1116 };
inline const juce::Colour header { 0xff141920 };
inline const juce::Colour panel { 0xff171c24 };
inline const juce::Colour panelBorder { 0xff262d38 };
inline const juce::Colour balkanPanel { 0xff1c171c };
inline const juce::Colour balkanBorder { 0xff5a2a33 };
inline const juce::Colour track { 0xff2a3340 };
inline const juce::Colour knob { 0xff222a35 };
inline const juce::Colour text { 0xffe6e9ee };
inline const juce::Colour dimText { 0xff9aa4b2 };
inline const juce::Colour teal { 0xff2dd4bf };
inline const juce::Colour red { 0xfff0616d };

/** Every modulation source has its own colour, on its chip and on the rings around the knobs. */
inline juce::Colour forSource (int source)
{
    static const juce::uint32 colours[amv::mod::numSources] { 0xff9aa4b2, 0xfff5b84a, 0xffb48cf2, 0xff4aa8ff, 0xff7ddc6f, 0xffff8a5c,
                                                              0xfff06292, 0xffffd54f, 0xff4dd0e1, 0xffaed581, 0xffe57373, 0xff9fa8da,
                                                              0xffff7043, 0xff26c6da, 0xff64b5f6 };
    return juce::Colour (colours[juce::jlimit (0, (int) amv::mod::numSources - 1, source)]);
}
} // namespace colour

class LookAndFeel final : public juce::LookAndFeel_V4
{
public:
    LookAndFeel();
    void drawRotarySlider (juce::Graphics&, int x, int y, int width, int height, float sliderPos,
                           float startAngle, float endAngle, juce::Slider&) override;
    void drawToggleButton (juce::Graphics&, juce::ToggleButton&, bool highlighted, bool down) override;
};
} // namespace amv

class AMVSynthAudioProcessorEditor final : public juce::AudioProcessorEditor,
                                           public juce::DragAndDropContainer,
                                           private juce::Timer
{
public:
    explicit AMVSynthAudioProcessorEditor (AMVSynthAudioProcessor&);
    ~AMVSynthAudioProcessorEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    using APVTS = juce::AudioProcessorValueTreeState;

    /** One modulation routed to a knob. */
    struct Assignment
    {
        int slot = 0, source = 0;
        float amount = 0.0f;
        int polarity = 0;

        bool operator== (const Assignment& other) const noexcept
        {
            return slot == other.slot && source == other.source && polarity == other.polarity && std::abs (amount - other.amount) < 1.0e-5f;
        }
    };

    /** A rotary knob. When its parameter can be modulated it takes a source dropped on it, shows
        a ring for every modulation with a dot at the value as it is right now, sets the depth
        with a shift-drag and opens a menu on a right click. */
    struct ModKnob final : public juce::Slider,
                           public juce::DragAndDropTarget
    {
        void paint (juce::Graphics&) override;
        void mouseDown (const juce::MouseEvent&) override;
        void mouseDrag (const juce::MouseEvent&) override;
        void mouseUp (const juce::MouseEvent&) override;
        bool isInterestedInDragSource (const SourceDetails&) override;
        void itemDragEnter (const SourceDetails&) override;
        void itemDragExit (const SourceDetails&) override;
        void itemDropped (const SourceDetails&) override;

        /** Where the knob stands, 0..1, with a modulation sum of `x` applied. */
        float proportionWith (float x) const;

        AMVSynthAudioProcessorEditor* owner = nullptr;
        juce::RangedAudioParameter* param = nullptr;
        int dest = -1;
        std::vector<Assignment> assignments;
        float live = 0.0f;       // sum of source times amount, as it is now
        bool showAsTarget = false, dropHover = false;

    private:
        bool depthDrag = false;
        float depthStart = 0.0f;
    };

    /** The row of modulation sources. A chip is dragged onto a knob to modulate it. */
    struct ModSourceBar final : public juce::Component
    {
        void paint (juce::Graphics&) override;
        void mouseDown (const juce::MouseEvent&) override;
        void mouseDrag (const juce::MouseEvent&) override;
        void mouseMove (const juce::MouseEvent&) override;
        void mouseExit (const juce::MouseEvent&) override;
        juce::Rectangle<float> chipBounds (int source) const;
        int chipAt (juce::Point<float>) const;

        AMVSynthAudioProcessorEditor* owner = nullptr;
        std::array<float, amv::mod::numSources> values {};
        int hover = 0, pressed = 0;
    };

    /** One cycle of the selected LFO with a marker where it is now. A Custom shape is edited
        here: points are dragged, a double click adds or removes one, the small handle between
        two points bends the line, and a right click offers shapes to start from. */
    struct LfoDisplay final : public juce::Component
    {
        void paint (juce::Graphics&) override;
        void mouseMove (const juce::MouseEvent&) override;
        void mouseExit (const juce::MouseEvent&) override;
        void mouseDown (const juce::MouseEvent&) override;
        void mouseDrag (const juce::MouseEvent&) override;
        void mouseUp (const juce::MouseEvent&) override;
        void mouseDoubleClick (const juce::MouseEvent&) override;

        /** Called from the editor's timer. */
        void refresh();

        /** Makes `newPoints` the Custom shape of the LFO and switches the LFO to it. */
        void setPoints (std::vector<amv::LfoPoint> newPoints, bool finished);

        AMVSynthAudioProcessorEditor* owner = nullptr;
        int lfo = 0;

    private:
        juce::Rectangle<float> graph() const;
        juce::Point<float> toScreen (float x, float y) const;
        juce::Point<float> handleFor (size_t segment) const;
        int pointAt (juce::Point<float>) const;
        int handleAt (juce::Point<float>) const;
        float valueAt (float phase) const;
        void showMenu();

        std::vector<amv::LfoPoint> points;
        const void* pointsFrom = nullptr;
        int shape = -1, mode = 0, shownLfo = -1;
        float phase = 0.0f;
        int hoverPoint = -1, hoverHandle = -1, dragPoint = -1, dragHandle = -1;
        float curveStart = 0.0f;
    };

    /** A panel of text. */
    struct TextDisplay final : public juce::Component
    {
        void paint (juce::Graphics&) override;
        juce::StringArray lines;
    };

    struct Knob
    {
        ModKnob slider;
        juce::Label label;
        std::unique_ptr<APVTS::SliderAttachment> attachment;
    };

    struct Toggle
    {
        juce::ToggleButton button;
        std::unique_ptr<APVTS::ButtonAttachment> attachment;
    };

    /** Draws the accent groups of the selected arp meter as bars. */
    struct PatternDisplay final : public juce::Component
    {
        void paint (juce::Graphics&) override;
        int meter = 0;
    };

    /** Draws the current shape of a wavetable oscillator. */
    struct WaveDisplay final : public juce::Component
    {
        void paint (juce::Graphics&) override;
        int table = -1;
        float position = 0.0f;
        bool active = false;
        const amv::UserWavetable* user = nullptr; // shown when the table is "User"
    };

    /** Stereo output level in decibels, with a slow fall and a clip light. */
    struct LevelMeter final : public juce::Component
    {
        void paint (juce::Graphics&) override;
        std::array<float, 2> level {}; // linear, already smoothed
        int clipHold = 0;
    };

    /** An ADSR envelope drawn as a curve. The three handles are dragged to set attack,
        decay with sustain, and release; a double click puts a handle back to its default. */
    struct EnvelopeDisplay final : public juce::Component
    {
        void setParameters (APVTS&, const char* attack, const char* decay, const char* sustain, const char* release);
        void paint (juce::Graphics&) override;
        void mouseMove (const juce::MouseEvent&) override;
        void mouseExit (const juce::MouseEvent&) override;
        void mouseDown (const juce::MouseEvent&) override;
        void mouseDrag (const juce::MouseEvent&) override;
        void mouseUp (const juce::MouseEvent&) override;
        void mouseDoubleClick (const juce::MouseEvent&) override;

        /** Repaints when a value was changed from outside, for example by a preset. */
        void refresh();

    private:
        enum Handle { none = -1, attackHandle, decayHandle, releaseHandle };

        bool showsValues() const { return getWidth() >= 230; }
        juce::Rectangle<float> graphArea() const;
        float segmentWidth() const;
        std::array<juce::Point<float>, 5> points() const;
        int handleAt (juce::Point<float>) const;
        void set (juce::RangedAudioParameter*, float normalised);

        std::array<juce::RangedAudioParameter*, 4> params {}; // attack, decay, sustain, release
        std::array<float, 4> shown { -1.0f, -1.0f, -1.0f, -1.0f };
        int hover = none, dragging = none;
    };

    struct Choice
    {
        juce::ComboBox box;
        std::unique_ptr<APVTS::ComboBoxAttachment> attachment;
    };

    /** A titled panel. Its controls are laid out left to right: an optional column of
        choices and toggles, then the knobs. */
    struct Section
    {
        juce::String title;
        bool balkan = false;
        int page = 0;
        int row = 0;
        float weight = 1.0f;
        std::vector<Knob*> knobs;
        std::vector<juce::Component*> column;
        int columnsInColumn = 1;
        int columnWidth = 0; // preferred width of a column cell, 0 for the default
        juce::Component* display = nullptr; // optional wide component between the column and the knobs
        float displayShare = 0.4f;          // part of the width the display takes when there are knobs too
        juce::Rectangle<int> bounds;
    };

    using ParamList = std::initializer_list<std::pair<juce::String, const char*>>;

    std::vector<Knob*> makeKnobs (bool balkan, ParamList params);
    Toggle* makeToggle (const juce::String& paramId, const juce::String& text, bool balkan);
    Choice* makeChoice (const juce::String& paramId, bool balkan);
    Section& addSection (const juce::String& title, bool balkan, int row, float weight);
    void layoutSection (Section&);
    void showModel (int index);
    void setPage (int page);
    void updateVisibility();
    void timerCallback() override;

    AMVSynthAudioProcessor& processor;
    amv::LookAndFeel lookAndFeel;

    std::vector<std::unique_ptr<Knob>> knobs;
    std::vector<std::unique_ptr<Toggle>> toggles;
    std::vector<std::unique_ptr<Choice>> choices;
    std::vector<Section> sections;

    std::vector<std::vector<Knob*>> modelKnobs;
    size_t engineSection = 0;
    int shownModel = -1;

    std::array<juce::TextButton, 8> tabs;
    static constexpr std::array<int, 8> tabPages { 0, 1, 5, 6, 7, 2, 3, 4 }; // the page each tab shows; WAVE, SAMPLE and SEQ were added last
    static constexpr int seqPage = 7;
    int currentPage = 0, buildingPage = 0;

    juce::ComboBox presetBox;
    juce::TextButton prevPreset { "<" }, nextPreset { ">" }, savePreset { "SAVE" }, browsePreset { "BROWSE" };
    std::unique_ptr<juce::FileChooser> fileChooser;
    int shownPreset = -1;
    void selectPreset (int index);
    void loadUserPreset (const juce::File&);
    void showBrowser (bool show);
    PresetBrowser browser;
    juce::File userPreset; // the preset file that is loaded, if any

    PatternDisplay patternDisplay;
    WaveDisplay waveA, waveB;
    LevelMeter levelMeter;
    EnvelopeDisplay ampEnvDisplay, filterEnvDisplay, env3Display;
    juce::TooltipWindow tooltipWindow { this };
    std::array<juce::TextButton, 2> importButtons;
    WavetableEditor waveEditor { processor };

    // SAMPLE page.
    enum SampleTool { toolLoad, toolNormalise, toolCrop, toolFade, toolFindRoot, toolToWaveA, toolToWaveB, toolClear, numSampleTools };
    SampleDisplay sampleDisplay { processor };
    std::array<juce::TextButton, numSampleTools> sampleButtons;
    TextDisplay sampleInfo;
    const amv::SampleData* infoFor = nullptr;
    bool infoShown = false;
    void runSampleTool (int tool);
    void updateSampleInfo();
    void setParameterValue (const char* paramId, float value);
    bool hasSourceBar() const noexcept { return currentPage < 4 || currentPage == 6; }

    // SEQ page.
    enum SeqTool { seqOneBar, seqTwoBars, seqAccents, seqRandom, seqShiftLeft, seqShiftRight, seqReverse, seqClear, seqCopy, seqPaste, numSeqTools };
    StepEditor stepEditor { processor };
    PatternBar patternBar { processor };
    std::array<juce::TextButton, numSeqTools> seqButtons;

    // Modulation by drag and drop.
    std::vector<Assignment> assignmentsFor (int dest) const;
    void assignModulation (int source, int dest);
    void setSlotAmount (int slot, float amount, bool gesture);
    void clearSlot (int slot);
    void showModMenu (ModKnob&);
    void updateModulationDisplay();
    struct SlotParams
    {
        juce::RangedAudioParameter* source;
        juce::RangedAudioParameter* dest;
        juce::RangedAudioParameter* amount;
        juce::RangedAudioParameter* polarity;
    };
    std::array<SlotParams, amv::mod::numSlots> slotParams {};
    ModSourceBar sourceBar;
    LfoDisplay lfoDisplay;
    std::array<juce::TextButton, amv::mod::numLfos> lfoSelect;
    std::array<std::vector<juce::Component*>, amv::mod::numLfos> lfoColumns;
    std::array<std::vector<Knob*>, amv::mod::numLfos> lfoKnobs;
    size_t lfoSection = 0;
    void showLfo (int index);
    TextDisplay lfoHelp;
    TextDisplay helpDisplay;
    juce::Label statusLabel;
    int statusTicks = 0;
    void showStatus (const juce::String&);
    juce::Slider tuneBox;
    std::unique_ptr<APVTS::SliderAttachment> tuneAttachment;
    juce::MidiKeyboardComponent keyboard;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AMVSynthAudioProcessorEditor)
};
