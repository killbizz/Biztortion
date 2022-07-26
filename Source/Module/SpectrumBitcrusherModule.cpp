/*
  ==============================================================================

    SpectrumBitcrusherModule.cpp

    Copyright (c) 2022 KillBizz - Gabriel Bizzo

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

#include "SpectrumBitcrusherModule.h"

//==============================================================================

/* SpectrumBitcrusherModule DSP */

//==============================================================================

SpectrumBitcrusherModuleDSP::SpectrumBitcrusherModuleDSP(juce::AudioProcessorValueTreeState& _apvts)
    : DSPModule(_apvts)
{
}

void SpectrumBitcrusherModuleDSP::updateDSPState(double sampleRate)
{
    auto settings = getSettings(apvts, parameterNumber);

    bypassed = settings.bypassed;

    auto mix = settings.mix * 0.01f;
    dryGain.setTargetValue(1.f - mix);
    wetGain.setTargetValue(mix);
    driveGain.setTargetValue(juce::Decibels::decibelsToGain(settings.drive));
    symmetry.setTargetValue(settings.symmetry * 0.01f);
    bias.setTargetValue(settings.bias);

    rateRedux.setTargetValue(settings.rateRedux);
    bitRedux.setTargetValue(settings.bitRedux);
}

void SpectrumBitcrusherModuleDSP::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    wetBuffer.setSize(2, samplesPerBlock, false, true, true); // clears
    tempBuffer.setSize(2, samplesPerBlock, false, true, true); // clears

    leftChannelFFTDataGenerator.prepare(FFTOrder::order4096);
    rightChannelFFTDataGenerator.prepare(FFTOrder::order4096);
    
    leftChannelAudioDataGenerator.prepare(samplesPerBlock, FFTOrder::order4096);
    rightChannelAudioDataGenerator.prepare(samplesPerBlock, FFTOrder::order4096);

    leftChannelSampleFifo.prepare(leftChannelFFTDataGenerator.getFFTSize());
    rightChannelSampleFifo.prepare(rightChannelFFTDataGenerator.getFFTSize());

    leftAudioBuffer.setSize(1, leftChannelFFTDataGenerator.getFFTSize());
    rightAudioBuffer.setSize(1, rightChannelFFTDataGenerator.getFFTSize());
    leftFFTBuffer.clear();
    leftFFTBuffer.resize(leftChannelFFTDataGenerator.getFFTSize());
    rightFFTBuffer.clear();
    rightFFTBuffer.resize(rightChannelFFTDataGenerator.getFFTSize());

    updateDSPState(sampleRate);
}

