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
    SlewLimiter
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

    /**
    * Use this function only in distortion modules to apply asymmetry (if symmetryBias !=0, else is normal symmetry)
    * symmetryBias range is -0.9/+0.9 so select carefully your bias in order to apply the desired asymmetry effect
    * 
    * @param    drySignal the original signal before processing [WARNING: the function does side effect to this buffer!]
    * @param    wetSignal the signal after being processed
    * @param    symmetryAmount determines the percentage of wet signal in the positive or negative part of the waveform
    * @param    symmetryBias determines the threshold to determine the separation between positive and negative phase of the waveform
    * @param    numSamples determines the number of samples in the buffer
    */
    void applyAsymmetry(juce::AudioBuffer<float>& drySignal, juce::AudioBuffer<float>& wetSignal, float symmetryAmount, float symmetryBias, int numSamples);

private:
    float sumSignals(float drySignal, float dryGain, float wetSignal, float wetGain);
};