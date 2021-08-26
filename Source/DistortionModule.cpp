/*
  ==============================================================================

    DistortionModule.cpp
    Created: 26 Aug 2021 12:05:19pm
    Author:  gabri

  ==============================================================================
*/

#include "DistortionModule.h"

DistortionModule::DistortionModule(juce::AudioProcessorValueTreeState& apvts)
    : apvts(apvts)
{
}

void DistortionModule::prepareToPlay(double sampleRate, int samplesPerBlock)
{
}

void DistortionModule::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages, double sampleRate)
{
    if (isActive) {
        updateDSP();
        // TODO : modifico nome del modulo ( tipologia della distorsione come nome )
        // => uso moduli diversi per waveshaping, bitcrushing, spatial distortion, ...
    }
}

void DistortionModule::addParameters(juce::AudioProcessorValueTreeState::ParameterLayout& layout)
{
    using namespace juce;

    layout.add(std::make_unique<juce::AudioParameterFloat>("DRIVE", "Drive", NormalisableRange<float>(0.f, 40.f, 0.01f), 20.f, "Distortion"));
    layout.add(std::make_unique<AudioParameterFloat>("MIX", "Mix", NormalisableRange<float>(0.f, 100.f, 0.01f), 100.f, "Distortion"));
    layout.add(std::make_unique<AudioParameterFloat>("OVERDRIVEAMP", "Overdrive Amp", NormalisableRange<float>(0.f, 100.f, 0.01f), 100.f, "Distortion"));
    layout.add(std::make_unique<AudioParameterFloat>("OVERDRIVESLOPE", "Overdrive Slope", NormalisableRange<float>(1.f, 15.f, 0.01f), 1.f, "Distortion"));
    layout.add(std::make_unique<AudioParameterFloat>("FUZZYAMP", "Fuzzy Amp", NormalisableRange<float>(0.f, 100.f, 0.01f), 0.f, "Distortion"));
    layout.add(std::make_unique<AudioParameterFloat>("FUZZYFREQ", "Fuzzy Freq", NormalisableRange<float>(0.5f, 100.f, 0.01f), 1.f, "Distortion"));
}

DistortionSettings DistortionModule::getSettings(juce::AudioProcessorValueTreeState& apvts)
{
    DistortionSettings settings;

    settings.mix = apvts.getRawParameterValue("MIX")->load();
    settings.drive = apvts.getRawParameterValue("DRIVE")->load();
    settings.overdriveAmp = apvts.getRawParameterValue("OVERDRIVEAMP")->load();
    settings.overdriveSlope = apvts.getRawParameterValue("OVERDRIVESLOPE")->load();
    settings.fuzzyAmp = apvts.getRawParameterValue("FUZZYAMP")->load();
    settings.fuzzyFreq = apvts.getRawParameterValue("FUZZYFREQ")->load();

    return settings;
}

void DistortionModule::updateDSP()
{

}
