/*
  ==============================================================================

    PluginProcessor.cpp

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

#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
BiztortionAudioProcessor::BiztortionAudioProcessor()
#ifndef JucePlugin_PreferredChannelConfigurations
     : AudioProcessor(BusesProperties()
                     #if ! JucePlugin_IsMidiEffect
                      #if ! JucePlugin_IsSynth
                       .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                      #endif
                       .withOutput ("Output", juce::AudioChannelSet::stereo(), true)
                     #endif
                       ),
    pluginState(*this),
    moduleGenerator(pluginState)

#endif
{}

BiztortionAudioProcessor::~BiztortionAudioProcessor()
{
}

//==============================================================================
const juce::String BiztortionAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool BiztortionAudioProcessor::acceptsMidi() const
{
   #if JucePlugin_WantsMidiInput
    return true;
   #else
    return false;
   #endif
}

bool BiztortionAudioProcessor::producesMidi() const
{
   #if JucePlugin_ProducesMidiOutput
    return true;
   #else
    return false;
   #endif
}

bool BiztortionAudioProcessor::isMidiEffect() const
{
   #if JucePlugin_IsMidiEffect
    return true;
   #else
    return false;
   #endif
}

double BiztortionAudioProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int BiztortionAudioProcessor::getNumPrograms()
{
    return 1;   // NB: some hosts don't cope very well if you tell them there are 0 programs,
                // so this should be at least 1, even if you're not really implementing programs.
}

int BiztortionAudioProcessor::getCurrentProgram()
{
    return 0;
}

void BiztortionAudioProcessor::setCurrentProgram (int index)
{
}

const juce::String BiztortionAudioProcessor::getProgramName (int index)
{
    return {};
}

void BiztortionAudioProcessor::changeProgramName (int index, const juce::String& newName)
{
}

//==============================================================================
void BiztortionAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    // Use this method as the place to do any pre-playback
    // initialisation that you need..

    // prepareToPlay for all the modules in the chain
    for (auto it = pluginState.DSPmodules.cbegin(); it < pluginState.DSPmodules.cend(); ++it) {
        (**it).prepareToPlay(sampleRate, samplesPerBlock);
    }

    // prepareToPlay for all the fft analyzer FIFOs
    for (auto it = pluginState.leftAnalyzerFIFOs.cbegin(); it < pluginState.leftAnalyzerFIFOs.cend(); ++it) {
        (*it)->prepare(samplesPerBlock);
    }
    for (auto it = pluginState.rightAnalyzerFIFOs.cbegin(); it < pluginState.rightAnalyzerFIFOs.cend(); ++it) {
        (*it)->prepare(samplesPerBlock);
    }

    //test signal preparation
    /*juce::dsp::ProcessSpec spec;
    spec.maximumBlockSize = samplesPerBlock;
    spec.numChannels = getTotalNumOutputChannels();
    spec.sampleRate = sampleRate;
    osc.initialise([](float x) { return std::sin(x); });
    osc.prepare(spec);
    osc.setFrequency(1000);*/

}

void BiztortionAudioProcessor::releaseResources()
{
    // When playback stops, you can use this as an opportunity to free up any
    // spare memory, etc.
}

#ifndef JucePlugin_PreferredChannelConfigurations
bool BiztortionAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
  #if JucePlugin_IsMidiEffect
    juce::ignoreUnused (layouts);
    return true;
  #else
    // This is the place where you check if the layout is supported.
    // In this template code we only support mono or stereo.
    // Some plugin hosts, such as certain GarageBand versions, will only
    // load plugins that support stereo bus layouts.
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
     && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

    // This checks if the input layout matches the output layout
   #if ! JucePlugin_IsSynth
    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
        return false;
   #endif

    return true;
  #endif
}
#endif

void BiztortionAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    auto totalNumInputChannels  = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();

    // In case we have more outputs than inputs, this code clears any output
    // channels that didn't contain input data, (because these aren't
    // guaranteed to be empty - they may contain garbage).
    // This is here to avoid people getting screaming feedback
    // when they first compile a plugin, but obviously you don't need to keep
    // this code if your algorithm always overwrites all the output channels.
    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear (i, 0, buffer.getNumSamples());

    if (!isSuspended()) {
        // using a module counter to find the right fft analyzer FIFO associated with the current module
        unsigned int moduleCounter = 0;
        // processBlock for all the modules in the chain
        for (auto it = pluginState.DSPmodules.cbegin(); it < pluginState.DSPmodules.cend(); ++it) {
            auto module = &**it;
            // fft analyzer FIFOs update
            if (module->getModuleType() == ModuleType::IIRFilter) {
                auto index = moduleCounter++;
                pluginState.leftAnalyzerFIFOs[index]->update(buffer);
                pluginState.rightAnalyzerFIFOs[index]->update(buffer);
            }
            module->processBlock(buffer, midiMessages, getSampleRate());
        }

        // test signal
        /*buffer.clear();
        juce::dsp::AudioBlock<float> block(buffer);
        juce::dsp::ProcessContextReplacing<float> stereoContext(block);
        osc.process(stereoContext);*/
    }
}

