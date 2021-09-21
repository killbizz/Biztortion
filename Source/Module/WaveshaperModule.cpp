/*
  ==============================================================================

    WaveshaperModule.cpp
    Created: 26 Aug 2021 12:05:19pm
    Author:  gabri

  ==============================================================================
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
    isActive = true;
    updateDSPState(sampleRate);
    /*oversampler.initProcessing(samplesPerBlock);
    oversampler.reset();*/
}

void WaveshaperModuleDSP::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages, double sampleRate)
{
    if (isActive) {

        // SAFETY CHECK :::: since some hosts will change buffer sizes without calling prepToPlay (ex: Bitwig)
        int numSamples = buffer.getNumSamples();
        if (wetBuffer.getNumSamples() != numSamples)
        {
            wetBuffer.setSize(2, numSamples, false, true, true); // clears
        }

        updateDSPState(sampleRate);
        
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


        // Mixing buffers
        dryGain.applyGain(buffer, numSamples);
        wetGain.applyGain(wetBuffer, numSamples);

        for (auto channel = 0; channel < 2; channel++)
            buffer.addFrom(channel, 0, wetBuffer, channel, 0, numSamples);

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
        layout.add(std::make_unique<juce::AudioParameterFloat>("Waveshaper Drive " + std::to_string(i), "Waveshaper Drive " + std::to_string(i), NormalisableRange<float>(0.f, 40.f, 0.01f), 10.f, "Waveshaper " + std::to_string(i)));
        layout.add(std::make_unique<AudioParameterFloat>("Waveshaper Mix " + std::to_string(i), "Waveshaper Mix " + std::to_string(i), NormalisableRange<float>(0.f, 100.f, 0.01f), 100.f, "Waveshaper " + std::to_string(i)));
        layout.add(std::make_unique<AudioParameterFloat>("Waveshaper Tanh Amp " + std::to_string(i), "Waveshaper Tanh Amp " + std::to_string(i), NormalisableRange<float>(0.f, 100.f, 0.01f), 100.f, "Waveshaper " + std::to_string(i)));
        layout.add(std::make_unique<AudioParameterFloat>("Waveshaper Tanh Slope " + std::to_string(i), "Waveshaper Tanh Slope " + std::to_string(i), NormalisableRange<float>(1.f, 15.f, 0.01f), 1.f, "Waveshaper " + std::to_string(i)));
        layout.add(std::make_unique<AudioParameterFloat>("Waveshaper Sine Amp " + std::to_string(i), "Waveshaper Sin Amp " + std::to_string(i), NormalisableRange<float>(0.f, 100.f, 0.01f), 0.f, "Waveshaper " + std::to_string(i)));
        layout.add(std::make_unique<AudioParameterFloat>("Waveshaper Sine Freq " + std::to_string(i), "Waveshaper Sin Freq " + std::to_string(i), NormalisableRange<float>(0.5f, 100.f, 0.01f), 1.f, "Waveshaper " + std::to_string(i)));
    }
    /*layout.add(std::make_unique<juce::AudioParameterFloat>("Waveshaper Drive", "Waveshaper Drive", NormalisableRange<float>(0.f, 40.f, 0.01f), 10.f, "Waveshaper"));
    layout.add(std::make_unique<AudioParameterFloat>("Waveshaper Mix", "Waveshaper Mix", NormalisableRange<float>(0.f, 100.f, 0.01f), 100.f, "Waveshaper"));
    layout.add(std::make_unique<AudioParameterFloat>("Waveshaper Tanh Amp", "Waveshaper Tanh Amp", NormalisableRange<float>(0.f, 100.f, 0.01f), 100.f, "Waveshaper"));
    layout.add(std::make_unique<AudioParameterFloat>("Waveshaper Tanh Slope", "Waveshaper Tanh Slope", NormalisableRange<float>(1.f, 15.f, 0.01f), 1.f, "Waveshaper"));
    layout.add(std::make_unique<AudioParameterFloat>("Waveshaper Sine Amp", "Waveshaper Sin Amp", NormalisableRange<float>(0.f, 100.f, 0.01f), 0.f, "Waveshaper"));
    layout.add(std::make_unique<AudioParameterFloat>("Waveshaper Sine Freq", "Waveshaper Sin Freq", NormalisableRange<float>(0.5f, 100.f, 0.01f), 1.f, "Waveshaper"));*/
}

