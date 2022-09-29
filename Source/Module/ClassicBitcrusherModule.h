/*
  ==============================================================================

    TimeBitcrusherModule.h

    Copyright (c) 2021 KillBizz - Gabriel Bizzo

  ==============================================================================
*/

/*
  ==============================================================================

    Copyright (c) 2018 Joshua Hodge
    Content: the original Time-Domain Bitcrusher Algorithm
    Source: https://github.com/theaudioprogrammer/bitcrusherDemo

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

/* ClassicBitcrusherModule DSP */

//==============================================================================

struct ClassicBitcrusherSettings {
    float mix{ 0 }, drive{ 0 }, fxDistribution{ 0 }, bias{ 0 }, symmetry{ 0 };
    float rateRedux{ 0 }, bitRedux{ 0 }, dither{ 0 };
    bool bypassed{ false };
};

const juce::String CLASSIC_BITCRUSHER_ID = "CBitcrusher ";


class ClassicBitcrusherModuleDSP : public DSPModule {
public:
    ClassicBitcrusherModuleDSP(juce::AudioProcessorValueTreeState& _apvts);

    Array<float> getWhiteNoise(int numSamples);

    void updateDSPState(double sampleRate) override;
    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages, double sampleRate) override;

    static void addParameters(juce::AudioProcessorValueTreeState::ParameterLayout&);
    static ClassicBitcrusherSettings getSettings(juce::AudioProcessorValueTreeState& apvts, unsigned int parameterNumber);

private:

    bool bypassed = false;
    juce::AudioBuffer<float> wetBuffer, noiseBuffer, tempBuffer;
    juce::LinearSmoothedValue<float> fxDistribution, bias, symmetry;
    juce::LinearSmoothedValue<float> driveGain, dryGain, wetGain, dither;
    juce::LinearSmoothedValue<float> rateRedux, bitRedux;

};

//==============================================================================

/* ClassicBitcrusherModule GUI */

//==============================================================================

class ClassicBitcrusherModuleGUI : public GUIModule {
public:
    ClassicBitcrusherModuleGUI(PluginState& p, unsigned int parameterNumber);
    ~ClassicBitcrusherModuleGUI();

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
        fxDistributionLabel,
        biasLabel,
        symmetryLabel,
        bitcrusherDitherLabel,
        bitcrusherRateReduxLabel,
        bitcrusherBitReduxLabel;
    RotarySliderWithLabels driveSlider,
        mixSlider,
        fxDistributionSlider,
        biasSlider,
        symmetrySlider,
        bitcrusherDitherSlider,
        bitcrusherRateReduxSlider,
        bitcrusherBitReduxSlider;
    Attachment driveSliderAttachment,
        mixSliderAttachment,
        fxDistributionSliderAttachment,
        biasSliderAttachment,
        symmetrySliderAttachment,
        bitcrusherDitherSliderAttachment,
        bitcrusherRateReduxSliderAttachment,
        bitcrusherBitReduxSliderAttachment;

    PowerButton bypassButton;

    ButtonAttachment bypassButtonAttachment;

    ButtonsLookAndFeel lnf;

};