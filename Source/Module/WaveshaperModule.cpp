/*
  ==============================================================================

    WaveshaperModule.cpp

    Copyright (c) 2021 KillBizz - Gabriel Bizzo

  ==============================================================================
*/

/*
  ==============================================================================

    Copyright (c) 2018 Daniele Filaretti
    Content: the original Waveshaper Algorithm
    Source: https://github.com/dfilaretti/waveshaper-demo

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

#include "WaveshaperModule.h"
#include "../PluginProcessor.h"

//==============================================================================

/* WaveshaperModule DSP */

//==============================================================================

WaveshaperModuleDSP::WaveshaperModuleDSP(juce::AudioProcessorValueTreeState& _apvts)
    : DSPModule(_apvts)
{
}

void WaveshaperModuleDSP::setModuleType()
{
    moduleType = ModuleType::Waveshaper;
}

void WaveshaperModuleDSP::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    wetBuffer.setSize(2, samplesPerBlock, false, true, true); // clears
    tempBuffer.setSize(2, samplesPerBlock, false, true, true); // clears
    updateDSPState(sampleRate);
    /*oversampler.initProcessing(samplesPerBlock);
    oversampler.reset();*/
}

void WaveshaperModuleDSP::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages, double sampleRate)
{

    updateDSPState(sampleRate);

    if (!bypassed) {

        // SAFETY CHECK :::: since some hosts will change buffer sizes without calling prepToPlay (ex: Bitwig)
        int numSamples = buffer.getNumSamples();
        if (wetBuffer.getNumSamples() != numSamples)
        {
            wetBuffer.setSize(2, numSamples, false, true, true); // clears
            tempBuffer.setSize(2, numSamples, false, true, true); // clears
        }
        
        // Wet Buffer feeding
        for (auto channel = 0; channel < 2; channel++)
            wetBuffer.copyFrom(channel, 0, buffer, channel, 0, numSamples);

        // Oversampling wetBuffer for processing
        /*juce::dsp::AudioBlock<float> block(wetBuffer);
        auto leftBlock = block.getSingleChannelBlock(0);
        auto rightBlock = block.getSingleChannelBlock(1);
        juce::dsp::ProcessContextReplacing<float> leftContext(leftBlock);
        juce::dsp::ProcessContextReplacing<float> rightContext(rightBlock);
        auto oversampledLeftBlock = oversampler.processSamplesUp(leftContext.getInputBlock());
        auto oversampledRightBlock = oversampler.processSamplesUp(rightContext.getInputBlock());*/

        // Drive
        driveGain.applyGain(wetBuffer, numSamples);

        // Temp Buffer feeding for applying asymmetry
        for (auto channel = 0; channel < 2; channel++)
            tempBuffer.copyFrom(channel, 0, wetBuffer, channel, 0, numSamples);

        // Waveshaper
        for (auto channel = 0; channel < 2; channel++)
        {
            auto* channelData = wetBuffer.getWritePointer(channel);

            for (auto i = 0; i < numSamples; i++)
                channelData[i] = tanhAmp.getNextValue() * std::tanh(channelData[i] * tanhSlope.getNextValue())
                + sineAmp.getNextValue() * std::sin(channelData[i] * sineFreq.getNextValue());
        }

        // Sampling back down the wetBuffer after processing
        /*oversampler.processSamplesDown(leftContext.getOutputBlock());
        oversampler.processSamplesDown(rightContext.getOutputBlock());

        auto* channelData = wetBuffer.getWritePointer(0);
        for (auto i = 0; i < numSamples; i++)
            channelData[i] = leftContext.getOutputBlock().getSample(0, i);
        channelData = wetBuffer.getWritePointer(1);
        for (auto i = 0; i < numSamples; i++)
            channelData[i] = rightContext.getOutputBlock().getSample(0, i);*/

        applyAsymmetry(tempBuffer, wetBuffer, symmetry.getNextValue(), bias.getNextValue(), numSamples);

        // Mixing buffers
        dryGain.applyGain(buffer, numSamples);
        wetGain.applyGain(tempBuffer, numSamples);

        for (auto channel = 0; channel < 2; channel++)
            buffer.addFrom(channel, 0, tempBuffer, channel, 0, numSamples);

        // Hard clipper for limiting
        for (auto channel = 0; channel < 2; channel++)
        {
            auto* channelData = buffer.getWritePointer(channel);

            for (auto i = 0; i < numSamples; i++)
                channelData[i] = juce::jlimit(-1.f, 1.f, channelData[i]);
        }
    }
}

