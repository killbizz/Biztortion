/*
  ==============================================================================

    DSPModule.h
    Created: 31 Aug 2021 3:45:49pm
    Author:  gabri

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>

class DSPModule {
public:
    DSPModule(juce::AudioProcessorValueTreeState& _apvts);

    virtual void updateDSPState(double sampleRate) = 0;
    virtual void prepareToPlay(double sampleRate, int samplesPerBlock) = 0;
    virtual void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&, double) = 0;

protected:
    juce::AudioProcessorValueTreeState& apvts;
};