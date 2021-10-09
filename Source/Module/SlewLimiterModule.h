/*
  ==============================================================================

    SlewLimiterModule.h
    Created: 8 Oct 2021 3:54:07pm
    Author:  gabri

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "DSPModule.h"
#include "GUIModule.h"
#include "../Component/GUIStuff.h"
class BiztortionAudioProcessor;

//==============================================================================

/* SlewLimiterModule DSP */

//==============================================================================

struct SlewLimiterSettings {
    float rise{ 0 }, fall{ 0 };
    bool bypassed{ false };
};


class SlewLimiterModuleDSP : public DSPModule {
public:
    SlewLimiterModuleDSP(juce::AudioProcessorValueTreeState& _apvts);

    void setModuleType() override;
    void updateDSPState(double sampleRate) override;
    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages, double sampleRate) override;

    static void addParameters(juce::AudioProcessorValueTreeState::ParameterLayout&);
    static SlewLimiterSettings getSettings(juce::AudioProcessorValueTreeState& apvts, unsigned int chainPosition);

private:

    bool bypassed = false;
    juce::LinearSmoothedValue<float> rise, fall;
    juce::AudioBuffer<float> wetBuffer;

    float slewMin = 0.1f;
    float slewMax = 1000.f;
    float lastOutput = 0.f;

};

//==============================================================================

/* SlewLimiterModule GUI */

//==============================================================================

class SlewLimiterModuleGUI : public GUIModule {
public:
    SlewLimiterModuleGUI(BiztortionAudioProcessor& p, unsigned int chainPosition);
    ~SlewLimiterModuleGUI();

    void paint(juce::Graphics& g) override;
    void resized() override;

    std::vector<juce::Component*> getComps() override;

private:

    using APVTS = juce::AudioProcessorValueTreeState;
    using Attachment = APVTS::SliderAttachment;
    using ButtonAttachment = APVTS::ButtonAttachment;

    // This reference is provided as a quick way for your editor to
    // access the processor object that created it.
    BiztortionAudioProcessor& audioProcessor;
    juce::Label title;

    PowerButton bypassButton;
    ButtonAttachment bypassButtonAttachment;
    ButtonsLookAndFeel lnf;

    juce::Label slewLimiterRiseLabel,
        slewLimiterFallLabel;
    RotarySliderWithLabels slewLimiterRiseSlider,
        slewLimiterFallSlider;
    Attachment slewLimiterRiseSliderAttachment,
        slewLimiterFallSliderAttachment;

};