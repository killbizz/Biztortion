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
    // the position of this in the DSPmodules vector
    unsigned int chainPosition;
    // number which determines the connection between this module and his parameters set
    unsigned int parameterNumber;
    // the type of this module
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