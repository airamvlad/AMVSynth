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
                                           private juce::Timer
{
public:
    explicit AMVSynthAudioProcessorEditor (AMVSynthAudioProcessor&);
    ~AMVSynthAudioProcessorEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    using APVTS = juce::AudioProcessorValueTreeState;

    struct Knob
    {
        juce::Slider slider;
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

    std::array<juce::TextButton, 4> tabs;
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
    juce::Slider tuneBox;
    std::unique_ptr<APVTS::SliderAttachment> tuneAttachment;
    juce::MidiKeyboardComponent keyboard;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AMVSynthAudioProcessorEditor)
};
