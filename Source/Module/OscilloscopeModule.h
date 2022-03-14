/*
  ==============================================================================

    OscilloscopeModule.h

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

/* OscilloscopeModule DSP */

//==============================================================================

struct OscilloscopeSettings {
    float hZoom{ 0 };
    float vZoom{ 0 };
    bool bypassed{ false };
};

class OscilloscopeModuleDSP : public DSPModule {
public:
    OscilloscopeModuleDSP(juce::AudioProcessorValueTreeState& _apvts);

    drow::AudioOscilloscope* getLeftOscilloscope();
    drow::AudioOscilloscope* getRightOscilloscope();

    static OscilloscopeSettings getSettings(juce::AudioProcessorValueTreeState& apvts, unsigned int chainPosition);
    static void addParameters(juce::AudioProcessorValueTreeState::ParameterLayout& layout);

    void setModuleType() override;

    void updateDSPState(double sampleRate) override;
    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages, double sampleRate) override;

private:
    bool bypassed = false;
    drow::AudioOscilloscope leftOscilloscope;
    drow::AudioOscilloscope rightOscilloscope;
};

//==============================================================================

/* OscilloscopeModule GUI */

//==============================================================================

class OscilloscopeModuleGUI : public GUIModule {
public:
    OscilloscopeModuleGUI(PluginState& p, drow::AudioOscilloscope* _leftOscilloscope, drow::AudioOscilloscope* _rightOscilloscope, unsigned int parameterNumber);
    ~OscilloscopeModuleGUI();

    std::vector<juce::Component*> getAllComps() override;
    virtual std::vector<juce::Component*> getParamComps() override;
    virtual void updateParameters(const juce::Array<juce::var>& values) override;
    virtual void resetParameters(unsigned int parameterNumber) override;
    virtual juce::Array<juce::var> getParamValues() override;

    void paintOverChildren(Graphics& g) override;
    void resized() override;

private:

    using APVTS = juce::AudioProcessorValueTreeState;
    using Attachment = APVTS::SliderAttachment;
    using ButtonAttachment = APVTS::ButtonAttachment;

    PluginState& pluginState;

    juce::Label title;

    juce::Label hZoomLabel,
        vZoomLabel;

    RotarySliderWithLabels hZoomSlider, vZoomSlider;
    Attachment hZoomSliderAttachment, vZoomSliderAttachment;
    
    
    juce::TextButton freezeButton{ "Freeze" };
    PowerButton bypassButton;

    ButtonAttachment bypassButtonAttachment;

    ButtonsLookAndFeel lnf;
    ModuleLookAndFeel freezeLnf;

    drow::AudioOscilloscope* leftOscilloscope;
    drow::AudioOscilloscope* rightOscilloscope;
};