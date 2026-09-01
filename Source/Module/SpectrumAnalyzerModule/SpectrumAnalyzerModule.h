/*
  ==============================================================================

    SpectrumAnalyzerModule.h

    Copyright (c) 2022 KillBizz - Gabriel Bizzo

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

#include "../../Component/FFTAnalyzerComponent.h"
#include "../../Shared/GUIStuff.h"
#include "../../Module/DSPModule.h"
#include "../../Module/GUIModule.h"
#include "../../Shared/PluginState.h"

//==============================================================================

/* SpectrumAnalyzerModule DSP */

//==============================================================================

struct SpectrumAnalyzerSettings {
    float hZoom{ 1.f };
    float vZoom{ 1.f };
    bool stereoView{ false };
    bool bypassed{ false };
};

// Analysis-only module: it leaves the audio untouched. The FFT FIFOs it reads from are
// allocated by PluginState and filled by the processor for every module type reported by
// moduleTypeOwnsAnalyzerFIFO(), exactly like the equalizer's built-in analyzer.
class SpectrumAnalyzerModuleDSP : public DSPModule {
public:
    SpectrumAnalyzerModuleDSP(juce::AudioProcessorValueTreeState& _apvts);

    static SpectrumAnalyzerSettings getSettings(juce::AudioProcessorValueTreeState& apvts, unsigned int parameterNumber);
    static void addParameters(juce::AudioProcessorValueTreeState::ParameterLayout& layout);

    void updateDSPState(double sampleRate) override;
    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages, double sampleRate) override;

private:
    bool bypassed = false;
};

//==============================================================================

/* SpectrumAnalyzerModule GUI */

//==============================================================================

class SpectrumAnalyzerModuleGUI : public GUIModule {
public:
    SpectrumAnalyzerModuleGUI(PluginState& p, unsigned int parameterNumber);
    ~SpectrumAnalyzerModuleGUI();

    std::vector<juce::Component*> getAllComps() override;
    std::vector<juce::Component*> getParamComps() override;
    virtual void updateParameters(const juce::Array<juce::var>& values) override;
    virtual void resetParameters(unsigned int parameterNumber) override;
    virtual juce::Array<juce::var> getParamValues() override;

    void paint(juce::Graphics& g) override;
    void resized() override;

private:

    using APVTS = juce::AudioProcessorValueTreeState;
    using Attachment = APVTS::SliderAttachment;
    using ButtonAttachment = APVTS::ButtonAttachment;

    void updateAnalyzerView();
    void updateControlEnablement();
    void updateAnalyzerZoom();

    // set while a wheel/pinch gesture is pushing its zoom into the sliders, to stop the slider
    // callback from applying it back to the graph
    bool applyingZoomFromGesture = false;

    PluginState& pluginState;

    juce::Label title;

    juce::Label hZoomLabel, vZoomLabel;
    RotarySliderWithLabels hZoomSlider, vZoomSlider;
    Attachment hZoomSliderAttachment, vZoomSliderAttachment;

    PowerButton bypassButton;
    StereoViewButton analyzerViewButton;
    juce::TextButton freezeButton{ "Freeze" };

    ButtonAttachment bypassButtonAttachment;
    ButtonAttachment analyzerViewButtonAttachment;

    ButtonsLookAndFeel lnf;
    ModuleLookAndFeel analyzerControlsLnf;

    FFTAnalyzerComponent fftAnalyzerComponent;
};
