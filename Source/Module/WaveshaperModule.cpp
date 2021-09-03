/*
  ==============================================================================

    WaveshaperModule.cpp
    Created: 26 Aug 2021 12:05:19pm
    Author:  gabri

  ==============================================================================
*/

#include "WaveshaperModule.h"

WaveshaperModule::WaveshaperModule(juce::AudioProcessorValueTreeState& apvts)
    : DSPModule(apvts)
{
}

void WaveshaperModule::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    wetBuffer.setSize(2, samplesPerBlock, false, true, true); // clears
    isActive = true;
    /*oversampler.initProcessing(samplesPerBlock);
    oversampler.reset();*/
}

void WaveshaperModule::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages, double sampleRate)
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

void WaveshaperModule::addParameters(juce::AudioProcessorValueTreeState::ParameterLayout& layout)
{
    using namespace juce;

    layout.add(std::make_unique<juce::AudioParameterFloat>("Waveshaper Drive", "Waveshaper Drive", NormalisableRange<float>(0.f, 40.f, 0.01f), 10.f, "Waveshaper"));
    layout.add(std::make_unique<AudioParameterFloat>("Waveshaper Mix", "Waveshaper Mix", NormalisableRange<float>(0.f, 100.f, 0.01f), 100.f, "Waveshaper"));
    layout.add(std::make_unique<AudioParameterFloat>("Waveshaper Tanh Amp", "Waveshaper Tanh Amp", NormalisableRange<float>(0.f, 100.f, 0.01f), 100.f, "Waveshaper"));
    layout.add(std::make_unique<AudioParameterFloat>("Waveshaper Tanh Slope", "Waveshaper Tanh Slope", NormalisableRange<float>(1.f, 15.f, 0.01f), 1.f, "Waveshaper"));
    layout.add(std::make_unique<AudioParameterFloat>("Waveshaper Sine Amp", "Waveshaper Sin Amp", NormalisableRange<float>(0.f, 100.f, 0.01f), 0.f, "Waveshaper"));
    layout.add(std::make_unique<AudioParameterFloat>("Waveshaper Sine Freq", "Waveshaper Sin Freq", NormalisableRange<float>(0.5f, 100.f, 0.01f), 1.f, "Waveshaper"));
}

WaveshaperSettings WaveshaperModule::getSettings(juce::AudioProcessorValueTreeState& apvts)
{
    WaveshaperSettings settings;

    settings.drive = apvts.getRawParameterValue("Waveshaper Drive")->load();
    settings.mix = apvts.getRawParameterValue("Waveshaper Mix")->load();
    settings.tanhAmp = apvts.getRawParameterValue("Waveshaper Tanh Amp")->load();
    settings.tanhSlope = apvts.getRawParameterValue("Waveshaper Tanh Slope")->load();
    settings.sinAmp = apvts.getRawParameterValue("Waveshaper Sine Amp")->load();
    settings.sinFreq = apvts.getRawParameterValue("Waveshaper Sine Freq")->load();

    return settings;
}

void WaveshaperModule::updateDSPState(double)
{
    auto settings = getSettings(apvts);
    driveGain.setTargetValue(juce::Decibels::decibelsToGain(settings.drive));

    tanhAmp.setTargetValue(settings.tanhAmp * 0.01f);
    tanhSlope.setTargetValue(settings.tanhSlope);
    sineAmp.setTargetValue(settings.sinAmp * 0.01f);
    sineFreq.setTargetValue(settings.sinFreq);

    auto mix = settings.mix * 0.01f;
    dryGain.setTargetValue(1.f - mix);
    wetGain.setTargetValue(mix);
}
