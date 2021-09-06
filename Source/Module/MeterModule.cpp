/*
  ==============================================================================

    MeterModule.cpp
    Created: 1 Sep 2021 2:57:12pm
    Author:  gabri

  ==============================================================================
*/

#include "MeterModule.h"
#include "../PluginProcessor.h"

//==============================================================================

/* MeterModule DSP */

//==============================================================================

MeterModuleDSP::MeterModuleDSP(juce::AudioProcessorValueTreeState& _apvts, juce::String _type)
    : DSPModule(_apvts), type(_type) {}

juce::String MeterModuleDSP::getType()
{
    return type;
}

MeterSettings MeterModuleDSP::getSettings(juce::AudioProcessorValueTreeState& apvts, juce::String type)
{
    MeterSettings settings;
    juce::String search = type + " Meter Level";
    settings.levelInDecibel = apvts.getRawParameterValue(search)->load();

    return settings;
}

void MeterModuleDSP::addParameters(juce::AudioProcessorValueTreeState::ParameterLayout& layout)
{
    layout.add(std::move(std::make_unique<juce::AudioParameterFloat>("Input Meter Level", "Input Meter Level", juce::NormalisableRange<float>(-60.f, 10.f, 0.5f), 0.f, "Input Meter")));
    layout.add(std::move(std::make_unique<juce::AudioParameterFloat>("Output Meter Level", "Output Meter Level", juce::NormalisableRange<float>(-60.f, 10.f, 0.5f), 0.f, "Output Meter")));
}

foleys::LevelMeterSource& MeterModuleDSP::getMeterSource()
{
    return meterSource;
}

void MeterModuleDSP::updateDSPState(double)
{
    auto settings = getSettings(apvts, type);
    level.setGainDecibels(settings.levelInDecibel);
}

void MeterModuleDSP::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    juce::dsp::ProcessSpec spec;

    spec.maximumBlockSize = samplesPerBlock;
    spec.numChannels = 2;
    spec.sampleRate = sampleRate;

    level.prepare(spec);

    meterSource.resize(2, sampleRate * 0.1 / samplesPerBlock);
}

void MeterModuleDSP::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer&, double sampleRate)
{
    updateDSPState(sampleRate);

    juce::dsp::AudioBlock<float> block(buffer);
    juce::dsp::ProcessContextReplacing<float> context(block);

    level.process(context);

    meterSource.measureBlock(buffer);
}

//==============================================================================

/* MeterModule GUI */

//==============================================================================

MeterModuleGUI::MeterModuleGUI(BiztortionAudioProcessor& p, juce::String _type)
    : GUIModule(_type == "Input" ? 0 : 9), audioProcessor(p), type(_type),
    levelSlider(*audioProcessor.apvts.getParameter(type + " Meter Level"), "dB"),
    levelSliderAttachment(audioProcessor.apvts, type + " Meter Level", levelSlider)
{
    lnf.setColour(foleys::LevelMeter::lmMeterGradientLowColour, juce::Colours::green);
    meter.setLookAndFeel(&lnf);
    meter.setMeterSource(getMeterSource());
    addAndMakeVisible(meter);

    addAndMakeVisible(levelSlider);
    levelSlider.labels.add({ 0.f, "-60dB" });
    levelSlider.labels.add({ 1.f, "+10dB" });
}

MeterModuleGUI::~MeterModuleGUI()
{
    meter.setLookAndFeel(nullptr);
}

juce::String MeterModuleGUI::getType()
{
    return type;
}

foleys::LevelMeterSource* MeterModuleGUI::getMeterSource()
{
    foleys::LevelMeterSource* source = nullptr;
    for (auto it = audioProcessor.DSPmodules.cbegin(); it < audioProcessor.DSPmodules.cend(); ++it) {
        auto temp = dynamic_cast<MeterModuleDSP*>(&(**it));
        if (temp && temp->getType() == type) {
            source = &temp->getMeterSource();
        }
    }
    return source;
}

std::vector<juce::Component*> MeterModuleGUI::getComps()
{
    return std::vector<juce::Component*>();
}

void MeterModuleGUI::paint(juce::Graphics& g)
{
    drawContainer(g);
}

void MeterModuleGUI::resized()
{
    auto bounds = getContentRenderArea();

    auto meterArea = bounds.removeFromLeft(bounds.getWidth() * (1.f / 2.f));
    meterArea.reduce(50, 50);
    bounds.reduce(20, 20);

    meter.setBounds(meterArea);
    levelSlider.setBounds(bounds);
}