void WaveshaperModuleDSP::addParameters(juce::AudioProcessorValueTreeState::ParameterLayout& layout)
{
    using namespace juce;

    for (int i = 1; i < 9; ++i) {
        layout.add(std::make_unique<juce::AudioParameterFloat>("Waveshaper Drive " + std::to_string(i), "Waveshaper Drive " + std::to_string(i), NormalisableRange<float>(0.f, 40.f, 0.01f), 0.f, "Waveshaper " + std::to_string(i)));
        layout.add(std::make_unique<AudioParameterFloat>("Waveshaper Mix " + std::to_string(i), "Waveshaper Mix " + std::to_string(i), NormalisableRange<float>(0.f, 100.f, 0.01f), 100.f, "Waveshaper " + std::to_string(i)));
        layout.add(std::make_unique<AudioParameterFloat>("Waveshaper Symmetry " + std::to_string(i), "Waveshaper Symmetry " + std::to_string(i), NormalisableRange<float>(-100.f, 100.f, 1.f), 0.f, "Waveshaper " + std::to_string(i)));
        layout.add(std::make_unique<AudioParameterFloat>("Waveshaper Bias " + std::to_string(i), "Waveshaper Bias " + std::to_string(i), NormalisableRange<float>(-90.f, 90.f, 1.f), 0.f, "Waveshaper " + std::to_string(i)));
        layout.add(std::make_unique<AudioParameterFloat>("Waveshaper Tanh Amp " + std::to_string(i), "Waveshaper Tanh Amp " + std::to_string(i), NormalisableRange<float>(0.f, 100.f, 0.01f), 100.f, "Waveshaper " + std::to_string(i)));
        layout.add(std::make_unique<AudioParameterFloat>("Waveshaper Tanh Slope " + std::to_string(i), "Waveshaper Tanh Slope " + std::to_string(i), NormalisableRange<float>(1.f, 15.f, 0.01f), 1.f, "Waveshaper " + std::to_string(i)));
        layout.add(std::make_unique<AudioParameterFloat>("Waveshaper Sine Amp " + std::to_string(i), "Waveshaper Sin Amp " + std::to_string(i), NormalisableRange<float>(0.f, 100.f, 0.01f), 0.f, "Waveshaper " + std::to_string(i)));
        layout.add(std::make_unique<AudioParameterFloat>("Waveshaper Sine Freq " + std::to_string(i), "Waveshaper Sin Freq " + std::to_string(i), NormalisableRange<float>(0.5f, 100.f, 0.01f), 0.5f, "Waveshaper " + std::to_string(i)));
        layout.add(std::make_unique<AudioParameterBool>("Waveshaper Bypassed " + std::to_string(i), "Waveshaper Bypassed " + std::to_string(i), false, "Waveshaper " + std::to_string(i)));
    }
}

WaveshaperSettings WaveshaperModuleDSP::getSettings(juce::AudioProcessorValueTreeState& apvts, unsigned int chainPosition)
{
    WaveshaperSettings settings;

    settings.drive = apvts.getRawParameterValue("Waveshaper Drive " + std::to_string(chainPosition))->load();
    settings.mix = apvts.getRawParameterValue("Waveshaper Mix " + std::to_string(chainPosition))->load();
    settings.symmetry = apvts.getRawParameterValue("Waveshaper Symmetry " + std::to_string(chainPosition))->load();
    settings.bias = apvts.getRawParameterValue("Waveshaper Bias " + std::to_string(chainPosition))->load();
    settings.tanhAmp = apvts.getRawParameterValue("Waveshaper Tanh Amp " + std::to_string(chainPosition))->load();
    settings.tanhSlope = apvts.getRawParameterValue("Waveshaper Tanh Slope " + std::to_string(chainPosition))->load();
    settings.sinAmp = apvts.getRawParameterValue("Waveshaper Sine Amp " + std::to_string(chainPosition))->load();
    settings.sinFreq = apvts.getRawParameterValue("Waveshaper Sine Freq " + std::to_string(chainPosition))->load();
    settings.bypassed = apvts.getRawParameterValue("Waveshaper Bypassed " + std::to_string(chainPosition))->load() > 0.5f;

    return settings;
}

