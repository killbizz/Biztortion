/*
  ==============================================================================

    WaveshaperModule.cpp
    Created: 26 Aug 2021 12:05:19pm
    Author:  gabri

  ==============================================================================
*/

/*
  ==============================================================================

    CREDITS for the original Waveshaper Algorithm
    Author: Daniele Filaretti
    Source: https://github.com/dfilaretti/waveshaper-demo

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
        layout.add(std::make_unique<AudioParameterFloat>("Waveshaper Sine Freq " + std::to_string(i), "Waveshaper Sin Freq " + std::to_string(i), NormalisableRange<float>(0.5f, 100.f, 0.01f), 1.f, "Waveshaper " + std::to_string(i)));
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
    title.setFont(24);

    // labels
    driveLabel.setText("Drive", juce::dontSendNotification);
    driveLabel.setFont(10);
    mixLabel.setText("Mix", juce::dontSendNotification);
    mixLabel.setFont(10);
    symmetryLabel.setText("Symmetry", juce::dontSendNotification);
    symmetryLabel.setFont(10);
    biasLabel.setText("Bias", juce::dontSendNotification);
    biasLabel.setFont(10);
    tanhAmpLabel.setText("Tanh Amp", juce::dontSendNotification);
    tanhAmpLabel.setFont(10);
    tanhSlopeLabel.setText("Tanh Slope", juce::dontSendNotification);
    tanhSlopeLabel.setFont(10);
    sineAmpLabel.setText("Sin Amp", juce::dontSendNotification);
    sineAmpLabel.setFont(10);
    sineFreqLabel.setText("Sin Freq", juce::dontSendNotification);
    sineFreqLabel.setFont(10);

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
    sineFreqSlider.labels.add({ 0.f, "0" });
    sineFreqSlider.labels.add({ 1.f, "100" });

    bypassButton.setLookAndFeel(&lnf);

    auto safePtr = juce::Component::SafePointer<WaveshaperModuleGUI>(this);
    bypassButton.onClick = [safePtr]()
    {
        if (auto* comp = safePtr.getComponent())
        {
            auto bypassed = comp->bypassButton.getToggleState();

            comp->driveSlider.setEnabled(!bypassed);
            comp->mixSlider.setEnabled(!bypassed);
            comp->symmetrySlider.setEnabled(!bypassed);
            comp->biasSlider.setEnabled(!bypassed);
            comp->tanhAmpSlider.setEnabled(!bypassed);
            comp->tanhSlopeSlider.setEnabled(!bypassed);
            comp->sineAmpSlider.setEnabled(!bypassed);
            comp->sineFreqSlider.setEnabled(!bypassed);
        }
    };

    for (auto* comp : getComps())
    {
        addAndMakeVisible(comp);
    }
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

    auto waveshaperGraphArea = waveshaperArea.removeFromLeft(waveshaperArea.getWidth() * (1.f / 3.f));
    waveshaperGraphArea.reduce(10, 10);

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
    auto mixArea = temp;
    auto symmetryArea = topArea.removeFromLeft(topArea.getWidth() * (1.f / 2.f));
    auto biasArea = topArea;
    temp = bottomArea.removeFromLeft(bottomArea.getWidth() * (1.f / 2.f));
    auto tanhAmpArea = temp.removeFromLeft(temp.getWidth() * (1.f / 2.f));
    auto tanhSlopeArea = temp;
    auto sineAmpArea = bottomArea.removeFromLeft(bottomArea.getWidth() * (1.f / 2.f));
    auto sineFreqArea = bottomArea;

    
    title.setBounds(titleAndBypassArea);
    title.setJustificationType(juce::Justification::centred);

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

std::vector<juce::Component*> WaveshaperModuleGUI::getComps()
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
