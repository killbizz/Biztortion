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

#include "../../Component/WaveShapeVisualiser.h"
#include "../../Shared/GUIStuff.h"
#include "../../Shared/SharedParamsLayout.h"
#include "../../Module/DSPModule.h"
#include "../../Module/GUIModule.h"
#include "../../Shared/PluginState.h"
#include "SpectrumBitcrusherProcessor.h"

//==============================================================================

/* SpectrumBitcrusherModule DSP */

//==============================================================================

struct SpectrumBitcrusherSettings {
    float mix{ 0 }, drive{ 0 }, fxDistribution{ 0 }, bias{ 0 }, symmetry{ 0 };
    float rateRedux{ 0 }, magnitudeBits{ 0 }, spectralTilt{ 0 };
    float robotisation{ 0 }, robotPitch{ 0 };
    bool bypassed{ false }, DCoffsetRemove{ false };
};

const juce::String SPECTRUM_BITCRUSHER_ID = "SBitcrusher ";

using Filter = juce::dsp::IIR::Filter<float>;

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
    bool spectrumProcessingEnabled = false;
    juce::AudioBuffer<float> wetBuffer, tempBuffer;
    juce::LinearSmoothedValue<float> fxDistribution, bias, symmetry;
    juce::LinearSmoothedValue<float> driveGain, dryGain, wetGain;
    juce::LinearSmoothedValue<float> rateRedux, magnitudeBits, spectralTilt;
    juce::LinearSmoothedValue<float> robotisation, robotPitch;

    double currentSampleRate = 44100.0;
    double robotPhase = 0.0;
    std::vector<float> magnitudeScratch;

    Filter leftDCoffsetRemoveHPF, rightDCoffsetRemoveHPF;
    bool DCoffsetRemoveEnabled = false;

    std::function<void(juce::AudioSampleBuffer&)> spectrumProcessingLambda;

    SpectrumBitcrusherProcessor spectrumProcessor;

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
    unsigned int parameterNumber;

    juce::Label title;

    // left: what this module is doing to the spectrum; centre of the hexagon: what the shared
    // drive/mix/distribution/bias/symmetry/dc parameters are doing
    std::unique_ptr<WaveShapeVisualiser> moduleGraph;
    std::unique_ptr<WaveShapeVisualiser> sharedParamsGraph;

    juce::Label driveLabel,
        mixLabel,
        fxDistributionLabel,
        biasLabel,
        symmetryLabel,
        binsReduxLabel,
        magnitudeBitsLabel,
        spectralTiltLabel,
        robotisationLabel,
        robotPitchLabel;
    RotarySliderWithLabels driveSlider,
        mixSlider,
        fxDistributionSlider,
        biasSlider,
        symmetrySlider,
        binsReduxSlider,
        magnitudeBitsSlider,
        spectralTiltSlider,
        robotisationSlider,
        robotPitchSlider;
    Attachment driveSliderAttachment,
        mixSliderAttachment,
        fxDistributionSliderAttachment,
        biasSliderAttachment,
        symmetrySliderAttachment,
        binsReduxSliderAttachment,
        magnitudeBitsSliderAttachment,
        spectralTiltSliderAttachment,
        robotisationSliderAttachment,
        robotPitchSliderAttachment;

    PowerButton bypassButton;

    ButtonAttachment bypassButtonAttachment;

    ButtonsLookAndFeel lnf;

    juce::ToggleButton DCoffsetEnabledButton;

    ButtonAttachment DCoffsetEnabledButtonAttachment;

    ButtonsLookAndFeel dcOffsetLnf;

    juce::Label DCoffsetEnabledButtonLabel;

};