void WaveshaperModuleDSP::updateDSPState(double)
{
    auto settings = getSettings(apvts, getChainPosition());

    bypassed = settings.bypassed;

    auto mix = settings.mix * 0.01f;
    dryGain.setTargetValue(1.f - mix);
    wetGain.setTargetValue(mix);
    driveGain.setTargetValue(juce::Decibels::decibelsToGain(settings.drive));
    symmetry.setTargetValue(settings.symmetry * 0.01f);
    bias.setTargetValue(settings.bias * 0.01f);

    tanhAmp.setTargetValue(settings.tanhAmp * 0.01f);
    tanhSlope.setTargetValue(settings.tanhSlope);
    sineAmp.setTargetValue(settings.sinAmp * 0.01f);
    sineFreq.setTargetValue(settings.sinFreq);
}

//==============================================================================

/* WaveshaperModule GUI */

//==============================================================================

WaveshaperModuleGUI::WaveshaperModuleGUI(BiztortionAudioProcessor& p, unsigned int chainPosition)
    : GUIModule(), audioProcessor(p),
    driveSlider(*audioProcessor.apvts.getParameter("Waveshaper Drive " + std::to_string(chainPosition)), "dB"),
    mixSlider(*audioProcessor.apvts.getParameter("Waveshaper Mix " + std::to_string(chainPosition)), "%"),
    symmetrySlider(*audioProcessor.apvts.getParameter("Waveshaper Symmetry " + std::to_string(chainPosition)), "%"),
    biasSlider(*audioProcessor.apvts.getParameter("Waveshaper Bias " + std::to_string(chainPosition)), ""),
    tanhAmpSlider(*audioProcessor.apvts.getParameter("Waveshaper Tanh Amp " + std::to_string(chainPosition)), ""),
    tanhSlopeSlider(*audioProcessor.apvts.getParameter("Waveshaper Tanh Slope " + std::to_string(chainPosition)), ""),
    sineAmpSlider(*audioProcessor.apvts.getParameter("Waveshaper Sine Amp " + std::to_string(chainPosition)), ""),
    sineFreqSlider(*audioProcessor.apvts.getParameter("Waveshaper Sine Freq " + std::to_string(chainPosition)), ""),
    transferFunctionGraph(p, chainPosition),
    driveSliderAttachment(audioProcessor.apvts, "Waveshaper Drive " + std::to_string(chainPosition), driveSlider),
    mixSliderAttachment(audioProcessor.apvts, "Waveshaper Mix " + std::to_string(chainPosition), mixSlider),
    symmetrySliderAttachment(audioProcessor.apvts, "Waveshaper Symmetry " + std::to_string(chainPosition), symmetrySlider),
    biasSliderAttachment(audioProcessor.apvts, "Waveshaper Bias " + std::to_string(chainPosition), biasSlider),
    tanhAmpSliderAttachment(audioProcessor.apvts, "Waveshaper Tanh Amp " + std::to_string(chainPosition), tanhAmpSlider),
    tanhSlopeSliderAttachment(audioProcessor.apvts, "Waveshaper Tanh Slope " + std::to_string(chainPosition), tanhSlopeSlider),
    sineAmpSliderAttachment(audioProcessor.apvts, "Waveshaper Sine Amp " + std::to_string(chainPosition), sineAmpSlider),
    sineFreqSliderAttachment(audioProcessor.apvts, "Waveshaper Sine Freq " + std::to_string(chainPosition), sineFreqSlider),
    bypassButtonAttachment(audioProcessor.apvts, "Waveshaper Bypassed " + std::to_string(chainPosition), bypassButton)
{
    // title setup
    title.setText("Waveshaper", juce::dontSendNotification);
    title.setFont(juce::Font("Courier New", 24, 0));

    // labels
    driveLabel.setText("Drive", juce::dontSendNotification);
    driveLabel.setFont(juce::Font("Courier New", 12, 0));
    mixLabel.setText("Mix", juce::dontSendNotification);
    mixLabel.setFont(juce::Font("Courier New", 12, 0));
    symmetryLabel.setText("Symmetry", juce::dontSendNotification);
    symmetryLabel.setFont(juce::Font("Courier New", 12, 0));
    biasLabel.setText("Bias", juce::dontSendNotification);
    biasLabel.setFont(juce::Font("Courier New", 12, 0));
    tanhAmpLabel.setText("Tanh Amp", juce::dontSendNotification);
    tanhAmpLabel.setFont(juce::Font("Courier New", 12, 0));
    tanhSlopeLabel.setText("Tanh Slope", juce::dontSendNotification);
    tanhSlopeLabel.setFont(juce::Font("Courier New", 12, 0));
    sineAmpLabel.setText("Sin Amp", juce::dontSendNotification);
    sineAmpLabel.setFont(juce::Font("Courier New", 12, 0));
    sineFreqLabel.setText("Sin Freq", juce::dontSendNotification);
    sineFreqLabel.setFont(juce::Font("Courier New", 12, 0));

    driveSlider.labels.add({ 0.f, "0dB" });
    driveSlider.labels.add({ 1.f, "40dB" });
    mixSlider.labels.add({ 0.f, "0%" });
    mixSlider.labels.add({ 1.f, "100%" });
    symmetrySlider.labels.add({ 0.f, "-100%" });
    symmetrySlider.labels.add({ 1.f, "+100%" });
    biasSlider.labels.add({ 0.f, "-0.9" });
    biasSlider.labels.add({ 1.f, "+0.9" });
    tanhAmpSlider.labels.add({ 0.f, "0" });
    tanhAmpSlider.labels.add({ 1.f, "100" });
    tanhSlopeSlider.labels.add({ 0.f, "1" });
    tanhSlopeSlider.labels.add({ 1.f, "15" });
    sineAmpSlider.labels.add({ 0.f, "0" });
    sineAmpSlider.labels.add({ 1.f, "100" });
    sineFreqSlider.labels.add({ 0.f, "0.5" });
    sineFreqSlider.labels.add({ 1.f, "100" });

    bypassButton.setLookAndFeel(&lnf);

    auto safePtr = juce::Component::SafePointer<WaveshaperModuleGUI>(this);
    bypassButton.onClick = [safePtr]()
    {
        if (auto* comp = safePtr.getComponent())
        {
            auto bypassed = comp->bypassButton.getToggleState();
            comp->handleParamCompsEnablement(bypassed);
        }
    };

    // tooltips
    bypassButton.setTooltip("Bypass this module");
    driveSlider.setTooltip("Select the amount of gain to be applied to the module input signal");
    mixSlider.setTooltip("Select the blend between the unprocessed and processed signal");
    symmetrySlider.setTooltip("Apply the signal processing to the positive or negative area of the waveform");
    biasSlider.setTooltip("Set the the value which determines the bias between the positive or negative area of the waveform");
    tanhAmpSlider.setTooltip("Set the hyperbolic tangent function amplitude");
    tanhSlopeSlider.setTooltip("Set the hyperbolic tangent function slope");
    sineAmpSlider.setTooltip("Set the sine function amplitude");
    sineFreqSlider.setTooltip("Set the sine function frequency");

    for (auto* comp : getAllComps())
    {
        addAndMakeVisible(comp);
    }

    handleParamCompsEnablement(bypassButton.getToggleState());
}

