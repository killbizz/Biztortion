/*
  ==============================================================================

    PluginState.h
    Created: 14 Mar 2022 3:05:30pm
    Author:  gabri

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>

#include "../Module/MeterModule.h"
#include "../Module/FilterModule.h"
#include "../Module/WaveshaperModule.h"
#include "../Module/BitcrusherModule.h"
#include "../Module/SlewLimiterModule.h"
#include "../Module/OscilloscopeModule.h"

#include "../Shared/ModuleType.h"

class PluginState {
public:

    PluginState(juce::AudioProcessor& ap);

    juce::AudioProcessor& audioProcessor;
    // APVTS
    juce::AudioProcessorValueTreeState apvts;
    // fft analyzers
    using BlockType = juce::AudioBuffer<float>;
    // analyzer FIFO allocated only if the relative module is istantiated
    std::vector<SingleChannelSampleFifo<BlockType>*> leftAnalyzerFIFOs;
    std::vector<SingleChannelSampleFifo<BlockType>*> rightAnalyzerFIFOs;
    // DSP modules
    std::vector<std::unique_ptr<DSPModule>> DSPmodules;

    // parameter which should not be visible in the DAW
    juce::Value moduleTypes;
    juce::Value moduleChainPositions;
    // TODO : add moduleParameterNumbers

    void addModuleToDSPmodules(DSPModule* module, unsigned int chainPosition);
    void addAndSetupModuleForDSP(DSPModule* module, unsigned int chainPosition);
    void addDSPmoduleToAPVTS(ModuleType mt, unsigned int chainPosition);
    void removeModuleFromDSPmodules(unsigned int chainPosition);
    void removeDSPmoduleTypeAndPositionFromAPVTS(unsigned int chainPosition);
    unsigned int getFftAnalyzerFifoIndexOfCorrespondingFilter(unsigned int chainPosition);
    void insertNewAnalyzerFIFO(unsigned int chainPosition);
    void deleteOldAnalyzerFIFO(unsigned int chainPosition);
    foleys::LevelMeterSource* getMeterSource(juce::String type);

private:

    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

};