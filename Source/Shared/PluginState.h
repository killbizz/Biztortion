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

#include "../Shared/ModuleType.h"
#include "../Module/DSPModule.h"

#include "../Shared/FFTAnalyzer.h"

class PluginState {
public:

    PluginState(juce::AudioProcessor& ap);

    void addModuleToDSPmodules(DSPModule* module, unsigned int chainPosition);
    void addAndSetupModuleForDSP(DSPModule* module, ModuleType moduleType, unsigned int chainPosition, unsigned int parameterNumber);
    // returns the parameter number of the freshly added DSPmodule
    unsigned int addDSPmoduleToAPVTS(ModuleType mt, unsigned int chainPosition, int parameterNumber = 0);
    void removeModuleFromDSPmodules(unsigned int chainPosition);
    void removeDSPmoduleFromAPVTS(unsigned int chainPosition, ModuleType moduleType, unsigned int parameterNumber);

    unsigned int getFftAnalyzerFifoIndexOfCorrespondingFilter(unsigned int chainPosition);
    void insertNewAnalyzerFIFO(unsigned int chainPosition);
    void deleteOldAnalyzerFIFO(unsigned int chainPosition);

    foleys::LevelMeterSource* getMeterSource(juce::String type);

    unsigned int getParameterNumberFromDSPmodule(ModuleType moduleType, unsigned int chainPosition);
    unsigned int getChainPositionFromDSPmodule(ModuleType moduleType, unsigned int parameterNumber);

    //==============================================================================
    
    juce::AudioProcessor& audioProcessor;
    juce::AudioProcessorValueTreeState apvts;

    // --------- DSP ---------

    // chain positions range : 1 - 8
    std::vector<std::unique_ptr<DSPModule>> DSPmodules;
    // fft analyzer FIFOs allocated only if a module which needs fft analysis is istantiated
    using BlockType = juce::AudioBuffer<float>;
    std::vector<SingleChannelSampleFifo<BlockType>*> leftAnalyzerFIFOs;
    std::vector<SingleChannelSampleFifo<BlockType>*> rightAnalyzerFIFOs;

    // parameters which should not be visible in the DAW, used for storing/retrieving the plugin state
    juce::Value moduleTypes;
    juce::Value moduleChainPositions;
    juce::Value moduleParameterNumbers;

private:

    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

};