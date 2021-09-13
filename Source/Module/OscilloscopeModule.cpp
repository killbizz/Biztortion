/*
  ==============================================================================

    OscilloscopeModule.cpp
    Created: 9 Sep 2021 9:24:50pm
    Author:  gabri

  ==============================================================================
*/

//==============================================================================

/* MeterModule DSP */

//==============================================================================

#include "OscilloscopeModule.h"
#include "../PluginProcessor.h"

OscilloscopeModuleDSP::OscilloscopeModuleDSP(juce::AudioProcessorValueTreeState& _apvts)
    : DSPModule(_apvts)
{
    oscilloscope.clear();
}

drow::AudioOscilloscope* OscilloscopeModuleDSP::getOscilloscope()
{
    return &oscilloscope;
}

OscilloscopeSettings OscilloscopeModuleDSP::getSettings(juce::AudioProcessorValueTreeState& apvts)
{
    OscilloscopeSettings settings;
    settings.hZoom = apvts.getRawParameterValue("Oscilloscope H Zoom")->load();
    settings.vZoom = apvts.getRawParameterValue("Oscilloscope V Zoom")->load();

    return settings;
}

void OscilloscopeModuleDSP::addParameters(juce::AudioProcessorValueTreeState::ParameterLayout& layout)
{
    layout.add(std::move(std::make_unique<juce::AudioParameterFloat>("Oscilloscope H Zoom", "Oscilloscope H Zoom", juce::NormalisableRange<float>(0.f, 1.f, 0.01f), 0.f, "Oscilloscope")));
    layout.add(std::move(std::make_unique<juce::AudioParameterFloat>("Oscilloscope V Zoom", "Oscilloscope V Zoom", juce::NormalisableRange<float>(0.f, 1.f, 0.01f), 1.f, "Oscilloscope")));
}

void OscilloscopeModuleDSP::updateDSPState(double sampleRate)
{
    auto settings = getSettings(apvts);
    oscilloscope.setHorizontalZoom(settings.hZoom);
    oscilloscope.setVerticalZoom(settings.vZoom);
}

void OscilloscopeModuleDSP::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    oscilloscope.clear();
    updateDSPState(sampleRate);
}

void OscilloscopeModuleDSP::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages, double sampleRate)
{
    updateDSPState(sampleRate);
    oscilloscope.processBlock(buffer.getReadPointer(0), buffer.getNumSamples());
}

//==============================================================================

/* MeterModule GUI */

//==============================================================================

OscilloscopeModuleGUI::OscilloscopeModuleGUI(BiztortionAudioProcessor& p, drow::AudioOscilloscope* _oscilloscope)
    : GUIModule(), audioProcessor(p), oscilloscope(_oscilloscope),
    hZoomSlider(*audioProcessor.apvts.getParameter("Oscilloscope H Zoom"), ""),
    vZoomSlider(*audioProcessor.apvts.getParameter("Oscilloscope V Zoom"), ""),
    hZoomSliderAttachment(audioProcessor.apvts, "Oscilloscope H Zoom", hZoomSlider),
    vZoomSliderAttachment(audioProcessor.apvts, "Oscilloscope V Zoom", vZoomSlider)
{
    hZoomSlider.labels.add({ 0.f, "0" });
    hZoomSlider.labels.add({ 1.f, "1" });
    vZoomSlider.labels.add({ 0.f, "0" });
    vZoomSlider.labels.add({ 1.f, "1" });

    for (auto* comp : getComps())
    {
        addAndMakeVisible(comp);
    }
}

std::vector<juce::Component*> OscilloscopeModuleGUI::getComps()
{
    return {
        oscilloscope,
        &hZoomSlider,
        &vZoomSlider
    };
}

void OscilloscopeModuleGUI::paint(juce::Graphics& g)
{
    drawContainer(g);
}

void OscilloscopeModuleGUI::resized()
{
    auto oscilloscopeArea = getContentRenderArea();

    auto graphArea = oscilloscopeArea.removeFromLeft(oscilloscopeArea.getWidth() * (1.f / 2.f));
    graphArea.reduce(10, 10);
    auto hZoomArea = oscilloscopeArea.removeFromLeft(oscilloscopeArea.getWidth() * (1.f / 2.f));
    auto vZoomArea = oscilloscopeArea;

    oscilloscope->setBounds(graphArea);
    hZoomSlider.setBounds(hZoomArea);
    vZoomSlider.setBounds(vZoomArea);
}
