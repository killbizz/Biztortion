/*
  ==============================================================================

    PluginProcessor.h

    Copyright (c) 2021 KillBizz - Gabriel Bizzo

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
#include "Module/BitcrusherModule.h"
#include "Module/SlewLimiterModule.h"
#include "Module/OscilloscopeModule.h"
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
    // fft analyzers
    using BlockType = juce::AudioBuffer<float>;
    // analyzer FIFO allocated only if the relative module is istantiated
    std::vector<SingleChannelSampleFifo<BlockType>*> leftAnalyzerFIFOs;
    std::vector<SingleChannelSampleFifo<BlockType>*> rightAnalyzerFIFOs;
    // modules
    std::vector<std::unique_ptr<DSPModule>> DSPmodules;

    DSPModule* createDSPModule(ModuleType mt);
    void addModuleToDSPmodules(DSPModule* module, unsigned int chainPosition);
    void addAndSetupModuleForDSP(DSPModule* module, unsigned int chainPosition);
    void addDSPmoduleTypeAndPositionToAPVTS(ModuleType mt, unsigned int chainPosition);
    void removeModuleFromDSPmodules(unsigned int chainPosition);
    void removeDSPmoduleTypeAndPositionFromAPVTS(unsigned int chainPosition);
    unsigned int getFftAnalyzerFifoIndexOfCorrespondingFilter(unsigned int chainPosition);
    void insertNewAnalyzerFIFO(unsigned int chainPosition);
    void deleteOldAnalyzerFIFO(unsigned int chainPosition);

private:

    // test signal
    // juce::dsp::Oscillator<float> osc;

    // parameter which should not be visible in the DAW
    juce::Value moduleTypes;
    juce::Value moduleChainPositions;

    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (BiztortionAudioProcessor)
};