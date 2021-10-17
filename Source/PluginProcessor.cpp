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
                       )
#endif
{
    DSPModule* inputMeter = new MeterModuleDSP(apvts, "Input");
    DSPmodules.push_back(std::unique_ptr<DSPModule>(inputMeter));
    DSPModule* outputMeter = new MeterModuleDSP(apvts, "Output");
    DSPmodules.push_back(std::unique_ptr<DSPModule>(outputMeter));

    //if (!apvts.state.hasProperty("moduleTypes")) {
    //    apvts.state.setProperty("moduleTypes", var(juce::Array<juce::var>()), nullptr);
    //}
    //moduleTypes.referTo(apvts.state.getPropertyAsValue("moduleTypes", nullptr));

    //if (!apvts.state.hasProperty("moduleChainPositions")) {
    //    apvts.state.setProperty("moduleChainPositions", var(juce::Array<juce::var>()), nullptr);
    //}
    //moduleChainPositions.referTo(apvts.state.getPropertyAsValue("moduleChainPositions", nullptr));

    //// modules types and chainPositions to re-create DSPmodules saved in the APVTS
    //auto mt = moduleTypes.getValue().getArray();
    //if (!moduleTypes.getValue().isArray()) {
    //    jassertfalse;
    //}
    //auto mcp = moduleChainPositions.getValue().getArray();
    //if (!moduleChainPositions.getValue().isArray()) {
    //    jassertfalse;
    //}

    //auto chainPosition = mcp->begin();
    //for (auto type = mt->begin(); type < mt->end(); ++type) {
    //    //auto type = mt->begin();

    //    addModuleToDSPmodules(createDSPModule(static_cast<ModuleType>(int(*type))), int(*chainPosition));
    //    //++type;
    //    ++chainPosition;
    //}

    //// updateDSPState for all the modules int the chain
    //for (auto it = DSPmodules.cbegin(); it < DSPmodules.cend(); ++it) {
    //    (**it).updateDSPState(getSampleRate());
    //}

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

    // prepareToPlay for all the modules in the chain
    for (auto it = DSPmodules.cbegin(); it < DSPmodules.cend(); ++it) {
        (**it).prepareToPlay(sampleRate, samplesPerBlock);
    }

    // fft analyzers
    for (auto it = leftAnalyzerFIFOs.cbegin(); it < leftAnalyzerFIFOs.cend(); ++it) {
        (*it)->prepare(samplesPerBlock);
    }
    for (auto it = rightAnalyzerFIFOs.cbegin(); it < rightAnalyzerFIFOs.cend(); ++it) {
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
        // using a filter modules counter to find the right fft analyzer FIFO associated with the current filter
        unsigned int filterModuleCounter = 0;
        // processBlock for all the modules in the chain
        for (auto it = DSPmodules.cbegin(); it < DSPmodules.cend(); ++it) {
            auto module = &**it;
            module->processBlock(buffer, midiMessages, getSampleRate());
            auto filter = dynamic_cast<FilterModuleDSP*>(module);
            // fft analyzers FIFOs update
            if (filter) {
                auto index = filterModuleCounter++;
                leftAnalyzerFIFOs[index]->update(buffer);
                rightAnalyzerFIFOs[index]->update(buffer);
            }
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

    /*auto state = apvts.copyState();
    std::unique_ptr<juce::XmlElement> xml(state.createXml());
    copyXmlToBinary(*xml, destData);*/
}

void BiztortionAudioProcessor::setStateInformation(const void* data, int sizeInBytes)
{
     // You should use this method to restore your parameters from this memory block,
     // whose contents will have been created by the getStateInformation() call.

    auto tree = juce::ValueTree::readFromData(data, sizeInBytes);
    if (tree.isValid())
    {
        apvts.replaceState(tree);

        if (!apvts.state.hasProperty("moduleTypes")) {
            apvts.state.setProperty("moduleTypes", var(juce::Array<juce::var>()), nullptr);
        }
        moduleTypes.referTo(apvts.state.getPropertyAsValue("moduleTypes", nullptr));

        if (!apvts.state.hasProperty("moduleChainPositions")) {
            apvts.state.setProperty("moduleChainPositions", var(juce::Array<juce::var>()), nullptr);
        }
        moduleChainPositions.referTo(apvts.state.getPropertyAsValue("moduleChainPositions", nullptr));

        // modules types and chainPositions to re-create DSPmodules saved in the APVTS

        auto mt = moduleTypes.getValue().getArray();
        if (!moduleTypes.getValue().isArray()) {
            jassertfalse;
        }
        auto mcp = moduleChainPositions.getValue().getArray();
        if (!moduleChainPositions.getValue().isArray()) {
            jassertfalse;
        }

        // restoring DSP modules
        auto chainPosition = mcp->begin();
        for (auto type = mt->begin(); type < mt->end(); ++type) {
            addAndSetupModuleForDSP(createDSPModule(static_cast<ModuleType>(int(*type))), int(*chainPosition));
            ++chainPosition;
        }

        // updateDSPState for all the modules int the chain
        for (auto it = DSPmodules.cbegin(); it < DSPmodules.cend(); ++it) {
            (**it).updateDSPState(getSampleRate());
        }

    }
}

juce::AudioProcessorValueTreeState::ParameterLayout BiztortionAudioProcessor::createParameterLayout() {
    juce::AudioProcessorValueTreeState::ParameterLayout layout;

    // dynamic parameter management is not supported

    MeterModuleDSP::addParameters(layout);
    FilterModuleDSP::addParameters(layout);
    WaveshaperModuleDSP::addParameters(layout);
    OscilloscopeModuleDSP::addParameters(layout);
    BitcrusherModuleDSP::addParameters(layout);
    SlewLimiterModuleDSP::addParameters(layout);

    return layout;
}

DSPModule* BiztortionAudioProcessor::createDSPModule(ModuleType mt)
{
    DSPModule* newModule = nullptr;
    switch (mt) {
    case ModuleType::Oscilloscope: {
        newModule = new OscilloscopeModuleDSP(apvts);
        break;
    }
    case ModuleType::IIRFilter: {
        newModule = new FilterModuleDSP(apvts);
        break;
    }
    case ModuleType::Waveshaper: {
        newModule = new WaveshaperModuleDSP(apvts);
        break;
    }
    case ModuleType::Bitcrusher: {
        newModule = new BitcrusherModuleDSP(apvts);
        break;
    }
    case ModuleType::Clipper: {
        break;
    }
    case ModuleType::SlewLimiter: {
        newModule = new SlewLimiterModuleDSP(apvts);
        break;
    }
    default:
        break;
    }
    return newModule;
}

void BiztortionAudioProcessor::addModuleToDSPmodules(DSPModule* module, unsigned int chainPosition)
{

    // TODO: capisco perchè ricrendo lo stato del plugin non mi ricrea correttamente un Filtro (non sembra allocare il modulo e la relativa analyzerFIFO

    // DSP module setup
    module->setChainPosition(chainPosition);
    module->setModuleType();
    // insert module to DSPmodules vector
    bool inserted = false;
    for (auto it = DSPmodules.begin(); !inserted; ++it) {
        // end = 8° grid cell
        if ((**it).getChainPosition() == 8) {
            inserted = true;
            it = DSPmodules.insert(it, std::unique_ptr<DSPModule>(module));
            continue;
        }
        // there is at least one module in the vector
        auto next = it;
        ++next;
        if ((**it).getChainPosition() < chainPosition && chainPosition < (**next).getChainPosition()) {
            inserted = true;
            it = DSPmodules.insert(next, std::unique_ptr<DSPModule>(module));
        }
        // else continue to iterate to find the right grid position
    }
    // module is a Filter => FIFO allocation for fft analyzer
    if (dynamic_cast<FilterModuleDSP*>(module)) {
        insertNewAnalyzerFIFO(chainPosition);
    }
}

void BiztortionAudioProcessor::addAndSetupModuleForDSP(DSPModule* module, unsigned int chainPosition)
{
    // suspend audio processing
    suspendProcessing(true);
    addModuleToDSPmodules(module, chainPosition);
    // prepare to play the audio chain
    prepareToPlay(getSampleRate(), getBlockSize());
    // re-enable audio processing
    suspendProcessing(false);
}

void BiztortionAudioProcessor::addDSPmoduleTypeAndPositionToAPVTS(ModuleType mt, unsigned int chainPosition)
{
    moduleTypes.getValue().getArray()->add(var((int) mt));
    moduleChainPositions.getValue().getArray()->add(var((int) chainPosition));
}

void BiztortionAudioProcessor::removeDSPmoduleTypeAndPositionFromAPVTS(unsigned int chainPosition)
{
    auto mt = moduleTypes.getValue().getArray();
    if (!moduleTypes.getValue().isArray()) {
        jassertfalse;
    }
    auto mcp = moduleChainPositions.getValue().getArray();
    if (!moduleChainPositions.getValue().isArray()) {
        jassertfalse;
    }

    //auto type = mt->begin();
    auto cp = mcp->begin();
    for (auto type = mt->begin(); type < mt->end(); ++type) {
        if (chainPosition == int(*cp)) {
            mt->remove(type);
            mcp->remove(cp);
            break;
        }
        ++cp;
    }
    moduleTypes.setValue(*mt);
    moduleChainPositions.setValue(*mcp);
}

unsigned int BiztortionAudioProcessor::getFftAnalyzerFifoIndexOfCorrespondingFilter(unsigned int chainPosition)
{
    // using a filter modules counter to find the right fft analyzer FIFO associated with the current filter
    unsigned int filterModuleCounter = 0;
    bool found = false;
    for (auto it = DSPmodules.cbegin(); !found && it < DSPmodules.cend(); ++it) {
        if ((*it)->getChainPosition() == chainPosition) {
            found = true;
            continue;
        }
        if (dynamic_cast<FilterModuleDSP*>(&**it)) {
            filterModuleCounter++;
        }
    }
    return filterModuleCounter;
}

void BiztortionAudioProcessor::insertNewAnalyzerFIFO(unsigned int chainPosition)
{
    auto leftChannelFifo = new SingleChannelSampleFifo<BlockType>{ Channel::Left };
    auto rightChannelFifo = new SingleChannelSampleFifo<BlockType>{ Channel::Right };
    auto index = getFftAnalyzerFifoIndexOfCorrespondingFilter(chainPosition);
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

void BiztortionAudioProcessor::deleteOldAnalyzerFIFO(unsigned int chainPosition)
{
    auto index = getFftAnalyzerFifoIndexOfCorrespondingFilter(chainPosition);
    auto iteratorToDelete = leftAnalyzerFIFOs.begin() + index;
    leftAnalyzerFIFOs.erase(iteratorToDelete);
    auto iteratorToDelete2 = rightAnalyzerFIFOs.begin() + index;
    rightAnalyzerFIFOs.erase(iteratorToDelete2);
}

//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new BiztortionAudioProcessor();
}
