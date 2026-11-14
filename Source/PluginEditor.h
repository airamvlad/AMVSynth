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
};
} // namespace amv

class AMVSynthAudioProcessorEditor final : public juce::AudioProcessorEditor
{
public:
    explicit AMVSynthAudioProcessorEditor (AMVSynthAudioProcessor&);
    ~AMVSynthAudioProcessorEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    struct Knob
    {
        juce::Slider slider;
        juce::Label label;
        std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> attachment;
    };

    struct Section
    {
        juce::String title;
        bool balkan = false;
        int row = 0;
        float weight = 1.0f;
        std::vector<Knob*> knobs;
        juce::Rectangle<int> bounds;
    };

    Section& addSection (const juce::String& title, bool balkan, int row,
                         std::initializer_list<std::pair<const char*, const char*>> params);

    AMVSynthAudioProcessor& processor;
    amv::LookAndFeel lookAndFeel;

    std::vector<std::unique_ptr<Knob>> knobs;
    std::vector<Section> sections;

    juce::ToggleButton monoButton { "MONO LEGATO" };
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> monoAttachment;
    juce::MidiKeyboardComponent keyboard;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AMVSynthAudioProcessorEditor)
};
