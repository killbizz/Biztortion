/*
  ==============================================================================

    FFTAnalyzerComponent.h

    Copyright (c) 2021 KillBizz - Gabriel Bizzo

  ==============================================================================
*/

/*
  ==============================================================================

    Copyright (c) 2021 Charles Schiermeyer
    Content: the original FFT Analyzer Component
    Source: https://github.com/matkatmusic/SimpleEQ

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