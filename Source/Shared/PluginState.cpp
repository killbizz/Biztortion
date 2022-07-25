/*
  ==============================================================================

    PluginState.cpp

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

#include "PluginState.h"

#include "../Module/MeterModule.h"
#include "../Module/FilterModule.h"
#include "../Module/WaveshaperModule.h"
#include "../Module/ClassicBitcrusherModule.h"
#include "../Module/SpectrumBitcrusherModule.h"
#include "../Module/SlewLimiterModule.h"
#include "../Module/OscilloscopeModule.h"

PluginState::PluginState(juce::AudioProcessor& ap) :
    audioProcessor(ap),
    apvts(ap, nullptr, "Parameters", createParameterLayout()),
    moduleTypes(var(juce::Array<juce::var>())),
    moduleChainPositions(var(juce::Array<juce::var>())),
    moduleParameterNumbers(var(juce::Array<juce::var>()))
{
    DSPModule* inputMeter = new MeterModuleDSP(apvts, "Input");
    DSPmodules.push_back(std::unique_ptr<DSPModule>(inputMeter));
    DSPModule* outputMeter = new MeterModuleDSP(apvts, "Output");
    DSPmodules.push_back(std::unique_ptr<DSPModule>(outputMeter));

    if (!apvts.state.hasProperty("moduleTypes")) {
        apvts.state.setProperty("moduleTypes", var(juce::Array<juce::var>()), nullptr);
    }
    moduleTypes.referTo(apvts.state.getPropertyAsValue("moduleTypes", nullptr));

    if (!apvts.state.hasProperty("moduleChainPositions")) {
        apvts.state.setProperty("moduleChainPositions", var(juce::Array<juce::var>()), nullptr);
    }
    moduleChainPositions.referTo(apvts.state.getPropertyAsValue("moduleChainPositions", nullptr));

    if (!apvts.state.hasProperty("moduleParameterNumbers")) {
        apvts.state.setProperty("moduleParameterNumbers", var(juce::Array<juce::var>()), nullptr);
    }
    moduleParameterNumbers.referTo(apvts.state.getPropertyAsValue("moduleParameterNumbers", nullptr));
}

void PluginState::addModuleToDSPmodules(DSPModule* module, unsigned int chainPosition)
{
    // insert module to DSPmodules vector
    bool inserted = false;
    for (auto it = DSPmodules.begin(); !inserted; ++it) {
        // end = 8° grid cell
        if ((**it).getChainPosition() == 9) {
            inserted = true;
            it = DSPmodules.insert(it, std::unique_ptr<DSPModule>(module));
            continue;
        }
        // there is at least one module in the vector
        auto next = it;
        ++next;
        if ((**it).getChainPosition() <= chainPosition && chainPosition < (**next).getChainPosition()) {
            inserted = true;
            it = DSPmodules.insert(next, std::unique_ptr<DSPModule>(module));
        }
        // else continue to iterate to find the right grid position
    }
    // module is a filter => FIFO allocation for FFT data
    if (module->getModuleType() == ModuleType::IIRFilter) {
        insertNewAnalyzerFIFO(chainPosition);
    }
}

void PluginState::addAndSetupModuleForDSP(DSPModule* module, ModuleType moduleType, unsigned int chainPosition, unsigned int parameterNumber)
{
    // suspend audio processing
    audioProcessor.suspendProcessing(true);
    // DSP module setup
    module->setChainPosition(chainPosition);
    module->setParameterNumber(parameterNumber);
    module->setModuleType(moduleType);
    addModuleToDSPmodules(module, chainPosition);
    // prepare to play the audio chain
    audioProcessor.prepareToPlay(audioProcessor.getSampleRate(), audioProcessor.getBlockSize());
    // re-enable audio processing
    audioProcessor.suspendProcessing(false);
}

unsigned int PluginState::addDSPmoduleToAPVTS(ModuleType moduleType, unsigned int chainPosition, int parameterNumber)
{
    auto mtArray = moduleTypes.getValue().getArray();
    if (!moduleTypes.getValue().isArray()) {
        jassertfalse;
    }

    auto mcpArray = moduleChainPositions.getValue().getArray();
    if (!moduleChainPositions.getValue().isArray()) {
        jassertfalse;
    }

    auto mpnArray = moduleParameterNumbers.getValue().getArray();
    if (!moduleParameterNumbers.getValue().isArray()) {
        jassertfalse;
    }

    // assigning the first available number to the parameters of the freshly added DSPmodule
    // only if parameterNumber == 0, else I use parameterNumber value 
    unsigned int parameterNumberToUse = parameterNumber != 0 ? parameterNumber : 1;

    auto mpn = mpnArray->begin();
    auto type = mtArray->begin();

    while (parameterNumber == 0 && type < mtArray->end()) {
        if (static_cast<ModuleType>(int(*type)) == moduleType && int(*mpn) == parameterNumberToUse) {
            ++parameterNumberToUse;
            mpn = mpnArray->begin();
            type = mtArray->begin();
        }
        else {
            ++mpn;
            ++type;
        }
    }

    mtArray->add(var((int)moduleType));
    mcpArray->add(var((int)chainPosition));
    mpnArray->add(var((int)parameterNumberToUse));

    moduleTypes.setValue(*mtArray);
    moduleChainPositions.setValue(*mcpArray);
    moduleParameterNumbers.setValue(*mpnArray);

    return parameterNumberToUse;
}

void PluginState::removeModuleFromDSPmodules(unsigned int chainPosition)
{
    bool found = false;
    for (auto it = DSPmodules.begin(); !found && it < DSPmodules.end(); ++it) {
        if ((**it).getChainPosition() == chainPosition) {
            found = true;
            audioProcessor.suspendProcessing(true);
            // remove fft analyzer FIFO associated with **it if it's a filter
            if ((&**it)->getModuleType() == ModuleType::IIRFilter) {
                deleteOldAnalyzerFIFO(chainPosition);
            }
            it = DSPmodules.erase(it);
            audioProcessor.suspendProcessing(false);
        }
    }
}

void PluginState::removeDSPmoduleFromAPVTS(unsigned int chainPosition, ModuleType moduleType, unsigned int parameterNumber)
{
    auto mt = moduleTypes.getValue().getArray();
    if (!moduleTypes.getValue().isArray()) {
        jassertfalse;
    }

    auto mcp = moduleChainPositions.getValue().getArray();
    if (!moduleChainPositions.getValue().isArray()) {
        jassertfalse;
    }

    auto mpn = moduleParameterNumbers.getValue().getArray();
    if (!moduleParameterNumbers.getValue().isArray()) {
        jassertfalse;
    }

    auto pn = mpn->begin();
    auto cp = mcp->begin();
    bool found = false;

    for (auto type = mt->begin(); !found && type < mt->end(); ++type) {
        if (chainPosition == int(*cp) && int(moduleType) == int(*type) && parameterNumber == int(*pn)) {
            found = true;
            mt->remove(type);
            mcp->remove(cp);
            mpn->remove(pn);
            break;
        }
        ++cp;
        ++pn;
    }

    moduleTypes.setValue(*mt);
    moduleChainPositions.setValue(*mcp);
    moduleParameterNumbers.setValue(*mpn);
}

unsigned int PluginState::getSampleFifoIndexOfCorrespondingModule(unsigned int chainPosition)
{
    // using a module counter to find the right fft analyzer FIFO associated with the current filter
    unsigned int moduleCounter = 0;
    bool found = false;
    for (auto it = DSPmodules.cbegin(); !found && it < DSPmodules.cend(); ++it) {
        if ((*it)->getChainPosition() == chainPosition) {
            found = true;
            continue;
        }
        if (((&**it)->getModuleType() == ModuleType::IIRFilter) || ((&**it)->getModuleType() == ModuleType::SpectrumBitcrusher)) {
            moduleCounter++;
        }
    }
    return moduleCounter;
}

void PluginState::insertNewAnalyzerFIFO(unsigned int chainPosition)
{
    auto leftChannelFifo = new SingleChannelSampleFifo<BlockType>{ Channel::Left };
    auto rightChannelFifo = new SingleChannelSampleFifo<BlockType>{ Channel::Right };
    auto index = getSampleFifoIndexOfCorrespondingModule(chainPosition);
    if (leftAnalyzerFIFOs.empty()) {
        leftAnalyzerFIFOs.push_back(leftChannelFifo);
    }
    else {
        auto position = leftAnalyzerFIFOs.begin() + index;
        leftAnalyzerFIFOs.insert(position, leftChannelFifo);
    }

    if (rightAnalyzerFIFOs.empty()) {
        rightAnalyzerFIFOs.push_back(rightChannelFifo);
    }
    else {
        auto position2 = rightAnalyzerFIFOs.begin() + index;
        rightAnalyzerFIFOs.insert(position2, rightChannelFifo);
    }
}

void PluginState::deleteOldAnalyzerFIFO(unsigned int chainPosition)
{
    auto index = getSampleFifoIndexOfCorrespondingModule(chainPosition);
    auto iteratorToDelete = leftAnalyzerFIFOs.begin() + index;
    leftAnalyzerFIFOs.erase(iteratorToDelete);
    auto iteratorToDelete2 = rightAnalyzerFIFOs.begin() + index;
    rightAnalyzerFIFOs.erase(iteratorToDelete2);
}

foleys::LevelMeterSource* PluginState::getMeterSource(juce::String type)
{
    foleys::LevelMeterSource* source = nullptr;
    for (auto it = DSPmodules.cbegin(); it < DSPmodules.cend(); ++it) {
        auto temp = dynamic_cast<MeterModuleDSP*>(&(**it));
        if (temp && temp->getType() == type) {
            source = &temp->getMeterSource();
        }
    }
    return source;
}

unsigned int PluginState::getParameterNumberFromDSPmodule(ModuleType moduleType, unsigned int chainPosition)
{
    unsigned int paramNumber = 0;
    bool found = false;
    for (auto it = DSPmodules.cbegin(); !found && it < DSPmodules.cend(); ++it) {
        if (( (*it)->getChainPosition() == chainPosition ) && ( (*it)->getModuleType() == moduleType )) {
            found = true;
            paramNumber = (*it)->getParameterNumber();
            continue;
        }
    }
    return paramNumber;
}

unsigned int PluginState::getChainPositionFromDSPmodule(ModuleType moduleType, unsigned int parameterNumber)
{
    unsigned int chainPosition = 0;
    bool found = false;
    for (auto it = DSPmodules.cbegin(); !found && it < DSPmodules.cend(); ++it) {
        if (((*it)->getParameterNumber() == parameterNumber) && ((*it)->getModuleType() == moduleType)) {
            found = true;
            chainPosition = (*it)->getChainPosition();
            continue;
        }
    }
    return chainPosition;
}

juce::AudioProcessorValueTreeState::ParameterLayout PluginState::createParameterLayout() {
    juce::AudioProcessorValueTreeState::ParameterLayout layout;

    // dynamic parameter management is not supported

    MeterModuleDSP::addParameters(layout);
    FilterModuleDSP::addParameters(layout);
    WaveshaperModuleDSP::addParameters(layout);
    OscilloscopeModuleDSP::addParameters(layout);
    ClassicBitcrusherModuleDSP::addParameters(layout);
    SpectrumBitcrusherModuleDSP::addParameters(layout);
    SlewLimiterModuleDSP::addParameters(layout);

    return layout;
}