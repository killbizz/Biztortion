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
                       ), /*preFilter(apvts, "Pre"), postFilter(apvts, "Post"),*/ waveshaperModule(apvts) /*, inputMeter(apvts, "Input"), outputMeter(apvts, "Output")*/
#endif
{
    DSPModule* inputMeter = new MeterModuleDSP(apvts, "Input");
    modules.push_back(std::unique_ptr<DSPModule>(inputMeter));
    DSPModule* preFilter = new FilterModuleDSP(apvts, "Pre");
    modules.push_back(std::unique_ptr<DSPModule>(preFilter));
    DSPModule* postFilter = new FilterModuleDSP(apvts, "Post");
    modules.push_back(std::unique_ptr<DSPModule>(postFilter));
    DSPModule* outputMeter = new MeterModuleDSP(apvts, "Output");
    modules.push_back(std::unique_ptr<DSPModule>(outputMeter));
}

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

    // prepareToPlay for all the modules int the chain
    for (auto it = modules.cbegin(); it < modules.cend(); ++it) {
        (**it).prepareToPlay(sampleRate, samplesPerBlock);
    }

    oscilloscope.clear();
    oscilloscope.setHorizontalZoom(0.1f);

    // waveshaperModule.prepareToPlay(sampleRate, samplesPerBlock);

    // fft analyzers
    preLeftChannelFifo.prepare(samplesPerBlock);
    preRightChannelFifo.prepare(samplesPerBlock);
    postLeftChannelFifo.prepare(samplesPerBlock);
    postRightChannelFifo.prepare(samplesPerBlock);

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

    // processBlock for all the modules int the chain
    for (auto it = modules.cbegin(); it < modules.cend(); ++it) {
        (**it).processBlock(buffer, midiMessages, getSampleRate());
        // if **it è un filtro
        //    allora faccio l'update della corrispettiva fifo per la fft
    }

    //oscilloscope.processBlock(buffer.getReadPointer(0), buffer.getNumSamples());
    // fft analyzers
    preLeftChannelFifo.update(buffer);
    preRightChannelFifo.update(buffer);
    postLeftChannelFifo.update(buffer);
    postRightChannelFifo.update(buffer);

    // test signal
    /*buffer.clear();
    juce::dsp::AudioBlock<float> block(buffer);
    juce::dsp::ProcessContextReplacing<float> stereoContext(block);
    osc.process(stereoContext);*/
}

bool BiztortionAudioProcessor::hasEditor() const
{
    return true; // (change this to false if you choose to not supply an editor)
}

juce::AudioProcessorEditor* BiztortionAudioProcessor::createEditor()
{
    return new BiztortionAudioProcessorEditor(*this);
    // return new juce::GenericAudioProcessorEditor(*this);
}

//==============================================================================
void BiztortionAudioProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    // You should use this method to store your parameters in the memory block.
    // You could do that either as raw data, or use the XML or ValueTree classes
    // as intermediaries to make it easy to save and load complex data.

    juce::MemoryOutputStream mos(destData, true);
    apvts.state.writeToStream(mos);
}

void BiztortionAudioProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    // You should use this method to restore your parameters from this memory block,
    // whose contents will have been created by the getStateInformation() call.
    auto tree = juce::ValueTree::readFromData(data, sizeInBytes);
    if (tree.isValid())
    {
        apvts.replaceState(tree);

        // updateDSPState for all the modules int the chain
        for (auto it = modules.cbegin(); it < modules.cend(); ++it) {
            (**it).updateDSPState(getSampleRate());
        }
    }
}

juce::AudioProcessorValueTreeState::ParameterLayout BiztortionAudioProcessor::createParameterLayout() {
    juce::AudioProcessorValueTreeState::ParameterLayout layout;

    // dynamic parameter management is not supported

    MeterModuleDSP::addParameters(layout);
    FilterModuleDSP::addParameters(layout);
    WaveshaperModule::addParameters(layout);

    return layout;
}

//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new BiztortionAudioProcessor();
}
