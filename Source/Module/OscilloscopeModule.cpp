/*
  ==============================================================================

    OscilloscopeModule.cpp
    Created: 9 Sep 2021 9:24:50pm
    Author:  gabri

  ==============================================================================
*/

//==============================================================================

/* OscilloscopeModule DSP */

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

OscilloscopeSettings OscilloscopeModuleDSP::getSettings(juce::AudioProcessorValueTreeState& apvts, unsigned int chainPosition)
{
    OscilloscopeSettings settings;
    settings.hZoom = apvts.getRawParameterValue("Oscilloscope H Zoom " + std::to_string(chainPosition))->load();
    settings.vZoom = apvts.getRawParameterValue("Oscilloscope V Zoom " + std::to_string(chainPosition))->load();

    return settings;
}

void OscilloscopeModuleDSP::addParameters(juce::AudioProcessorValueTreeState::ParameterLayout& layout)
{
    for (int i = 1; i < 9; ++i) {
        layout.add(std::move(std::make_unique<juce::AudioParameterFloat>("Oscilloscope H Zoom " + std::to_string(i), 
            "Oscilloscope H Zoom " + std::to_string(i), 
            juce::NormalisableRange<float>(0.f, 1.f, 0.01f), 0.f, "Oscilloscope " + std::to_string(i))));
        layout.add(std::move(std::make_unique<juce::AudioParameterFloat>("Oscilloscope V Zoom " + std::to_string(i), 
            "Oscilloscope V Zoom " + std::to_string(i), 
            juce::NormalisableRange<float>(0.f, 1.f, 0.01f), 1.f, "Oscilloscope " + std::to_string(i))));
    }
}

void OscilloscopeModuleDSP::setModuleType()
{
    moduleType = ModuleType::Oscilloscope;
}

void OscilloscopeModuleDSP::updateDSPState(double sampleRate)
{
    auto settings = getSettings(apvts, getChainPosition());
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

/* OscilloscopeModule GUI */

//==============================================================================

OscilloscopeModuleGUI::OscilloscopeModuleGUI(BiztortionAudioProcessor& p, drow::AudioOscilloscope* _oscilloscope, unsigned int chainPosition)
    : GUIModule(), audioProcessor(p), oscilloscope(_oscilloscope),
    hZoomSlider(*audioProcessor.apvts.getParameter("Oscilloscope H Zoom " + std::to_string(chainPosition)), ""),
    vZoomSlider(*audioProcessor.apvts.getParameter("Oscilloscope V Zoom " + std::to_string(chainPosition)), ""),
    hZoomSliderAttachment(audioProcessor.apvts, "Oscilloscope H Zoom " + std::to_string(chainPosition), hZoomSlider),
    vZoomSliderAttachment(audioProcessor.apvts, "Oscilloscope V Zoom " + std::to_string(chainPosition), vZoomSlider)
{
    hZoomSlider.labels.add({ 0.f, "0" });
    hZoomSlider.labels.add({ 1.f, "1" });
    vZoomSlider.labels.add({ 0.f, "0" });
    vZoomSlider.labels.add({ 1.f, "1" });

    for (auto* comp : getComps())
    {
        addAndMakeVisible(comp);
    }

    // TODO : add a sample-and-hold algorithm to drow::oscilloscope for a better visual
    oscilloscope->startTimerHz(60);
}

OscilloscopeModuleGUI::~OscilloscopeModuleGUI()
{
    oscilloscope->stopTimer();
    oscilloscope = nullptr;
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
