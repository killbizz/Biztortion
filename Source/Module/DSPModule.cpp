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
