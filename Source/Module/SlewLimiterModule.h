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
#include "DSPModule.h"
#include "GUIModule.h"
#include "../Shared/GUIStuff.h"
class BiztortionAudioProcessor;

//==============================================================================

/* SlewLimiterModule DSP */

//==============================================================================

struct SlewLimiterSettings {
    float symmetry{ 0 }, bias{ 0 }, drive{ 0 }, mix{ 0 };
    float rise{ 0 }, fall{ 0 };
    bool bypassed{ false }, DCoffsetRemove{ false };
};

using Filter = juce::dsp::IIR::Filter<float>;


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
    juce::LinearSmoothedValue<float> symmetry, bias;
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

    juce::ToggleButton DCoffsetEnabledButton;
    ButtonAttachment DCoffsetEnabledButtonAttachment;
    juce::Label DCoffsetEnabledButtonLabel;

    juce::Label driveLabel,
        mixLabel,
        symmetryLabel,
        biasLabel,
        slewLimiterRiseLabel,
        slewLimiterFallLabel;
    RotarySliderWithLabels driveSlider,
        mixSlider,
        symmetrySlider,
        biasSlider,
        slewLimiterRiseSlider,
        slewLimiterFallSlider;
    Attachment driveSliderAttachment,
        mixSliderAttachment,
        symmetrySliderAttachment,
        biasSliderAttachment,
        slewLimiterRiseSliderAttachment,
        slewLimiterFallSliderAttachment;

};