/*
  ==============================================================================

    MeterModule.cpp

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

#include "MeterModule.h"

//==============================================================================

/* MeterModule DSP */

//==============================================================================

MeterModuleDSP::MeterModuleDSP(juce::AudioProcessorValueTreeState& _apvts, juce::String _type)
    : DSPModule(_apvts), type(_type) {
    setChainPosition(type == "Input" ? 0 : 9);
}

//DSPModule* MeterModuleDSP::clone()
//{
//    return new MeterModuleDSP(*this);
//}

juce::String MeterModuleDSP::getType()
{
    return type;
}

MeterSettings MeterModuleDSP::getSettings(juce::AudioProcessorValueTreeState& apvts, juce::String type)
{
    MeterSettings settings;
    juce::String search = type + " Meter Level";
    settings.levelInDecibel = apvts.getRawParameterValue(search)->load();

    return settings;
}

void MeterModuleDSP::addParameters(juce::AudioProcessorValueTreeState::ParameterLayout& layout)
{
    layout.add(std::move(std::make_unique<juce::AudioParameterFloat>("Input Meter Level", "Input Meter Level", juce::NormalisableRange<float>(-60.f, 10.f, 0.5f), 0.f, "Input Meter")));
    layout.add(std::move(std::make_unique<juce::AudioParameterFloat>("Output Meter Level", "Output Meter Level", juce::NormalisableRange<float>(-60.f, 10.f, 0.5f), 0.f, "Output Meter")));
}

foleys::LevelMeterSource& MeterModuleDSP::getMeterSource()
{
    return meterSource;
}

void MeterModuleDSP::setModuleType()
{
    moduleType = ModuleType::Meter;
}

void MeterModuleDSP::updateDSPState(double)
{
    auto settings = getSettings(apvts, type);
    level.setGainDecibels(settings.levelInDecibel);
}

void MeterModuleDSP::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    juce::dsp::ProcessSpec spec;

    spec.maximumBlockSize = samplesPerBlock;
    spec.numChannels = 2;
    spec.sampleRate = sampleRate;

    level.prepare(spec);

    meterSource.resize(2, sampleRate * 0.1 / samplesPerBlock);
}

void MeterModuleDSP::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer&, double sampleRate)
{
    updateDSPState(sampleRate);

    juce::dsp::AudioBlock<float> block(buffer);
    juce::dsp::ProcessContextReplacing<float> context(block);

    level.process(context);

    meterSource.measureBlock(buffer);
}

//==============================================================================

/* MeterModule GUI */

//==============================================================================

MeterModuleGUI::MeterModuleGUI(PluginState& p, juce::String _type, foleys::LevelMeterSource* ms)
    : GUIModule(), pluginState(p), type(_type),
    levelSlider(*pluginState.apvts.getParameter(type + " Meter Level"), "dB"),
    levelSliderAttachment(pluginState.apvts, type + " Meter Level", levelSlider)
{
    title.setText(type, juce::dontSendNotification);
    title.setFont(juce::Font("Courier New", 20, juce::Font::bold));

    // meter custom colors
    lnf.setColour(foleys::LevelMeter::lmBackgroundColour, juce::Colours::black);
    lnf.setColour(foleys::LevelMeter::lmTicksColour, juce::Colours::white);
    lnf.setColour(foleys::LevelMeter::lmMeterGradientMidColour, juce::Colours::darkorange);
    lnf.setColour(foleys::LevelMeter::lmMeterGradientLowColour, juce::Colour(0, 240, 48));
    meter.setLookAndFeel(&lnf);
    meter.setMeterSource(ms);

    levelSlider.labels.add({ 0.f, "-60dB" });
    levelSlider.labels.add({ 1.f, "10dB" });

    // tooltips
    levelSlider.setTooltip("Select the amount of gain to be applied to the signal");

    for (auto* comp : getAllComps())
    {
        addAndMakeVisible(comp);
    }
}

MeterModuleGUI::~MeterModuleGUI()
{
    meter.setLookAndFeel(nullptr);
}

juce::String MeterModuleGUI::getType()
{
    return type;
}

std::vector<juce::Component*> MeterModuleGUI::getAllComps()
{
    return {
        &title,
        &meter,
        &levelSlider
    };
}

std::vector<juce::Component*> MeterModuleGUI::getParamComps()
{
    return std::vector<juce::Component*>();
}

void MeterModuleGUI::resized()
{
    auto bounds = getContentRenderArea();

    auto titleArea = bounds.removeFromTop(bounds.getHeight() * (1.f / 8.f));
    auto meterArea = bounds.removeFromTop(bounds.getHeight() * (3.f / 4.f));
    meterArea.reduce(4, 4);
    bounds.reduce(4, 4);

    juce::Rectangle<int> renderArea;
    renderArea.setSize(bounds.getHeight(), bounds.getHeight());

    title.setBounds(titleArea);
    title.setJustificationType(juce::Justification::centred);

    meter.setBounds(meterArea);

    renderArea.setCentre(bounds.getCentre());
    renderArea.setY(bounds.getTopLeft().getY());
    levelSlider.setBounds(renderArea);
}
