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
        int row = 0;
        float weight = 1.0f;
        std::vector<Knob*> knobs;
        std::vector<juce::Component*> column;
        int columnsInColumn = 1;
        juce::Rectangle<int> bounds;
    };

    using ParamList = std::initializer_list<std::pair<const char*, const char*>>;

    std::vector<Knob*> makeKnobs (bool balkan, ParamList params);
    Toggle* makeToggle (const char* paramId, const juce::String& text, bool balkan);
    Choice* makeChoice (const char* paramId, bool balkan);
    Section& addSection (const juce::String& title, bool balkan, int row, float weight);
    void layoutSection (Section&);
    void showModel (int index);
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

    juce::MidiKeyboardComponent keyboard;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AMVSynthAudioProcessorEditor)
};