void SpectrumBitcrusherModuleDSP::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages, double sampleRate)
{

    updateDSPState(sampleRate);

    if (!bypassed) {

        // SAFETY CHECK :::: since some hosts will change buffer sizes without calling prepToPlay (ex: Bitwig)
        int numSamples = buffer.getNumSamples();

        if (wetBuffer.getNumSamples() != numSamples)
            prepareToPlay(sampleRate, numSamples);

        // PROCESSING

        // TODO : frequency-domain bitcrushing (and a way to switch between time/frequency-domain)
        // see FFTAnalyzerComponent for FFT-related stuff 
        // https://forum.juce.com/t/issue-with-fft-plugin-inverse-transformation/16630/3
        // https://forum.juce.com/t/fft-amplitude/28574
        // tips:
        // - delay iniziale del segnale = aggiunta di 0.f in tutti i sample del buffer in uscita
        // - nel caso in cui il param mix > 0% devo ritardare la riproduzione anche del segnale dry !!
        // - minore FFT-order - bufferSize => minor delay del segnale
        // - outputBuffer[i+(2^fft-order / bufferSIze)] = modifiedBuffer[i] -> considerando outputBuffer e modifiedBuffer come array di buffer
        // - risorse utili: FFTAnalyzerComponent + https://docs.juce.com/master/tutorial_simple_fft.html

        // Wet Buffer feeding
        for (auto channel = 0; channel < 2; channel++)
            wetBuffer.copyFrom(channel, 0, buffer, channel, 0, numSamples);

        // Drive
        driveGain.applyGain(wetBuffer, numSamples);

        leftChannelSampleFifo.update(wetBuffer);
        rightChannelSampleFifo.update(wetBuffer);

        // Temp Buffer feeding for applying asymmetry
        /*for (auto channel = 0; channel < 2; channel++)
            tempBuffer.copyFrom(channel, 0, wetBuffer, channel, 0, numSamples);*/

        // produce fft data information
        if (leftChannelSampleFifo.getNumCompleteBuffersAvailable() > 0) {
            if (leftChannelSampleFifo.getAudioBuffer(leftAudioBuffer)) {
                // generate FFT data from audio buffer
                leftChannelFFTDataGenerator.produceFFTDataForSpectrumElaboration(leftAudioBuffer);
            }
        }
        if (rightChannelSampleFifo.getNumCompleteBuffersAvailable() > 0) {
            if (rightChannelSampleFifo.getAudioBuffer(rightAudioBuffer)) {
                // generate FFT data from audio buffer
                rightChannelFFTDataGenerator.produceFFTDataForSpectrumElaboration(rightAudioBuffer);
            }
        }

        // SPECTRUM DATA ELABORATION

        // produce audio data
        if (leftChannelFFTDataGenerator.getNumAvailableFFTDataBlocks() > 0) {
            if (leftChannelFFTDataGenerator.getFFTData(leftFFTBuffer)) {
                // generate audio data from fft buffer
                leftChannelAudioDataGenerator.produceAudioDataFromFFTData(leftFFTBuffer);
            }
        }
        if (rightChannelFFTDataGenerator.getNumAvailableFFTDataBlocks() > 0) {
            if (rightChannelFFTDataGenerator.getFFTData(rightFFTBuffer)) {
                // generate audio data from fft buffer
                rightChannelAudioDataGenerator.produceAudioDataFromFFTData(rightFFTBuffer);
            }
        }

        // fill wetBuffer with a freshly built audio buffer
        for (int chan = 0; chan < wetBuffer.getNumChannels(); chan++)
        {
            float* data = wetBuffer.getWritePointer(chan);
            juce::AudioBuffer<float> tempIncomingBuffer;

            AudioDataGenerator<std::vector<float>>& generatorRef = static_cast<Channel>(chan) == Channel::Left ?
                leftChannelAudioDataGenerator : rightChannelAudioDataGenerator;

            if (leftChannelAudioDataGenerator.getNumAvailablAudioDataBlocks() > 0) {
                bool bufferCorrectlyRetrieved = generatorRef.getAudioData(tempIncomingBuffer);
                jassert(bufferCorrectlyRetrieved);
                auto* readIndex = tempIncomingBuffer.getReadPointer(0);

                for (int i = 0; i < numSamples; i++)
                {
                    data[i] = readIndex[i];
                }
            }
            else {
                for (int i = 0; i < numSamples; i++)
                {
                    data[i] = 0.f;
                }
            }
        }

        // applyAsymmetry(tempBuffer, wetBuffer, symmetry.getNextValue(), bias.getNextValue(), numSamples);

        // Mixing buffers
        dryGain.applyGain(buffer, numSamples);
        // wetGain.applyGain(tempBuffer, numSamples);
        wetGain.applyGain(wetBuffer, numSamples);

        /*for (auto channel = 0; channel < 2; channel++)
            buffer.addFrom(channel, 0, tempBuffer, channel, 0, numSamples);*/
        for (auto channel = 0; channel < 2; channel++)
            buffer.addFrom(channel, 0, wetBuffer, channel, 0, numSamples);
    }
}

