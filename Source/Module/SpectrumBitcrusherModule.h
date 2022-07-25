/*
  ==============================================================================

    SpectrumBitcrusherModule.h

    Copyright (c) 2022 KillBizz - Gabriel Bizzo

  ==============================================================================
*/

/*

This file is part of Biztortion software.

Biztortion is free software : you can redistribute it and /or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

Biztortion is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Biztortion. If not, see < http://www.gnu.org/licenses/>.

*/

#pragma once

#include <JuceHeader.h>

#include "../Shared/GUIStuff.h"
#include "../Module/DSPModule.h"
#include "../Module/GUIModule.h"
#include "../Shared/PluginState.h"

//==============================================================================

/* SpectrumBitcrusherModule DSP */

//==============================================================================

struct SpectrumBitcrusherSettings {
    float mix{ 0 }, drive{ 0 };
    float symmetry{ 0 }, bias{ 0 };
    float rateRedux{ 0 }, bitRedux{ 0 };
    bool bypassed{ false };
};

const juce::String SPECTRUM_BITCRUSHER_ID = "SBitcrusher ";


class SpectrumBitcrusherModuleDSP : public DSPModule {
public:
    SpectrumBitcrusherModuleDSP(juce::AudioProcessorValueTreeState& _apvts);

    void updateDSPState(double sampleRate) override;
    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages, double sampleRate) override;

    static void addParameters(juce::AudioProcessorValueTreeState::ParameterLayout&);
    static SpectrumBitcrusherSettings getSettings(juce::AudioProcessorValueTreeState& apvts, unsigned int parameterNumber);

private:

    // parameters
    bool bypassed = false;
    juce::AudioBuffer<float> wetBuffer, tempBuffer;
    juce::LinearSmoothedValue<float> symmetry, bias;
    juce::LinearSmoothedValue<float> driveGain, dryGain, wetGain;
    juce::LinearSmoothedValue<float> rateRedux, bitRedux;

    // FFT elaboration stuff
    SingleChannelSampleFifo<juce::AudioBuffer<float>> leftChannelSampleFifo { Channel::Left };
    SingleChannelSampleFifo<juce::AudioBuffer<float>> rightChannelSampleFifo { Channel::Right };
    juce::AudioBuffer<float> leftAudioBuffer;
    juce::AudioBuffer<float> rightAudioBuffer;
    FFTDataGenerator<std::vector<float>> leftChannelFFTDataGenerator;
    FFTDataGenerator<std::vector<float>> rightChannelFFTDataGenerator;
    AudioDataGenerator<std::vector<float>> leftChannelAudioDataGenerator;
    AudioDataGenerator<std::vector<float>> rightChannelAudioDataGenerator;
    std::vector<float> leftFFTBuffer;
    std::vector<float> rightFFTBuffer;

};

//==============================================================================

/* SpectrumBitcrusherModule GUI */

//==============================================================================

class SpectrumBitcrusherModuleGUI : public GUIModule {
public:
    SpectrumBitcrusherModuleGUI(PluginState& p, unsigned int parameterNumber);
    ~SpectrumBitcrusherModuleGUI();

    std::vector<juce::Component*> getAllComps() override;
    std::vector<juce::Component*> getParamComps() override;
    virtual void updateParameters(const juce::Array<juce::var>& values) override;
    virtual void resetParameters(unsigned int parameterNumber) override;
    virtual juce::Array<juce::var> getParamValues() override;

    void resized() override;

private:

    using APVTS = juce::AudioProcessorValueTreeState;
    using Attachment = APVTS::SliderAttachment;
    using ButtonAttachment = APVTS::ButtonAttachment;

    PluginState& pluginState;

    juce::Label title;

    juce::Label driveLabel,
        mixLabel,
        symmetryLabel,
        biasLabel,
        bitcrusherRateReduxLabel,
        bitcrusherBitReduxLabel;
    RotarySliderWithLabels driveSlider,
        mixSlider,
        symmetrySlider,
        biasSlider,
        bitcrusherRateReduxSlider,
        bitcrusherBitReduxSlider;
    Attachment driveSliderAttachment,
        mixSliderAttachment,
        symmetrySliderAttachment,
        biasSliderAttachment,
        bitcrusherRateReduxSliderAttachment,
        bitcrusherBitReduxSliderAttachment;

    PowerButton bypassButton;

    ButtonAttachment bypassButtonAttachment;

    ButtonsLookAndFeel lnf;

};