bool BiztortionAudioProcessor::hasEditor() const
{
    return true; // (change this to false if you choose to not supply an editor)
}

juce::AudioProcessorEditor* BiztortionAudioProcessor::createEditor()
{
    return new BiztortionAudioProcessorEditor(*this, pluginState);
    // return new juce::GenericAudioProcessorEditor(*this);
}

//==============================================================================
void BiztortionAudioProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    // You should use this method to store your parameters in the memory block.
    // You could do that either as raw data, or use the XML or ValueTree classes
    // as intermediaries to make it easy to save and load complex data.

    juce::MemoryOutputStream mos(destData, true);
    pluginState.apvts.state.writeToStream(mos);
}

void BiztortionAudioProcessor::setStateInformation(const void* data, int sizeInBytes)
{
     // You should use this method to restore your parameters from this memory block,
     // whose contents will have been created by the getStateInformation() call.

    auto tree = juce::ValueTree::readFromData(data, sizeInBytes);
    if (tree.isValid())
    {
        pluginState.apvts.replaceState(tree);

        if (!pluginState.apvts.state.hasProperty("moduleTypes")) {
            pluginState.apvts.state.setProperty("moduleTypes", var(juce::Array<juce::var>()), nullptr);
        }
        pluginState.moduleTypes.referTo(pluginState.apvts.state.getPropertyAsValue("moduleTypes", nullptr));

        if (!pluginState.apvts.state.hasProperty("moduleChainPositions")) {
            pluginState.apvts.state.setProperty("moduleChainPositions", var(juce::Array<juce::var>()), nullptr);
        }
        pluginState.moduleChainPositions.referTo(pluginState.apvts.state.getPropertyAsValue("moduleChainPositions", nullptr));

        if (!pluginState.apvts.state.hasProperty("moduleParameterNumbers")) {
            pluginState.apvts.state.setProperty("moduleParameterNumbers", var(juce::Array<juce::var>()), nullptr);
        }
        pluginState.moduleParameterNumbers.referTo(pluginState.apvts.state.getPropertyAsValue("moduleParameterNumbers", nullptr));

        // moduleTypes, chainPositions and parameterNumbers to re-create DSPmodules saved in the APVTS

        auto mt = pluginState.moduleTypes.getValue().getArray();
        if (!pluginState.moduleTypes.getValue().isArray()) {
            jassertfalse;
        }
        auto mcp = pluginState.moduleChainPositions.getValue().getArray();
        if (!pluginState.moduleChainPositions.getValue().isArray()) {
            jassertfalse;
        }
        auto mpn = pluginState.moduleParameterNumbers.getValue().getArray();
        if (!pluginState.moduleParameterNumbers.getValue().isArray()) {
            jassertfalse;
        }

        if (mt->isEmpty() || mcp->isEmpty() || mpn->isEmpty()) {
            pluginState.moduleTypes = var(juce::Array<juce::var>());
            mt = pluginState.moduleTypes.getValue().getArray();
            pluginState.moduleChainPositions = var(juce::Array<juce::var>());
            auto mcp = pluginState.moduleChainPositions.getValue().getArray();
            pluginState.moduleParameterNumbers = var(juce::Array<juce::var>());
            auto mpn = pluginState.moduleParameterNumbers.getValue().getArray();
        }

        // restoring DSP modules
        auto chainPosition = mcp->begin();
        auto parameterNumber = mpn->begin();
        for (auto type = mt->begin(); type < mt->end(); ++type) {

            pluginState.addAndSetupModuleForDSP(
                moduleGenerator.createDSPModule(static_cast<ModuleType>(int(*type))), static_cast<ModuleType>(int(*type)), int(*chainPosition), int(*parameterNumber)
            );

            ++chainPosition;
            ++parameterNumber;
        }

        if (getActiveEditor() != nullptr) {
            dynamic_cast<BiztortionAudioProcessorEditor*>(getActiveEditor())->guiState.setupChainModules();
        }

    }
}

//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new BiztortionAudioProcessor();
}
