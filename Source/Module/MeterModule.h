/*
  ==============================================================================

    MeterModule.h

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

class BiztortionAudioProcessor;
#include "../Module/GUIModule.h"
#include "../Module/DSPModule.h"
#include "../Shared/GUIStuff.h"

//==============================================================================

/* MeterModule DSP */

//==============================================================================

struct MeterSettings {
    float levelInDecibel{ 0 };
};

class MeterModuleDSP : public DSPModule {
public:
    MeterModuleDSP(juce::AudioProcessorValueTreeState& _apvts, juce::String _type);

    juce::String getType();
    static MeterSettings getSettings(juce::AudioProcessorValueTreeState& apvts, juce::String type);
    static void addParameters(juce::AudioProcessorValueTreeState::ParameterLayout& layout);
    foleys::LevelMeterSource& getMeterSource();

    void setModuleType() override;

    void updateDSPState(double sampleRate) override;
    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages, double sampleRate) override;

private:
    juce::String type;
    juce::dsp::Gain<float> level;

    foleys::LevelMeterSource meterSource;
};

//==============================================================================

/* MeterModule GUI */

//==============================================================================

class MeterModuleGUI : public GUIModule {
public:
    MeterModuleGUI(BiztortionAudioProcessor& p, juce::String type);
    ~MeterModuleGUI();

    juce::String getType();
    foleys::LevelMeterSource* getMeterSource();

    std::vector<juce::Component*> getAllComps() override;
    std::vector<juce::Component*> getParamComps() override;
    // useless methods because (at this moment) this is not a module usable in the processing chain
    virtual void updateParameters(GUIModule* moduleToCopy) override {};
    virtual void resetParameters(unsigned int chainPosition) override {};

    void paint(juce::Graphics& g) override;
    void resized() override;

private:

    using APVTS = juce::AudioProcessorValueTreeState;
    using Attachment = APVTS::SliderAttachment;

    // This reference is provided as a quick way for your editor to
    // access the processor object that created it.
    BiztortionAudioProcessor& audioProcessor;
    // input/output meter
    juce::String type;
    //gain
    RotarySliderWithLabels levelSlider;
    Attachment levelSliderAttachment;

    juce::Label meterTitle;

    foleys::LevelMeterLookAndFeel lnf;
    foleys::LevelMeter meter{ foleys::LevelMeter::MeterFlags::Default };

};