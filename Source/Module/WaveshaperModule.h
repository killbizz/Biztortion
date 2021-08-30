/*
  ==============================================================================

    WaveshaperModule.h
    Created: 26 Aug 2021 12:05:19pm
    Author:  gabri

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>

//==============================================================================

/* WaveshaperModule DSP */

//==============================================================================

struct WaveshaperSettings {
    float mix{ 0 }, drive{ 0 }, tanhAmp{ 0 }, tanhSlope{ 0 }, sinAmp{ 0 }, sinFreq{ 0 };
};

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

class WaveshaperModule {
public:
    WaveshaperModule(juce::AudioProcessorValueTreeState&);
    void prepareToPlay(double sampleRate, int samplesPerBlock);
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&, double);

    static void addParameters(juce::AudioProcessorValueTreeState::ParameterLayout&);
    static WaveshaperSettings getSettings(juce::AudioProcessorValueTreeState& apvts);

private:
    juce::AudioProcessorValueTreeState& apvts;

    bool isActive = false;
    juce::AudioBuffer<float> wetBuffer;
    juce::LinearSmoothedValue<float> driveGain, dryGain, wetGain;
    juce::LinearSmoothedValue<float> tanhAmp, tanhSlope, sineAmp, sineFreq;

    /*static const size_t numChannels = 2;
    static const size_t oversamplingOrder = 4;
    static const int    oversamplingFactor = 1 << oversamplingOrder;
    static const auto filterType = juce::dsp::Oversampling<float>::filterHalfBandPolyphaseIIR;

    juce::dsp::Oversampling<float> oversampler{ numChannels, oversamplingOrder, filterType };*/

    void updateDSP();
};

//==============================================================================

/* WaveshaperModule GUI */

//==============================================================================