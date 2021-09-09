/*
  ==============================================================================

    OscilloscopeModule.h
    Created: 9 Sep 2021 9:24:50pm
    Author:  gabri

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>

class BiztortionAudioProcessor;
#include "../Module/GUIModule.h"
#include "../Module/DSPModule.h"
#include "../Component/GUIStuff.h"

//==============================================================================

/* MeterModule DSP */

//==============================================================================

struct OscilloscopeSettings {
    float hZoom{ 0 };
    float vZoom{ 0 };
};

class OscilloscopeModuleDSP : public DSPModule {
public:
    OscilloscopeModuleDSP(juce::AudioProcessorValueTreeState& _apvts, drow::AudioOscilloscope* _oscilloscope);

    static OscilloscopeSettings getSettings(juce::AudioProcessorValueTreeState& apvts);
    static void addParameters(juce::AudioProcessorValueTreeState::ParameterLayout& layout);

    void updateDSPState(double sampleRate) override;
    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages, double sampleRate) override;

private:
    drow::AudioOscilloscope* oscilloscope;
};

//==============================================================================

/* MeterModule GUI */

//==============================================================================

class OscilloscopeModuleGUI : public GUIModule {
public:
    OscilloscopeModuleGUI(BiztortionAudioProcessor& p, unsigned int gridPosition);

    drow::AudioOscilloscope* getOscilloscope();

    std::vector<juce::Component*> getComps() override;
    void paint(juce::Graphics& g) override;
    void resized() override;

private:

    using APVTS = juce::AudioProcessorValueTreeState;
    using Attachment = APVTS::SliderAttachment;

    // This reference is provided as a quick way for your editor to
    // access the processor object that created it.
    BiztortionAudioProcessor& audioProcessor;

    RotarySliderWithLabels hZoomSlider, vZoomSlider;
    Attachment hZoomSliderAttachment, vZoomSliderAttachment;
    drow::AudioOscilloscope oscilloscope;

};