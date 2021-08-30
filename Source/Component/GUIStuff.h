/*
  ==============================================================================

    GUIStuff.h
    Created: 29 Aug 2021 9:54:43pm
    Author:  gabri

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>

struct LookAndFeel : juce::LookAndFeel_V4
{
    void drawRotarySlider(juce::Graphics&,
        int x, int y, int width, int height,
        float sliderPosProportional,
        float rotaryStartAngle,
        float rotaryEndAngle,
        juce::Slider&) override;

    /*void drawToggleButton(juce::Graphics& g,
        juce::ToggleButton& toggleButton,
        bool shouldDrawButtonAsHighlighted,
        bool shouldDrawButtonAsDown) override;*/
};

struct LabelWithPosition
{
    float pos;
    juce::String label;
};

struct RotarySliderWithLabels : juce::Slider
{
    RotarySliderWithLabels(juce::RangedAudioParameter& rap, const juce::String& unitSuffix) :
        juce::Slider(juce::Slider::SliderStyle::RotaryHorizontalVerticalDrag,
            juce::Slider::TextEntryBoxPosition::NoTextBox),
        param(&rap),
        suffix(unitSuffix)
    {
        setLookAndFeel(&lnf);
    }

    ~RotarySliderWithLabels()
    {
        setLookAndFeel(nullptr);
    }

    juce::Array<LabelWithPosition> labels;

    void paint(juce::Graphics& g) override;
    juce::Rectangle<int> getSliderBounds() const;
    int getTextHeight() const { return 14; }
    juce::String getDisplayString() const;
private:

    LookAndFeel lnf;
    juce::RangedAudioParameter* param;
    juce::String suffix;
};