/*
  ==============================================================================

    BitcrusherModule.h
    Created: 22 Sep 2021 10:28:48am
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

/* BitcrusherModule DSP */

//==============================================================================

struct BitcrusherSettings {
    float mix{ 0 }, rateRedux{ 0 }, bitRedux{ 0 }, dither{ 0 };
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

    bool isActive = false;
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

    void paint(juce::Graphics& g) override;
    void resized() override;

    std::vector<juce::Component*> getComps() override;

private:

    using APVTS = juce::AudioProcessorValueTreeState;
    using Attachment = APVTS::SliderAttachment;

    // This reference is provided as a quick way for your editor to
    // access the processor object that created it.
    BiztortionAudioProcessor& audioProcessor;

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

};