void SpectrumBitcrusherModuleDSP::addParameters(juce::AudioProcessorValueTreeState::ParameterLayout& layout)
{
    using namespace juce;

    for (int i = 1; i < 9; ++i) {
        layout.add(std::make_unique<AudioParameterFloat>(SPECTRUM_BITCRUSHER_ID + "Drive " + std::to_string(i), "Spectrum Bitcrusher Drive " + std::to_string(i), NormalisableRange<float>(0.f, 40.f, 0.01f), 0.f, "Spectrum Bitcrusher " + std::to_string(i)));
        layout.add(std::make_unique<AudioParameterFloat>(SPECTRUM_BITCRUSHER_ID + "Mix " + std::to_string(i), "Spectrum Bitcrusher Mix " + std::to_string(i), NormalisableRange<float>(0.f, 100.f, 0.01f), 100.f, "Spectrum Bitcrusher " + std::to_string(i)));
        layout.add(std::make_unique<AudioParameterFloat>(SPECTRUM_BITCRUSHER_ID + "Symmetry " + std::to_string(i), "Spectrum Bitcrusher Symmetry " + std::to_string(i), NormalisableRange<float>(-100.f, 100.f, 1.f), 0.f, "Spectrum Bitcrusher " + std::to_string(i)));
        layout.add(std::make_unique<AudioParameterFloat>(SPECTRUM_BITCRUSHER_ID + "Bias " + std::to_string(i), "Spectrum Bitcrusher Bias " + std::to_string(i), NormalisableRange<float>(-0.9f, 0.9f, 0.01f), 0.f, "Spectrum Bitcrusher " + std::to_string(i)));
        layout.add(std::make_unique<AudioParameterFloat>(SPECTRUM_BITCRUSHER_ID + "Bins Redux " + std::to_string(i), "Spectrum Bitcrusher Bins Redux " + std::to_string(i), NormalisableRange<float>(64.f, 2048.f, 2.f, 0.25f), 2048.f, "Spectrum Bitcrusher " + std::to_string(i)));
        layout.add(std::make_unique<AudioParameterFloat>(SPECTRUM_BITCRUSHER_ID + "Bit Redux " + std::to_string(i), "Spectrum Bitcrusher Bit Redux " + std::to_string(i), NormalisableRange<float>(1.f, 16.f, 0.01f, 0.25f), 16.f, "Spectrum Bitcrusher " + std::to_string(i)));
        layout.add(std::make_unique<AudioParameterBool>(SPECTRUM_BITCRUSHER_ID + "Bypassed " + std::to_string(i), "Spectrum Bitcrusher Bypassed " + std::to_string(i), false, "Spectrum Bitcrusher " + std::to_string(i)));
    }
}

SpectrumBitcrusherSettings SpectrumBitcrusherModuleDSP::getSettings(juce::AudioProcessorValueTreeState& apvts, unsigned int parameterNumber)
{
    SpectrumBitcrusherSettings settings;

    settings.drive = apvts.getRawParameterValue(SPECTRUM_BITCRUSHER_ID + "Drive " + std::to_string(parameterNumber))->load();
    settings.mix = apvts.getRawParameterValue(SPECTRUM_BITCRUSHER_ID + "Mix " + std::to_string(parameterNumber))->load();
    settings.symmetry = apvts.getRawParameterValue(SPECTRUM_BITCRUSHER_ID + "Symmetry " + std::to_string(parameterNumber))->load();
    settings.bias = apvts.getRawParameterValue(SPECTRUM_BITCRUSHER_ID + "Bias " + std::to_string(parameterNumber))->load();
    settings.rateRedux = apvts.getRawParameterValue(SPECTRUM_BITCRUSHER_ID + "Bins Redux " + std::to_string(parameterNumber))->load();
    settings.bitRedux = apvts.getRawParameterValue(SPECTRUM_BITCRUSHER_ID + "Bit Redux " + std::to_string(parameterNumber))->load();
    settings.bypassed = apvts.getRawParameterValue(SPECTRUM_BITCRUSHER_ID + "Bypassed " + std::to_string(parameterNumber))->load() > 0.5f;

    return settings;
}

//==============================================================================

/* SpectrumBitcrusherModule GUI */

//==============================================================================

