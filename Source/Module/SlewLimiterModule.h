/*
  ==============================================================================

    SlewLimiterModule.h

    Copyright (c) 2021 KillBizz - Gabriel Bizzo

  ==============================================================================
*/

/*
  ==============================================================================

    Copyright (c) 2017 Ivan Cohen
    Content: the original Slew Limiter Algorithm
    Source: https://www.dropbox.com/s/cjq4t08u6pqkaas/ADC17FiftyShadesDistortion.zip?dl=0

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

/* SlewLimiterModule DSP */

//==============================================================================

struct SlewLimiterSettings {
    float drive{ 0 }, mix{ 0 }, fxDistribution{ 0 }, bias{ 0 }, symmetry{ 0 };
    float rise{ 0 }, fall{ 0 };
    bool bypassed{ false }, DCoffsetRemove{ false };
};

using Filter = juce::dsp::IIR::Filter<float>;


class SlewLimiterModuleDSP : public DSPModule {
public:
    SlewLimiterModuleDSP(juce::AudioProcessorValueTreeState& _apvts);

    void updateDSPState(double sampleRate) override;
    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages, double sampleRate) override;

    static void addParameters(juce::AudioProcessorValueTreeState::ParameterLayout&);
    static SlewLimiterSettings getSettings(juce::AudioProcessorValueTreeState& apvts, unsigned int parameterNumber);

private:

    bool bypassed = false;
    juce::LinearSmoothedValue<float> fxDistribution, bias, symmetry;
    juce::LinearSmoothedValue<float> driveGain, dryGain, wetGain;
    juce::LinearSmoothedValue<float> rise, fall;
    juce::AudioBuffer<float> wetBuffer, tempBuffer;
    Filter leftDCoffsetRemoveHPF, rightDCoffsetRemoveHPF;
    bool DCoffsetRemoveEnabled = false;

    // minimum slope in volts per second
    const float slewMin = 0.1f;
    // maximum slope in volts per second
    const float slewMax = 10000.f;
    float lastOutput = 0.f;

};

//==============================================================================

/* SlewLimiterModule GUI */

//==============================================================================

class SlewLimiterModuleGUI : public GUIModule {
public:
    SlewLimiterModuleGUI(PluginState& p, unsigned int parameterNumber);
    ~SlewLimiterModuleGUI();

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

    PowerButton bypassButton;

    ButtonAttachment bypassButtonAttachment;

    ButtonsLookAndFeel lnf;

    juce::ToggleButton DCoffsetEnabledButton;

    ButtonAttachment DCoffsetEnabledButtonAttachment;

    ButtonsLookAndFeel dcOffsetLnf;

    juce::Label DCoffsetEnabledButtonLabel;

    juce::Label driveLabel,
        mixLabel,
        fxDistributionLabel,
        biasLabel,
        symmetryLabel,
        slewLimiterRiseLabel,
        slewLimiterFallLabel;
    RotarySliderWithLabels driveSlider,
        mixSlider,
        fxDistributionSlider,
        biasSlider,
        symmetrySlider,
        slewLimiterRiseSlider,
        slewLimiterFallSlider;
    Attachment driveSliderAttachment,
        mixSliderAttachment,
        fxDistributionSliderAttachment,
        biasSliderAttachment,
        symmetrySliderAttachment,
        slewLimiterRiseSliderAttachment,
        slewLimiterFallSliderAttachment;

};