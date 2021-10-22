/*
  ==============================================================================

    DSPModule.cpp
    Created: 31 Aug 2021 3:45:50pm
    Author:  gabri

  ==============================================================================
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

ModuleType DSPModule::getModuleType()
{
    return moduleType;
}

void DSPModule::applyAsymmetry(juce::AudioBuffer<float>& drySignal, juce::AudioBuffer<float>& wetSignal, float symmetryAmount, float symmetryBias, int numSamples)
{

    // TO FIX

    float dryGain = std::abs(symmetryAmount);
    float wetGain = 1 - dryGain;

    for (auto channel = 0; channel < 2; ++channel)
    {
        auto* wetData = wetSignal.getWritePointer(channel);
        auto* bufferData = drySignal.getWritePointer(channel);
        for (auto i = 0; i < numSamples; ++i) {
            if (symmetryAmount == 0.f) {
                bufferData[i] = wetData[i];
            }
            else if (symmetryAmount > 0.f) {
                if (bufferData[i] < -symmetryBias) {
                    bufferData[i] = sumSignals(bufferData[i], 0.f, wetData[i], 1.f);
                }
                else {
                    bufferData[i] = sumSignals(bufferData[i], dryGain, wetData[i], wetGain);
                }
            }
            else {
                if (bufferData[i] >= -symmetryBias) {
                    bufferData[i] = sumSignals(bufferData[i], 0.f, wetData[i], 1.f);
                }
                else {
                    bufferData[i] = sumSignals(bufferData[i], dryGain, wetData[i], wetGain);
                }
            }
        }
    }
}

float DSPModule::sumSignals(float drySignal, float dryGain, float wetSignal, float wetGain)
{
    return (drySignal * dryGain) + (wetSignal * wetGain);
}