SpectrumBitcrusherModuleGUI::SpectrumBitcrusherModuleGUI(PluginState& p, unsigned int parameterNumber)
    : GUIModule(), pluginState(p),
    driveSlider(*pluginState.apvts.getParameter(SPECTRUM_BITCRUSHER_ID + "Drive " + std::to_string(parameterNumber)), "dB"),
    mixSlider(*pluginState.apvts.getParameter(SPECTRUM_BITCRUSHER_ID + "Mix " + std::to_string(parameterNumber)), "%"),
    symmetrySlider(*pluginState.apvts.getParameter(SPECTRUM_BITCRUSHER_ID + "Symmetry " + std::to_string(parameterNumber)), "%"),
    biasSlider(*pluginState.apvts.getParameter(SPECTRUM_BITCRUSHER_ID + "Bias " + std::to_string(parameterNumber)), ""),
    bitcrusherRateReduxSlider(*pluginState.apvts.getParameter(SPECTRUM_BITCRUSHER_ID + "Bins Redux " + std::to_string(parameterNumber)), "Hz"),
    bitcrusherBitReduxSlider(*pluginState.apvts.getParameter(SPECTRUM_BITCRUSHER_ID + "Bit Redux " + std::to_string(parameterNumber)), ""),
    driveSliderAttachment(pluginState.apvts, SPECTRUM_BITCRUSHER_ID + "Drive " + std::to_string(parameterNumber), driveSlider),
    mixSliderAttachment(pluginState.apvts, SPECTRUM_BITCRUSHER_ID + "Mix " + std::to_string(parameterNumber), mixSlider),
    symmetrySliderAttachment(pluginState.apvts, SPECTRUM_BITCRUSHER_ID + "Symmetry " + std::to_string(parameterNumber), symmetrySlider),
    biasSliderAttachment(pluginState.apvts, SPECTRUM_BITCRUSHER_ID + "Bias " + std::to_string(parameterNumber), biasSlider),
    bitcrusherRateReduxSliderAttachment(pluginState.apvts, SPECTRUM_BITCRUSHER_ID + "Bins Redux " + std::to_string(parameterNumber), bitcrusherRateReduxSlider),
    bitcrusherBitReduxSliderAttachment(pluginState.apvts, SPECTRUM_BITCRUSHER_ID + "Bit Redux " + std::to_string(parameterNumber), bitcrusherBitReduxSlider),
    bypassButtonAttachment(pluginState.apvts, SPECTRUM_BITCRUSHER_ID + "Bypassed " + std::to_string(parameterNumber), bypassButton)
{
    // title setup
    title.setText("Spectrum Bitcrusher", juce::dontSendNotification);
    title.setFont(ModuleLookAndFeel::getTitlesFont());

    // labels
    driveLabel.setText("Drive", juce::dontSendNotification);
    driveLabel.setFont(ModuleLookAndFeel::getLabelsFont());
    mixLabel.setText("Mix", juce::dontSendNotification);
    mixLabel.setFont(ModuleLookAndFeel::getLabelsFont());
    symmetryLabel.setText("Symmetry", juce::dontSendNotification);
    symmetryLabel.setFont(ModuleLookAndFeel::getLabelsFont());
    biasLabel.setText("Bias", juce::dontSendNotification);
    biasLabel.setFont(ModuleLookAndFeel::getLabelsFont());
    bitcrusherRateReduxLabel.setText("Bins Rate Redux", juce::dontSendNotification);
    bitcrusherRateReduxLabel.setFont(ModuleLookAndFeel::getLabelsFont());
    bitcrusherBitReduxLabel.setText("Bit Depht Redux", juce::dontSendNotification);
    bitcrusherBitReduxLabel.setFont(ModuleLookAndFeel::getLabelsFont());

    driveSlider.labels.add({ 0.f, "0dB" });
    driveSlider.labels.add({ 1.f, "40dB" });
    mixSlider.labels.add({ 0.f, "0%" });
    mixSlider.labels.add({ 1.f, "100%" });
    symmetrySlider.labels.add({ 0.f, "-100%" });
    symmetrySlider.labels.add({ 1.f, "+100%" });
    biasSlider.labels.add({ 0.f, "-0.9" });
    biasSlider.labels.add({ 1.f, "+0.9" });
    bitcrusherRateReduxSlider.labels.add({ 0.f, "64" });
    bitcrusherRateReduxSlider.labels.add({ 1.f, "2048" });
    bitcrusherBitReduxSlider.labels.add({ 0.f, "1" });
    bitcrusherBitReduxSlider.labels.add({ 1.f, "16" });

    bypassButton.setLookAndFeel(&lnf);

    auto safePtr = juce::Component::SafePointer<SpectrumBitcrusherModuleGUI>(this);
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
    bitcrusherRateReduxSlider.setTooltip("Set the bins rate for the reduction of the spectrum resolution");
    bitcrusherBitReduxSlider.setTooltip("Set the bit depth for the reduction of the spectrum resolution");

    for (auto* comp : getAllComps())
    {
        addAndMakeVisible(comp);
    }

    handleParamCompsEnablement(bypassButton.getToggleState());
}

