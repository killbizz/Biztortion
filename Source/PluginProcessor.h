/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "Module/MeterModule.h"
#include "Module/FilterModule.h"
#include "Module/WaveshaperModule.h"
#include "Component/ResponseCurveComponent.h"
#include "Component/FFTAnalyzerComponent.h"

//==============================================================================
/**
*/
class BiztortionAudioProcessor  : public juce::AudioProcessor
{
public:
    //==============================================================================
    BiztortionAudioProcessor();
    ~BiztortionAudioProcessor() override;

    //==============================================================================
    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

   #ifndef JucePlugin_PreferredChannelConfigurations
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
   #endif

    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    //==============================================================================
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    void getStateInformation(juce::MemoryBlock& destData) override;
    void setStateInformation(const void* data, int sizeInBytes) override;

    //==============================================================================
    const juce::String getName() const override;

    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;

    //==============================================================================
    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int index, const juce::String& newName) override;

    //==============================================================================

    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

    // APVTS
    juce::AudioProcessorValueTreeState apvts{
      *this, nullptr, "Parameters", createParameterLayout()
    };
    // oscilloscope
    drow::AudioOscilloscope oscilloscope;
    // fft analyzers
    using BlockType = juce::AudioBuffer<float>;
    SingleChannelSampleFifo<BlockType> preLeftChannelFifo{ Channel::Left };
    SingleChannelSampleFifo<BlockType> preRightChannelFifo{ Channel::Right };
    SingleChannelSampleFifo<BlockType> postLeftChannelFifo{ Channel::Left };
    SingleChannelSampleFifo<BlockType> postRightChannelFifo{ Channel::Right };
    // mid analyzer allocated only if the relative module is istantiated
    SingleChannelSampleFifo<BlockType>* midLeftChannelFifo = nullptr;
    SingleChannelSampleFifo<BlockType>* midRightChannelFifo = nullptr;
    // modules
    std::vector<std::unique_ptr<DSPModule>> DSPmodules;

    void updateModulesChain(juce::String moduleName, unsigned int gridPosition);
    int getNumSamples();

private:

    // test signal
    // juce::dsp::Oscillator<float> osc;
    static juce::Identifier modulesChainID;
    juce::Value modulesChain;
    int numSamples;

    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (BiztortionAudioProcessor)
};