WaveshaperSettings WaveshaperModuleDSP::getSettings(juce::AudioProcessorValueTreeState& apvts, unsigned int chainPosition)
{
    WaveshaperSettings settings;

    settings.drive = apvts.getRawParameterValue("Waveshaper Drive " + std::to_string(chainPosition))->load();
    settings.mix = apvts.getRawParameterValue("Waveshaper Mix " + std::to_string(chainPosition))->load();
    settings.tanhAmp = apvts.getRawParameterValue("Waveshaper Tanh Amp " + std::to_string(chainPosition))->load();
    settings.tanhSlope = apvts.getRawParameterValue("Waveshaper Tanh Slope " + std::to_string(chainPosition))->load();
    settings.sinAmp = apvts.getRawParameterValue("Waveshaper Sine Amp " + std::to_string(chainPosition))->load();
    settings.sinFreq = apvts.getRawParameterValue("Waveshaper Sine Freq " + std::to_string(chainPosition))->load();

    return settings;
}

void WaveshaperModuleDSP::updateDSPState(double)
{
    auto settings = getSettings(apvts, getChainPosition());
    driveGain.setTargetValue(juce::Decibels::decibelsToGain(settings.drive));

    tanhAmp.setTargetValue(settings.tanhAmp * 0.01f);
    tanhSlope.setTargetValue(settings.tanhSlope);
    sineAmp.setTargetValue(settings.sinAmp * 0.01f);
    sineFreq.setTargetValue(settings.sinFreq);

    auto mix = settings.mix * 0.01f;
    dryGain.setTargetValue(1.f - mix);
    wetGain.setTargetValue(mix);
}

//==============================================================================

/* WaveshaperModule GUI */

//==============================================================================

WaveshaperModuleGUI::WaveshaperModuleGUI(BiztortionAudioProcessor& p, unsigned int chainPosition)
    : GUIModule(), audioProcessor(p),
    waveshaperDriveSlider(*audioProcessor.apvts.getParameter("Waveshaper Drive " + std::to_string(chainPosition)), "dB"),
    waveshaperMixSlider(*audioProcessor.apvts.getParameter("Waveshaper Mix " + std::to_string(chainPosition)), "%"),
    tanhAmpSlider(*audioProcessor.apvts.getParameter("Waveshaper Tanh Amp " + std::to_string(chainPosition)), ""),
    tanhSlopeSlider(*audioProcessor.apvts.getParameter("Waveshaper Tanh Slope " + std::to_string(chainPosition)), ""),
    sineAmpSlider(*audioProcessor.apvts.getParameter("Waveshaper Sine Amp " + std::to_string(chainPosition)), ""),
    sineFreqSlider(*audioProcessor.apvts.getParameter("Waveshaper Sine Freq " + std::to_string(chainPosition)), ""),
    transferFunctionGraph(p, chainPosition),
    waveshaperDriveSliderAttachment(audioProcessor.apvts, "Waveshaper Drive " + std::to_string(chainPosition), waveshaperDriveSlider),
    waveshaperMixSliderAttachment(audioProcessor.apvts, "Waveshaper Mix " + std::to_string(chainPosition), waveshaperMixSlider),
    tanhAmpSliderAttachment(audioProcessor.apvts, "Waveshaper Tanh Amp " + std::to_string(chainPosition), tanhAmpSlider),
    tanhSlopeSliderAttachment(audioProcessor.apvts, "Waveshaper Tanh Slope " + std::to_string(chainPosition), tanhSlopeSlider),
    sineAmpSliderAttachment(audioProcessor.apvts, "Waveshaper Sine Amp " + std::to_string(chainPosition), sineAmpSlider),
    sineFreqSliderAttachment(audioProcessor.apvts, "Waveshaper Sine Freq " + std::to_string(chainPosition), sineFreqSlider)
{
    // labels
    waveshaperDriveLabel.setText("Drive", juce::dontSendNotification);
    waveshaperDriveLabel.setFont(10);
    waveshaperMixLabel.setText("Mix", juce::dontSendNotification);
    waveshaperMixLabel.setFont(10);
    tanhAmpLabel.setText("Tanh Amp", juce::dontSendNotification);
    tanhAmpLabel.setFont(10);
    tanhSlopeLabel.setText("Tanh Slope", juce::dontSendNotification);
    tanhSlopeLabel.setFont(10);
    sineAmpLabel.setText("Sin Amp", juce::dontSendNotification);
    sineAmpLabel.setFont(10);
    sineFreqLabel.setText("Sin Freq", juce::dontSendNotification);
    sineFreqLabel.setFont(10);

    waveshaperDriveSlider.labels.add({ 0.f, "0dB" });
    waveshaperDriveSlider.labels.add({ 1.f, "40dB" });
    waveshaperMixSlider.labels.add({ 0.f, "0%" });
    waveshaperMixSlider.labels.add({ 1.f, "100%" });
    tanhAmpSlider.labels.add({ 0.f, "0" });
    tanhAmpSlider.labels.add({ 1.f, "100" });
    tanhSlopeSlider.labels.add({ 0.f, "1" });
    tanhSlopeSlider.labels.add({ 1.f, "15" });
    sineAmpSlider.labels.add({ 0.f, "0" });
    sineAmpSlider.labels.add({ 1.f, "100" });
    sineFreqSlider.labels.add({ 0.f, "0" });
    sineFreqSlider.labels.add({ 1.f, "100" });

    for (auto* comp : getComps())
    {
        addAndMakeVisible(comp);
    }
}