WaveshaperModuleGUI::~WaveshaperModuleGUI()
{
    bypassButton.setLookAndFeel(nullptr);
}

void WaveshaperModuleGUI::paint(juce::Graphics& g)
{
    drawContainer(g);
}

void WaveshaperModuleGUI::resized()
{
    auto waveshaperArea = getContentRenderArea();

    // bypass
    auto temp = waveshaperArea;
    auto bypassButtonArea = temp.removeFromTop(25);

    bypassButtonArea.setWidth(35.f);
    bypassButtonArea.setX(145.f);
    bypassButtonArea.setY(20.f);

    bypassButton.setBounds(bypassButtonArea);

    auto titleAndBypassArea = waveshaperArea.removeFromTop(30);
    titleAndBypassArea.translate(0, 4);

    auto waveshaperGraphArea = waveshaperArea.removeFromLeft(waveshaperArea.getWidth() * (4.f / 10.f));
    waveshaperGraphArea.reduce(10, 10);

    waveshaperArea.translate(0, 8);

    auto topArea = waveshaperArea.removeFromTop(waveshaperArea.getHeight() * (1.f / 2.f));
    auto bottomArea = waveshaperArea;

    auto topLabelsArea = topArea.removeFromTop(12);
    auto bottomLabelsArea = bottomArea.removeFromTop(12);

    // label areas
    temp = topLabelsArea.removeFromLeft(topLabelsArea.getWidth() * (1.f / 2.f));
    auto driveLabelArea = temp.removeFromLeft(temp.getWidth() * (1.f / 2.f));
    auto mixLabelArea = temp;
    auto symmetryLabelArea = topLabelsArea.removeFromLeft(topLabelsArea.getWidth() * (1.f / 2.f));
    auto biasLabelArea = topLabelsArea;
    temp = bottomLabelsArea.removeFromLeft(bottomLabelsArea.getWidth() * (1.f / 2.f));
    auto tanhAmpLabelArea = temp.removeFromLeft(temp.getWidth() * (1.f / 2.f));
    auto tanhSlopeLabelArea = temp;
    auto sineAmpLabelArea = bottomLabelsArea.removeFromLeft(bottomLabelsArea.getWidth() * (1.f / 2.f));
    auto sineFreqLabelArea = bottomLabelsArea;

    // slider areas
    temp = topArea.removeFromLeft(topArea.getWidth() * (1.f / 2.f));
    auto driveArea = temp.removeFromLeft(temp.getWidth() * (1.f / 2.f));
    driveArea.reduce(4, 0);
    auto mixArea = temp;
    mixArea.reduce(4, 0);
    auto symmetryArea = topArea.removeFromLeft(topArea.getWidth() * (1.f / 2.f));
    symmetryArea.reduce(4, 0);
    auto biasArea = topArea;
    biasArea.reduce(4, 0);
    temp = bottomArea.removeFromLeft(bottomArea.getWidth() * (1.f / 2.f));
    auto tanhAmpArea = temp.removeFromLeft(temp.getWidth() * (1.f / 2.f));
    tanhAmpArea.reduce(4, 0);
    auto tanhSlopeArea = temp;
    tanhSlopeArea.reduce(4, 0);
    auto sineAmpArea = bottomArea.removeFromLeft(bottomArea.getWidth() * (1.f / 2.f));
    sineAmpArea.reduce(4, 0);
    auto sineFreqArea = bottomArea;
    sineFreqArea.reduce(4, 0);

    
    title.setBounds(titleAndBypassArea);
    title.setJustificationType(juce::Justification::centredBottom);

    transferFunctionGraph.setBounds(waveshaperGraphArea);

    driveSlider.setBounds(driveArea);
    driveLabel.setBounds(driveLabelArea);
    driveLabel.setJustificationType(juce::Justification::centred);

    mixSlider.setBounds(mixArea);
    mixLabel.setBounds(mixLabelArea);
    mixLabel.setJustificationType(juce::Justification::centred);

    symmetrySlider.setBounds(symmetryArea);
    symmetryLabel.setBounds(symmetryLabelArea);
    symmetryLabel.setJustificationType(juce::Justification::centred);

    biasSlider.setBounds(biasArea);
    biasLabel.setBounds(biasLabelArea);
    biasLabel.setJustificationType(juce::Justification::centred);

    tanhAmpSlider.setBounds(tanhAmpArea);
    tanhAmpLabel.setBounds(tanhAmpLabelArea);
    tanhAmpLabel.setJustificationType(juce::Justification::centred);

    tanhSlopeSlider.setBounds(tanhSlopeArea);
    tanhSlopeLabel.setBounds(tanhSlopeLabelArea);
    tanhSlopeLabel.setJustificationType(juce::Justification::centred);

    sineAmpSlider.setBounds(sineAmpArea);
    sineAmpLabel.setBounds(sineAmpLabelArea);
    sineAmpLabel.setJustificationType(juce::Justification::centred);

    sineFreqSlider.setBounds(sineFreqArea);
    sineFreqLabel.setBounds(sineFreqLabelArea);
    sineFreqLabel.setJustificationType(juce::Justification::centred);
}

std::vector<juce::Component*> WaveshaperModuleGUI::getAllComps()
{
    return {
        &title,
        &transferFunctionGraph,
        & driveSlider,
        & mixSlider,
        & symmetrySlider,
        & biasSlider,
        &tanhAmpSlider,
        &tanhSlopeSlider,
        &sineAmpSlider,
        &sineFreqSlider,
        // labels
        & driveLabel,
        & mixLabel,
        & symmetryLabel,
        & biasLabel,
        &tanhAmpLabel,
        &tanhSlopeLabel,
        &sineAmpLabel,
        &sineFreqLabel,
        // bypass
        &bypassButton
    };
}

std::vector<juce::Component*> WaveshaperModuleGUI::getParamComps()
{
    return {
        &driveSlider,
        &mixSlider,
        &symmetrySlider,
        &biasSlider,
        &tanhAmpSlider,
        &tanhSlopeSlider,
        &sineAmpSlider,
        &sineFreqSlider
    };
}