SpectrumBitcrusherModuleGUI::~SpectrumBitcrusherModuleGUI()
{
    bypassButton.setLookAndFeel(nullptr);
}

std::vector<juce::Component*> SpectrumBitcrusherModuleGUI::getAllComps()
{
    return {
        &title,
        &driveSlider,
        &mixSlider,
        &symmetrySlider,
        &biasSlider,
        &bitcrusherRateReduxSlider,
        &bitcrusherBitReduxSlider,
        // labels
        &driveLabel,
        &mixLabel,
        &symmetryLabel,
        &biasLabel,
        &bitcrusherRateReduxLabel,
        &bitcrusherBitReduxLabel,
        // bypass
        &bypassButton
    };
}

std::vector<juce::Component*> SpectrumBitcrusherModuleGUI::getParamComps()
{
    return {
        &driveSlider,
        &mixSlider,
        &symmetrySlider,
        &biasSlider,
        &bitcrusherRateReduxSlider,
        &bitcrusherBitReduxSlider
    };
}

void SpectrumBitcrusherModuleGUI::updateParameters(const juce::Array<juce::var>& values)
{
    auto value = values.begin();

    bypassButton.setToggleState(*(value++), juce::NotificationType::sendNotificationSync);
    driveSlider.setValue(*(value++), juce::NotificationType::sendNotificationSync);
    mixSlider.setValue(*(value++), juce::NotificationType::sendNotificationSync);
    symmetrySlider.setValue(*(value++), juce::NotificationType::sendNotificationSync);
    biasSlider.setValue(*(value++), juce::NotificationType::sendNotificationSync);
    bitcrusherRateReduxSlider.setValue(*(value++), juce::NotificationType::sendNotificationSync);
    bitcrusherBitReduxSlider.setValue(*(value++), juce::NotificationType::sendNotificationSync);
}

void SpectrumBitcrusherModuleGUI::resetParameters(unsigned int parameterNumber)
{
    auto drive = pluginState.apvts.getParameter(SPECTRUM_BITCRUSHER_ID + "Drive " + std::to_string(parameterNumber));
    auto mix = pluginState.apvts.getParameter(SPECTRUM_BITCRUSHER_ID + "Mix " + std::to_string(parameterNumber));
    auto symmetry = pluginState.apvts.getParameter(SPECTRUM_BITCRUSHER_ID + "Symmetry " + std::to_string(parameterNumber));
    auto bias = pluginState.apvts.getParameter(SPECTRUM_BITCRUSHER_ID + "Bias " + std::to_string(parameterNumber));
    auto rateRedux = pluginState.apvts.getParameter(SPECTRUM_BITCRUSHER_ID + "Bins Redux " + std::to_string(parameterNumber));
    auto bitRedux = pluginState.apvts.getParameter(SPECTRUM_BITCRUSHER_ID + "Bit Redux " + std::to_string(parameterNumber));
    auto bypassed = pluginState.apvts.getParameter(SPECTRUM_BITCRUSHER_ID + "Bypassed " + std::to_string(parameterNumber));

    drive->setValueNotifyingHost(drive->getDefaultValue());
    mix->setValueNotifyingHost(mix->getDefaultValue());
    symmetry->setValueNotifyingHost(symmetry->getDefaultValue());
    bias->setValueNotifyingHost(bias->getDefaultValue());
    rateRedux->setValueNotifyingHost(rateRedux->getDefaultValue());
    bitRedux->setValueNotifyingHost(bitRedux->getDefaultValue());
    bypassed->setValueNotifyingHost(bypassed->getDefaultValue());
}

juce::Array<juce::var> SpectrumBitcrusherModuleGUI::getParamValues()
{
    juce::Array<juce::var> values;

    values.add(juce::var(bypassButton.getToggleState()));
    values.add(juce::var(driveSlider.getValue()));
    values.add(juce::var(mixSlider.getValue()));
    values.add(juce::var(symmetrySlider.getValue()));
    values.add(juce::var(biasSlider.getValue()));
    values.add(juce::var(bitcrusherRateReduxSlider.getValue()));
    values.add(juce::var(bitcrusherBitReduxSlider.getValue()));

    return values;
}

