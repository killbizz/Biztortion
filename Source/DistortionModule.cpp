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
}
