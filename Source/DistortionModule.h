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

struct DistortionSettings {
    float mix{ 0 }, drive{ 0 }, overdriveAmp{ 0 }, overdriveSlope{ 0 }, fuzzyAmp{ 0 }, fuzzyFreq{ 0 };
};

class DistortionModule {
public:
    DistortionModule(juce::AudioProcessorValueTreeState&);
    void prepareToPlay(double sampleRate, int samplesPerBlock);
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&, double);

    static void addParameters(juce::AudioProcessorValueTreeState::ParameterLayout&);
    static DistortionSettings getSettings(juce::AudioProcessorValueTreeState& apvts);
private:
    juce::AudioProcessorValueTreeState& apvts;

    bool isActive = true;
    juce::AudioBuffer<float> wetBuffer;

    void updateDSP();
};

//==============================================================================

/* DistortionModule GUI */

//==============================================================================