void WaveshaperModuleGUI::paint(juce::Graphics& g)
{
    drawContainer(g);
}

void WaveshaperModuleGUI::resized()
{
    auto waveshaperArea = getContentRenderArea();

    auto waveshaperGraphArea = waveshaperArea.removeFromLeft(waveshaperArea.getWidth() * (1.f / 2.f));
    waveshaperGraphArea.reduce(10, 10);

    auto waveshaperBasicControlsArea = waveshaperArea.removeFromTop(waveshaperArea.getHeight() * (1.f / 3.f));
    auto basicControlsLabelsArea = waveshaperBasicControlsArea.removeFromTop(12);
    auto driveLabelArea = basicControlsLabelsArea.removeFromLeft(basicControlsLabelsArea.getWidth() * (1.f / 2.f));
    auto mixLabelArea = basicControlsLabelsArea;

    auto waveshaperTanhControlsArea = waveshaperArea.removeFromTop(waveshaperArea.getHeight() * (1.f / 2.f));
    auto tanhControlsLabelsArea = waveshaperTanhControlsArea.removeFromTop(12);
    auto tanhAmpLabelArea = tanhControlsLabelsArea.removeFromLeft(tanhControlsLabelsArea.getWidth() * (1.f / 2.f));
    auto tanhSlopeLabelArea = tanhControlsLabelsArea;

    auto waveshaperSineControlsArea = waveshaperArea;
    auto sineControlsLabelsArea = waveshaperSineControlsArea.removeFromTop(12);
    auto sineAmpLabelArea = sineControlsLabelsArea.removeFromLeft(sineControlsLabelsArea.getWidth() * (1.f / 2.f));
    auto sineFreqLabelArea = sineControlsLabelsArea;

    transferFunctionGraph.setBounds(waveshaperGraphArea);

    waveshaperDriveSlider.setBounds(waveshaperBasicControlsArea.removeFromLeft(waveshaperBasicControlsArea.getWidth() * (1.f / 2.f)));
    waveshaperDriveLabel.setBounds(driveLabelArea);
    waveshaperDriveLabel.setJustificationType(juce::Justification::centred);

    waveshaperMixSlider.setBounds(waveshaperBasicControlsArea);
    waveshaperMixLabel.setBounds(mixLabelArea);
    waveshaperMixLabel.setJustificationType(juce::Justification::centred);

    tanhAmpSlider.setBounds(waveshaperTanhControlsArea.removeFromLeft(waveshaperTanhControlsArea.getWidth() * (1.f / 2.f)));
    tanhAmpLabel.setBounds(tanhAmpLabelArea);
    tanhAmpLabel.setJustificationType(juce::Justification::centred);

    tanhSlopeSlider.setBounds(waveshaperTanhControlsArea);
    tanhSlopeLabel.setBounds(tanhSlopeLabelArea);
    tanhSlopeLabel.setJustificationType(juce::Justification::centred);

    sineAmpSlider.setBounds(waveshaperSineControlsArea.removeFromLeft(waveshaperSineControlsArea.getWidth() * (1.f / 2.f)));
    sineAmpLabel.setBounds(sineAmpLabelArea);
    sineAmpLabel.setJustificationType(juce::Justification::centred);

    sineFreqSlider.setBounds(waveshaperSineControlsArea);
    sineFreqLabel.setBounds(sineFreqLabelArea);
    sineFreqLabel.setJustificationType(juce::Justification::centred);

    // labels
}

std::vector<juce::Component*> WaveshaperModuleGUI::getComps()
{
    return {
        &transferFunctionGraph,
        &waveshaperDriveSlider,
        &waveshaperMixSlider,
        &tanhAmpSlider,
        &tanhSlopeSlider,
        &sineAmpSlider,
        &sineFreqSlider,
        // labels
        &waveshaperDriveLabel,
        &waveshaperMixLabel,
        &tanhAmpLabel,
        &tanhSlopeLabel,
        &sineAmpLabel,
        &sineFreqLabel
    };
}
