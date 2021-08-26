/*
  ==============================================================================

    DistortionModule.h
    Created: 26 Aug 2021 12:05:19pm
    Author:  gabri

  ==============================================================================
*/

#pragma once

#include "../JuceLibraryCode/JuceHeader.h"

//==============================================================================

/* DistortionModule DSP */

//==============================================================================

class DistortionModule {
public:
    DistortionModule(juce::AudioProcessorValueTreeState&);
    void prepareToPlay(double sampleRate, int samplesPerBlock);
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&, double);

    static void addDistortionParameters(juce::AudioProcessorValueTreeState::ParameterLayout&);
private:
    juce::AudioProcessorValueTreeState& apvts;
};

//==============================================================================

/* DistortionModule GUI */

//==============================================================================