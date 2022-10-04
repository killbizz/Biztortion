/*
  ==============================================================================

    DSPModule.cpp

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

#include "DSPModule.h"

DSPModule::DSPModule(juce::AudioProcessorValueTreeState& _apvts)
    : apvts(_apvts)
{
}

unsigned int DSPModule::getChainPosition()
{
    return chainPosition;
}

void DSPModule::setChainPosition(unsigned int cp)
{
    chainPosition = cp;
}

unsigned int DSPModule::getParameterNumber()
{
    return parameterNumber;
}

void DSPModule::setParameterNumber(unsigned int pn)
{
    parameterNumber = pn;
}

ModuleType DSPModule::getModuleType()
{
    return moduleType;
}

void DSPModule::setModuleType(ModuleType mt)
{
    moduleType = mt;
}

void DSPModule::effectDistribution(juce::AudioBuffer<float>& inOutDrySignal, juce::AudioBuffer<float>& wetSignal, float amount, float bias, int numSamples)
{
    float dryGain = std::abs(amount);
    float wetGain = 1 - dryGain;

    for (auto channel = 0; channel < 2; ++channel)
    {
        auto* wetData = wetSignal.getWritePointer(channel);
        auto* bufferData = inOutDrySignal.getWritePointer(channel);
        for (auto i = 0; i < numSamples; ++i) {
            if (amount == 0.f) {
                bufferData[i] = wetData[i];
            }
            else if (amount > 0.f) {
                if (bufferData[i] >= -bias) {
                    bufferData[i] = sumSignals(bufferData[i], 0.f, wetData[i], 1.f);
                }
                else {
                    bufferData[i] = sumSignals(bufferData[i], dryGain, wetData[i], wetGain);
                }
            }
            else {
                if (bufferData[i] < -bias) {
                    bufferData[i] = sumSignals(bufferData[i], 0.f, wetData[i], 1.f);
                }
                else {
                    bufferData[i] = sumSignals(bufferData[i], dryGain, wetData[i], wetGain);
                }
            }
        }
    }
}

void DSPModule::applySymmetry(juce::AudioBuffer<float>& signal, float amount, int numSamples)
{
    for (auto channel = 0; channel < 2; ++channel)
    {
        auto* bufferData = signal.getWritePointer(channel);
        for (auto i = 0; i < numSamples; ++i) {
            if (bufferData[i] < 0.0f) {
                bufferData[i] *= (2.0f - amount);
            } else {
                bufferData[i] *= amount;
            }
        }
    }
}

float DSPModule::sumSignals(float drySignal, float dryGain, float wetSignal, float wetGain)
{
    return (drySignal * dryGain) + (wetSignal * wetGain);
}
