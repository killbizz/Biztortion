/*
  ==============================================================================

    MeterModule.h
    Created: 1 Sep 2021 2:57:12pm
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

struct MeterSettings {
    float levelInDecibel{ 0 };
};

class MeterModuleDSP : public DSPModule {
public:
    MeterModuleDSP(juce::AudioProcessorValueTreeState& _apvts, juce::String _type);

    static MeterSettings getSettings(juce::AudioProcessorValueTreeState& apvts, juce::String type);
    static void addParameters(juce::AudioProcessorValueTreeState::ParameterLayout& layout);
    foleys::LevelMeterSource& getMeterResource();

    void updateDSPState(double sampleRate) override;
    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages, double sampleRate) override;

private:
    juce::String type;
    juce::dsp::Gain<float> level;
    //juce::LinearSmoothedValue<float> level;

    foleys::LevelMeterSource meterSource;
};

//==============================================================================

/* MeterModule GUI */

//==============================================================================

class MeterModuleGUI : public GUIModule {
public:
    MeterModuleGUI(BiztortionAudioProcessor& p, juce::String type);
    ~MeterModuleGUI();

    foleys::LevelMeterSource* getMeterSource();
    std::vector<juce::Component*> getComps() override;

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

    foleys::LevelMeterLookAndFeel lnf;
    foleys::LevelMeter meter{ foleys::LevelMeter::Minimal };

};