void SpectrumBitcrusherModuleGUI::resized()
{
    auto bitcrusherArea = getContentRenderArea();

    // bypass
    auto temp = bitcrusherArea;
    auto bypassButtonArea = temp.removeFromTop(25);

    bypassButtonArea.setWidth(35.f);
    bypassButtonArea.setX(145.f);
    bypassButtonArea.setY(20.f);

    bypassButton.setBounds(bypassButtonArea);

    auto titleAndBypassArea = bitcrusherArea.removeFromTop(30);
    titleAndBypassArea.translate(0, 4);

    bitcrusherArea.translate(0, 8);

    auto topArea = bitcrusherArea.removeFromTop(bitcrusherArea.getHeight() * (1.f / 2.f));
    auto bottomArea = bitcrusherArea;

    auto topLabelsArea = topArea.removeFromTop(14);
    auto bottomLabelsArea = bottomArea.removeFromTop(14);

    // label areas
    temp = topLabelsArea.removeFromLeft(topLabelsArea.getWidth() * (1.f / 2.f));
    auto driveLabelArea = temp.removeFromLeft(temp.getWidth() * (1.f / 2.f));
    auto mixLabelArea = temp;
    auto symmetryLabelArea = topLabelsArea.removeFromLeft(topLabelsArea.getWidth() * (1.f / 2.f));
    auto biasLabelArea = topLabelsArea;
    auto rateLabelArea = bottomLabelsArea.removeFromLeft(bottomLabelsArea.getWidth() * (1.f / 2.f));
    auto bitLabelArea = bottomLabelsArea;

    // slider areas
    temp = topArea.removeFromLeft(topArea.getWidth() * (1.f / 2.f));
    auto driveArea = temp.removeFromLeft(temp.getWidth() * (1.f / 2.f));
    auto mixArea = temp;
    auto symmetryArea = topArea.removeFromLeft(topArea.getWidth() * (1.f / 2.f));
    auto biasArea = topArea;
    auto rateArea = bottomArea.removeFromLeft(bottomArea.getWidth() * (1.f / 2.f));
    auto bitArea = bottomArea;

    juce::Rectangle<int> renderArea;
    renderArea.setSize(driveArea.getWidth(), driveArea.getWidth());

    title.setBounds(titleAndBypassArea);
    title.setJustificationType(juce::Justification::centredBottom);

    renderArea.setCentre(driveArea.getCentre());
    renderArea.setY(driveArea.getTopLeft().getY());
    driveSlider.setBounds(renderArea);
    driveLabel.setBounds(driveLabelArea);
    driveLabel.setJustificationType(juce::Justification::centred);

    renderArea.setCentre(mixArea.getCentre());
    renderArea.setY(mixArea.getTopLeft().getY());
    mixSlider.setBounds(renderArea);
    mixLabel.setBounds(mixLabelArea);
    mixLabel.setJustificationType(juce::Justification::centred);

    renderArea.setCentre(symmetryArea.getCentre());
    renderArea.setY(symmetryArea.getTopLeft().getY());
    symmetrySlider.setBounds(renderArea);
    symmetryLabel.setBounds(symmetryLabelArea);
    symmetryLabel.setJustificationType(juce::Justification::centred);

    renderArea.setCentre(biasArea.getCentre());
    renderArea.setY(biasArea.getTopLeft().getY());
    biasSlider.setBounds(renderArea);
    biasLabel.setBounds(biasLabelArea);
    biasLabel.setJustificationType(juce::Justification::centred);

    renderArea.setSize(rateArea.getHeight(), rateArea.getHeight());

    renderArea.setCentre(rateArea.getCentre());
    renderArea.setY(rateArea.getTopLeft().getY());
    bitcrusherRateReduxSlider.setBounds(renderArea);
    bitcrusherRateReduxLabel.setBounds(rateLabelArea);
    bitcrusherRateReduxLabel.setJustificationType(juce::Justification::centred);

    renderArea.setCentre(bitArea.getCentre());
    renderArea.setY(bitArea.getTopLeft().getY());
    bitcrusherBitReduxSlider.setBounds(renderArea);
    bitcrusherBitReduxLabel.setBounds(bitLabelArea);
    bitcrusherBitReduxLabel.setJustificationType(juce::Justification::centred);

}