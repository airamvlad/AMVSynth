#pragma once

#include "PluginProcessor.h"

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
                                                              0xfff06292, 0xffffd54f, 0xff4dd0e1, 0xffaed581, 0xffe57373, 0xff9fa8da };
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

        bool operator== (const Assignment& other) const noexcept
        {
            return slot == other.slot && source == other.source && std::abs (amount - other.amount) < 1.0e-5f;
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

    /** The shape of an LFO with a marker at its current value. */
    struct LfoDisplay final : public juce::Component
    {
        void paint (juce::Graphics&) override;
        int shape = 0, source = amv::mod::srcLfo1;
        float value = 0.0f;
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

    std::array<juce::TextButton, 5> tabs;
    int currentPage = 0, buildingPage = 0;

    juce::ComboBox presetBox;
    juce::TextButton prevPreset { "<" }, nextPreset { ">" }, savePreset { "SAVE" }, loadPreset { "LOAD" };
    std::unique_ptr<juce::FileChooser> fileChooser;
    int shownPreset = -1;
    void selectPreset (int index);

    PatternDisplay patternDisplay;
    WaveDisplay waveA, waveB;
    LevelMeter levelMeter;
    EnvelopeDisplay ampEnvDisplay, filterEnvDisplay;
    juce::TooltipWindow tooltipWindow { this };
    std::array<juce::TextButton, 2> importButtons;
    void importWavetable (int osc);

    // Modulation by drag and drop.
    std::vector<Assignment> assignmentsFor (int dest) const;
    void assignModulation (int source, int dest);
    void setSlotAmount (int slot, float amount, bool gesture);
    void clearSlot (int slot);
    void showModMenu (ModKnob&);
    void updateModulationDisplay();
    struct SlotParams { juce::RangedAudioParameter* source; juce::RangedAudioParameter* dest; juce::RangedAudioParameter* amount; };
    std::array<SlotParams, amv::mod::numSlots> slotParams {};
    ModSourceBar sourceBar;
    std::array<LfoDisplay, amv::mod::numLfos> lfoDisplays;
    TextDisplay helpDisplay;
    juce::Label statusLabel;
    int statusTicks = 0;
    void showStatus (const juce::String&);
    juce::Slider tuneBox;
    std::unique_ptr<APVTS::SliderAttachment> tuneAttachment;
    juce::MidiKeyboardComponent keyboard;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AMVSynthAudioProcessorEditor)
};
