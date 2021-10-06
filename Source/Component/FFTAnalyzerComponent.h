/*
  ==============================================================================

    FFTAnalyzerComponent.h
    Created: 24 Aug 2021 5:21:17pm
    Author:  gabri

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "../FFTAnalyzer.h"

class BiztortionAudioProcessor;

struct FFTAnalyzerComponent : juce::Component,
    juce::Timer {

    FFTAnalyzerComponent(BiztortionAudioProcessor& p, unsigned int chainPosition);

    void timerCallback() override;
    void paint(juce::Graphics& g) override;
    void resized() override;
    PathProducer& getLeftPathProducer();
    PathProducer& getRightPathProducer();
    void toggleFFTanaysis(bool shouldEnableFFTanalysis);

private:

    BiztortionAudioProcessor& audioProcessor;
    bool enableFFTanalysis = true;

    juce::Image background;

    // for the FFT analyzer
    PathProducer leftPathProducer, rightPathProducer;

    juce::Rectangle<int> getRenderArea();
    juce::Rectangle<int> getAnalysysArea();
};