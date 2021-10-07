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

    drow::AudioOscilloscope* getOscilloscope();

    static OscilloscopeSettings getSettings(juce::AudioProcessorValueTreeState& apvts, unsigned int chainPosition);
    static void addParameters(juce::AudioProcessorValueTreeState::ParameterLayout& layout);

    void setModuleType() override;

    void updateDSPState(double sampleRate) override;
    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages, double sampleRate) override;

private:
    bool bypassed = false;
    drow::AudioOscilloscope oscilloscope;
};

//==============================================================================

/* OscilloscopeModule GUI */

//==============================================================================

class OscilloscopeModuleGUI : public GUIModule {
public:
    OscilloscopeModuleGUI(BiztortionAudioProcessor& p, drow::AudioOscilloscope* _oscilloscope, unsigned int chainPosition);
    ~OscilloscopeModuleGUI();

    std::vector<juce::Component*> getComps() override;
    void paint(juce::Graphics& g) override;
    void resized() override;

private:

    using APVTS = juce::AudioProcessorValueTreeState;
    using Attachment = APVTS::SliderAttachment;
    using ButtonAttachment = APVTS::ButtonAttachment;

    // This reference is provided as a quick way for your editor to
    // access the processor object that created it.
    BiztortionAudioProcessor& audioProcessor;
    juce::Label title;

    juce::Label hZoomLabel,
        vZoomLabel;

    // TODO : add a Freeze button

    RotarySliderWithLabels hZoomSlider, vZoomSlider;
    Attachment hZoomSliderAttachment, vZoomSliderAttachment;
    
    juce::LookAndFeel_V4 freezeLnf;
    juce::TextButton freezeButton{ "Freeze" };
    PowerButton bypassButton;
    ButtonAttachment bypassButtonAttachment;
    ButtonsLookAndFeel lnf;

    // TODO : add a sample-and-hold algorithm to drow::oscilloscope for a better visual + fix visual glitches
    drow::AudioOscilloscope* oscilloscope;
};