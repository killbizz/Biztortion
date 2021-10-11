/*
  ==============================================================================

    BitcrusherModule.h
    Created: 22 Sep 2021 10:28:48am
    Author:  gabri

  ==============================================================================
*/

/*
  ==============================================================================

    CREDITS for the original Bitcrusher Algorithm
    Author: Aaron Leese
    Source: https://youtu.be/1PLn8IAKEb4

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "DSPModule.h"
#include "GUIModule.h"
#include "../Component/GUIStuff.h"
class BiztortionAudioProcessor;

//==============================================================================

/* BitcrusherModule DSP */

//==============================================================================

struct BitcrusherSettings {
    float mix{ 0 }, rateRedux{ 0 }, bitRedux{ 0 }, dither{ 0 };
    bool bypassed{ false };
};


class BitcrusherModuleDSP : public DSPModule {
public:
    BitcrusherModuleDSP(juce::AudioProcessorValueTreeState& _apvts);

    Array<float> getWhiteNoise(int numSamples);

    void setModuleType() override;
    void updateDSPState(double sampleRate) override;
    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages, double sampleRate) override;

    static void addParameters(juce::AudioProcessorValueTreeState::ParameterLayout&);
    static BitcrusherSettings getSettings(juce::AudioProcessorValueTreeState& apvts, unsigned int chainPosition);

private:

    bool bypassed = false;
    juce::AudioBuffer<float> wetBuffer, noiseBuffer;
    juce::LinearSmoothedValue<float> dryGain, wetGain, dither;
    juce::LinearSmoothedValue<float> rateRedux, bitRedux;

};

//==============================================================================

/* BitcrusherModule GUI */

//==============================================================================

class BitcrusherModuleGUI : public GUIModule {
public:
    BitcrusherModuleGUI(BiztortionAudioProcessor& p, unsigned int chainPosition);
    ~BitcrusherModuleGUI();

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

    juce::Label bitcrusherMixLabel,
        bitcrusherDitherLabel,
        bitcrusherRateReduxLabel,
        bitcrusherBitReduxLabel;
    RotarySliderWithLabels bitcrusherMixSlider,
        bitcrusherDitherSlider,
        bitcrusherRateReduxSlider,
        bitcrusherBitReduxSlider;
    Attachment bitcrusherMixSliderAttachment,
        bitcrusherDitherSliderAttachment,
        bitcrusherRateReduxSliderAttachment,
        bitcrusherBitReduxSliderAttachment;

    PowerButton bypassButton;
    ButtonAttachment bypassButtonAttachment;
    ButtonsLookAndFeel lnf;

};