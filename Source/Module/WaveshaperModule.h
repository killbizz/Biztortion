/*
  ==============================================================================

    WaveshaperModule.h
    Created: 26 Aug 2021 12:05:19pm
    Author:  gabri

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "DSPModule.h"
#include "GUIModule.h"
#include "../Component/GUIStuff.h"
#include "../Component/TransferFunctionGraphComponent.h"
class BiztortionAudioProcessor;

//==============================================================================

/* WaveshaperModule DSP */

//==============================================================================

struct WaveshaperSettings {
    float mix{ 0 }, drive{ 0 }, tanhAmp{ 0 }, tanhSlope{ 0 }, sinAmp{ 0 }, sinFreq{ 0 };
};

// TODO : implementing oversampling
//class Waveshaper : public juce::dsp::ProcessorBase
//{
//public:
//    Waveshaper()
//    {
//        // This is where the magic happens :D
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

    bool isActive = false;
    juce::AudioBuffer<float> wetBuffer;
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

    void paint(juce::Graphics& g) override;
    void resized() override;

    std::vector<juce::Component*> getComps() override;

private:

    using APVTS = juce::AudioProcessorValueTreeState;
    using Attachment = APVTS::SliderAttachment;

    // This reference is provided as a quick way for your editor to
    // access the processor object that created it.
    BiztortionAudioProcessor& audioProcessor;

    // waveshaperModule
    TransferFunctionGraphComponent transferFunctionGraph;
    // juce::LookAndFeel_V4 lookAndFeel1, lookAndFeel2, lookAndFeel3;
    juce::Label waveshaperDriveLabel,
        waveshaperMixLabel,
        tanhAmpLabel,
        tanhSlopeLabel,
        sineAmpLabel,
        sineFreqLabel;
    RotarySliderWithLabels waveshaperDriveSlider,
        waveshaperMixSlider,
        tanhAmpSlider,
        tanhSlopeSlider,
        sineAmpSlider,
        sineFreqSlider;
    Attachment waveshaperDriveSliderAttachment,
        waveshaperMixSliderAttachment,
        tanhAmpSliderAttachment,
        tanhSlopeSliderAttachment,
        sineAmpSliderAttachment,
        sineFreqSliderAttachment;

};