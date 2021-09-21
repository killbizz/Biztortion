/*
  ==============================================================================

    DSPModule.h
    Created: 31 Aug 2021 3:45:49pm
    Author:  gabri

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include <string>

enum ModuleType {
    Uninstantiated,
    Meter,
    IIRFilter,
    Oscilloscope,
    Waveshaper,
    Bitcrusher,
    Clipper
};

class DSPModule {
public:
    DSPModule(juce::AudioProcessorValueTreeState& _apvts);
    unsigned int getChainPosition();
    void setChainPosition(unsigned int cp);
    ModuleType getModuleType();
    virtual void setModuleType() = 0;
    virtual void updateDSPState(double sampleRate) = 0;
    virtual void prepareToPlay(double sampleRate, int samplesPerBlock) = 0;
    virtual void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&, double) = 0;

protected:
    juce::AudioProcessorValueTreeState& apvts;
    // 0 - 9
    unsigned int chainPosition;
    ModuleType moduleType;
};