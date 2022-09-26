/*
  ==============================================================================

    DSPModule.h

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

#pragma once

#include <JuceHeader.h>
#include <string>
#include "../Shared/ModuleType.h"

class DSPModule {
public:
    DSPModule(juce::AudioProcessorValueTreeState& _apvts);

    unsigned int getChainPosition();
    void setChainPosition(unsigned int cp);
    unsigned int getParameterNumber();
    void setParameterNumber(unsigned int pn);
    ModuleType getModuleType();
    void setModuleType(ModuleType moduleType);

    virtual void updateDSPState(double sampleRate) = 0;
    virtual void prepareToPlay(double sampleRate, int samplesPerBlock) = 0;
    virtual void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&, double) = 0;

protected:
    juce::AudioProcessorValueTreeState& apvts;

    // range: 1 - 8
    // the position of this in the audio processing chain
    unsigned int chainPosition;
    // number which determines the connection between this module and his parameters set
    unsigned int parameterNumber;
    // the module type of this
    ModuleType moduleType;

    /**
    * Use this function to apply the module processing to the positive or negative section of the waveform, moving the waveform center point with the bias parameter
    * 
    * @param    inOutDrySignal the original signal before processing [WARNING: the function does side effect to this buffer!]
    * @param    wetSignal the signal after being processed
    * @param    amount determines the percentage of wet signal in the positive and negative section of the waveform ; range = -1.f/+1.f
    * @param    bias determines the threshold to determine the separation between positive and negative section of the waveform ; range = -0.9f/+0.9f
    * @param    numSamples determines the number of samples in the buffers
    */
    void effectDistribution(juce::AudioBuffer<float>& inOutDrySignal, juce::AudioBuffer<float>& wetSignal, float amount, float bias, int numSamples);

    /**
    * Use this function to apply symmetry to the input buffer
    *
    * @param    signal : the input signal [WARNING: the function does side effect to this buffer!]
    * @param    amount determines the amount of asymmetry to apply [if amount == 0 => the signal is uneffected] ; range = 0.f/+2.f
    * @param    numSamples determines the number of samples in the buffer
    */
    void applySymmetry(juce::AudioBuffer<float>& signal, float amount, int numSamples);

private:
    float sumSignals(float drySignal, float dryGain, float wetSignal, float wetGain);
};