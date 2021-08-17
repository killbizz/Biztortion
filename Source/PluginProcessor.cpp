/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "ResponseCurve.h"

//==============================================================================
BiztortionAudioProcessor::BiztortionAudioProcessor()
#ifndef JucePlugin_PreferredChannelConfigurations
     : MagicProcessor(BusesProperties()
                     #if ! JucePlugin_IsMidiEffect
                      #if ! JucePlugin_IsSynth
                       .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                      #endif
                       .withOutput ("Output", juce::AudioChannelSet::stereo(), true)
                     #endif
                       ), filterModule(apvts, getSampleRate())
#endif
{
    FOLEYS_SET_SOURCE_PATH(__FILE__);

    magicState.setGuiValueTree(BinaryData::magic_xml, BinaryData::magic_xmlSize);

    // MAGIC GUI: add visualizations to Editor
    outputMeter = magicState.createAndAddObject<foleys::MagicLevelSource>("output");
    oscilloscope = magicState.createAndAddObject<foleys::MagicOscilloscope>("waveform");
    analyser = magicState.createAndAddObject<foleys::MagicAnalyser>("analyser");
    magicState.addBackgroundProcessing(analyser);
}

BiztortionAudioProcessor::~BiztortionAudioProcessor()
{
}

void BiztortionAudioProcessor::initialiseBuilder(foleys::MagicGUIBuilder& builder)
{
    builder.registerJUCEFactories();

    builder.registerFactory("ResponseCurveComponent", &ResponseCurveComponentItem::factory);
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

    filterModule.prepareToPlay(sampleRate, samplesPerBlock);

    oscilloscope->prepareToPlay(sampleRate, samplesPerBlock);
    analyser->prepareToPlay(sampleRate, samplesPerBlock);
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

    filterModule.processBlock(buffer, midiMessages);

    oscilloscope->pushSamples(buffer);
    analyser->pushSamples(buffer);
}

juce::AudioProcessorValueTreeState::ParameterLayout BiztortionAudioProcessor::createParameterLayout() {
    juce::AudioProcessorValueTreeState::ParameterLayout layout;

    FilterModule::addFilterParameters(layout);

    return layout;
}

//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new BiztortionAudioProcessor();
}
