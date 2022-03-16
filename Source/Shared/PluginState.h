/*
  ==============================================================================

    PluginState.h

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

#include "../Module/MeterModule.h"
#include "../Module/FilterModule.h"
#include "../Module/WaveshaperModule.h"
#include "../Module/BitcrusherModule.h"
#include "../Module/SlewLimiterModule.h"
#include "../Module/OscilloscopeModule.h"
#include "../Module/ChainModule.h"

#include "../Shared/ModuleType.h"

class PluginState {
public:

    PluginState(juce::AudioProcessor& ap);

    // quick way to access the audio processor
    juce::AudioProcessor& audioProcessor;
    // APVTS
    juce::AudioProcessorValueTreeState apvts;

    // --------- DSP ---------
    std::vector<std::unique_ptr<DSPModule>> DSPmodules;
    // fft analyzer FIFOs allocated only if a module which needs fft analysis is istantiated
    using BlockType = juce::AudioBuffer<float>;
    std::vector<SingleChannelSampleFifo<BlockType>*> leftAnalyzerFIFOs;
    std::vector<SingleChannelSampleFifo<BlockType>*> rightAnalyzerFIFOs;

    // parameters which should not be visible in the DAW
    juce::Value moduleTypes;
    juce::Value moduleChainPositions;
    // TODO : add moduleParameterNumbers

    void addModuleToDSPmodules(DSPModule* module, unsigned int chainPosition);
    void addAndSetupModuleForDSP(DSPModule* module, unsigned int chainPosition);
    void addDSPmoduleToAPVTS(ModuleType mt, unsigned int chainPosition);
    void removeModuleFromDSPmodules(unsigned int chainPosition);
    void removeDSPmoduleFromAPVTS(unsigned int chainPosition);
    unsigned int getFftAnalyzerFifoIndexOfCorrespondingFilter(unsigned int chainPosition);
    void insertNewAnalyzerFIFO(unsigned int chainPosition);
    void deleteOldAnalyzerFIFO(unsigned int chainPosition);
    foleys::LevelMeterSource* getMeterSource(juce::String type);

private:

    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

};