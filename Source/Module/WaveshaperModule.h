/*
  ==============================================================================

    WaveshaperModule.h

    Copyright (c) 2021 KillBizz - Gabriel Bizzo

  ==============================================================================
*/

/*
  ==============================================================================

    Copyright (c) 2018 Daniele Filaretti
    Content: the original Waveshaper Algorithm
    Source: https://github.com/dfilaretti/waveshaper-demo

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
#include "../Component/TransferFunctionGraphComponent.h"
class BiztortionAudioProcessor;

//==============================================================================

/* WaveshaperModule DSP */

//==============================================================================

struct WaveshaperSettings {
    float mix{ 0 }, drive{ 0 }, symmetry{ 0 }, bias{ 0 };
    float tanhAmp{ 0 }, tanhSlope{ 0 }, sinAmp{ 0 }, sinFreq{ 0 };
    bool bypassed{ false };
};

// TODO : implementing oversampling
//class Waveshaper : public juce::dsp::ProcessorBase
//{
//public:
//    Waveshaper()
//    {
//        
//        waveshaper.functionToUse = [](float in)
//        {
//            float out = tanhAmp.getNextValue() * std::tanh(in * tanhSlope.getNextValue())
//                + sineAmp.getNextValue() * std::sin(in * sineFreq.getNextValue());
//
//            return out;
//        };
//    }
//
//    void prepare(const juce::dsp::ProcessSpec& spec) override
//    {
//        oversampler.initProcessing(spec.maximumBlockSize);
//    }
//
//    void process(const juce::dsp::ProcessContextReplacing<float>& context) override
//    {
//        // First sample up...
//        auto oversampledBlock = oversampler.processSamplesUp(context.getInputBlock());
//        // Then process with the waveshaper...
//        waveshaper.process(juce::dsp::ProcessContextReplacing<float>(oversampledBlock));
//        // Finally sample back down
//        oversampler.processSamplesDown(context.getOutputBlock());
//    }
//
//    void reset() override { oversampler.reset(); }
//
//    float getLatencyInSamples() { return oversampler.getLatencyInSamples(); }
//
//private:
//    static const size_t numChannels = 1;
//    static const size_t oversamplingOrder = 4;
//    static const int    oversamplingFactor = 1 << oversamplingOrder;
//    static const auto filterType = juce::dsp::Oversampling<float>::filterHalfBandPolyphaseIIR;
//
//    juce::dsp::Oversampling<float> oversampler{ numChannels, oversamplingOrder, filterType };
//    juce::dsp::WaveShaper<float> waveshaper;
//};

class WaveshaperModuleDSP : public DSPModule {
public:
    WaveshaperModuleDSP(juce::AudioProcessorValueTreeState& _apvts);

    void setModuleType() override;

    void updateDSPState(double sampleRate) override;
    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages, double sampleRate) override;

    static void addParameters(juce::AudioProcessorValueTreeState::ParameterLayout&);
    static WaveshaperSettings getSettings(juce::AudioProcessorValueTreeState& apvts, unsigned int chainPosition);

private:

    bool bypassed = false;
    juce::AudioBuffer<float> wetBuffer, tempBuffer;
    juce::LinearSmoothedValue<float> symmetry, bias;
    juce::LinearSmoothedValue<float> driveGain, dryGain, wetGain;
    juce::LinearSmoothedValue<float> tanhAmp, tanhSlope, sineAmp, sineFreq;

    /*static const size_t numChannels = 2;
    static const size_t oversamplingOrder = 4;
    static const int    oversamplingFactor = 1 << oversamplingOrder;
    static const auto filterType = juce::dsp::Oversampling<float>::filterHalfBandPolyphaseIIR;

    juce::dsp::Oversampling<float> oversampler{ numChannels, oversamplingOrder, filterType };*/
};

//==============================================================================

/* WaveshaperModule GUI */

//==============================================================================

class WaveshaperModuleGUI : public GUIModule {
public:
    WaveshaperModuleGUI(BiztortionAudioProcessor& p, unsigned int chainPosition);
    ~WaveshaperModuleGUI();

    void paint(juce::Graphics& g) override;
    void resized() override;

    std::vector<juce::Component*> getAllComps() override;
    std::vector<juce::Component*> getParamComps() override;

private:

    using APVTS = juce::AudioProcessorValueTreeState;
    using Attachment = APVTS::SliderAttachment;
    using ButtonAttachment = APVTS::ButtonAttachment;

    // This reference is provided as a quick way for your editor to
    // access the processor object that created it.
    BiztortionAudioProcessor& audioProcessor;
    juce::Label title;

    TransferFunctionGraphComponent transferFunctionGraph;
    // juce::LookAndFeel_V4 lookAndFeel1, lookAndFeel2, lookAndFeel3;
    juce::Label driveLabel,
        mixLabel,
        symmetryLabel,
        biasLabel,
        tanhAmpLabel,
        tanhSlopeLabel,
        sineAmpLabel,
        sineFreqLabel;
    RotarySliderWithLabels driveSlider,
        mixSlider,
        symmetrySlider,
        biasSlider,
        tanhAmpSlider,
        tanhSlopeSlider,
        sineAmpSlider,
        sineFreqSlider;
    Attachment driveSliderAttachment,
        mixSliderAttachment,
        symmetrySliderAttachment,
        biasSliderAttachment,
        tanhAmpSliderAttachment,
        tanhSlopeSliderAttachment,
        sineAmpSliderAttachment,
        sineFreqSliderAttachment;

    PowerButton bypassButton;
    ButtonAttachment bypassButtonAttachment;
    ButtonsLookAndFeel lnf;

};