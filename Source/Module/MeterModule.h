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

#include "../Shared/GUIStuff.h"
#include "../Module/DSPModule.h"
#include "../Module/GUIModule.h"
#include "../Shared/PluginState.h"

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
    MeterModuleGUI(PluginState& p, juce::String type, foleys::LevelMeterSource* ms);
    ~MeterModuleGUI();

    juce::String getType();

    std::vector<juce::Component*> getAllComps() override;
    std::vector<juce::Component*> getParamComps() override;
    // useless methods because (at this moment) this is not a module usable in the processing chain
    virtual void updateParameters(const juce::Array<juce::var>& values) override {};
    virtual void resetParameters(unsigned int parameterNumber) override {};
    virtual juce::Array<juce::var> getParamValues() override { return juce::Array<juce::var>(); };

    void resized() override;

private:

    using APVTS = juce::AudioProcessorValueTreeState;
    using Attachment = APVTS::SliderAttachment;

    PluginState& pluginState;

    // input/output meter
    juce::String type;
    //gain
    RotarySliderWithLabels levelSlider;
    Attachment levelSliderAttachment;

    juce::Label title;

    foleys::LevelMeterLookAndFeel lnf;
    foleys::LevelMeter meter{ foleys::LevelMeter::MeterFlags::Default };

};