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

#include <complex>
#include <cmath>
#include "../EqualizerModule/EqualizerModule.h"

using namespace std::complex_literals;

//==============================================================================

/* SpectrumBitcrusherModule DSP */

//==============================================================================

SpectrumBitcrusherModuleDSP::SpectrumBitcrusherModuleDSP(juce::AudioProcessorValueTreeState& _apvts)
    : DSPModule(_apvts)
{
    spectrumProcessingLambda = [this](juce::AudioSampleBuffer& fftBuffer) {

        auto numSamples = fftBuffer.getNumSamples();
        auto numBins = numSamples / 4;
        int binsReductionRatio = numBins / rateRedux.getNextValue();

        for (int chan = 0; chan < fftBuffer.getNumChannels(); chan++) {

            float* floatData = fftBuffer.getWritePointer(chan);
            auto* data = reinterpret_cast<std::complex<float>*>(floatData);

            for (int i = 0; i < numBins; i++) {

                // Spectrum Resolution Reduction
                if ((binsReductionRatio > 1) && (i % binsReductionRatio != 0)) {
                    double previousBinMagnitude = std::abs(data[i - i % binsReductionRatio]);
                    double currentBinPhase = std::arg(data[i]);

                    data[i] = previousBinMagnitude * (std::cos(currentBinPhase) + (1i * std::sin(currentBinPhase)));
                }

                // Robotisation
                double robotisationMix = robotisation.getNextValue();
                double currentBinMagnitude = std::abs(data[i]);
                // phase = 0 => constant pitch determined by the hopSize
                double currentBinPhase = std::arg(data[i]);
                double newPhase = currentBinPhase - ( currentBinPhase * robotisationMix );

                data[i] = currentBinMagnitude * (std::cos(newPhase) + (1i * std::sin(newPhase)));

            }
        }
    };
}

void SpectrumBitcrusherModuleDSP::updateDSPState(double sampleRate)
{
    auto settings = getSettings(apvts, parameterNumber);

    bypassed = settings.bypassed;
    
    auto symmetryValue = juce::jmap(settings.symmetry, -100.f, 100.f, 0.f, 2.f);

    auto mix = settings.mix * 0.01f;
    dryGain.setTargetValue(1.f - mix);
    wetGain.setTargetValue(mix);
    driveGain.setTargetValue(juce::Decibels::decibelsToGain(settings.drive));
    fxDistribution.setTargetValue(settings.fxDistribution * 0.01f);
    bias.setTargetValue(settings.bias);
    symmetry.setTargetValue(symmetryValue);

    rateRedux.setTargetValue(settings.rateRedux);
    robotisation.setTargetValue(settings.robotisation * 0.01f);

    DCoffsetRemoveEnabled = settings.DCoffsetRemove;
}

void SpectrumBitcrusherModuleDSP::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    juce::dsp::ProcessSpec spec;

    spec.maximumBlockSize = samplesPerBlock;
    spec.numChannels = 1;
    spec.sampleRate = sampleRate;

    leftDCoffsetRemoveHPF.prepare(spec);
    rightDCoffsetRemoveHPF.prepare(spec);

    // create a 1pole HPF at 5Hz in order to remove DC offset
    auto filterCoefficients = juce::dsp::FilterDesign<float>::designIIRHighpassHighOrderButterworthMethod(5, sampleRate, 1);
    updateCoefficients(leftDCoffsetRemoveHPF.coefficients, filterCoefficients[0]);
    updateCoefficients(rightDCoffsetRemoveHPF.coefficients, filterCoefficients[0]);

    wetBuffer.setSize(2, samplesPerBlock, false, true, true); // clears
    tempBuffer.setSize(2, samplesPerBlock, false, true, true); // clears

    spectrumProcessor.prepare(sampleRate, samplesPerBlock, 2, 2);
    spectrumProcessor.setSpectrumProcessingCallback(spectrumProcessingLambda);

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

        // Wet Buffer feeding
        for (auto channel = 0; channel < 2; channel++)
            wetBuffer.copyFrom(channel, 0, buffer, channel, 0, numSamples);

        // Drive
        driveGain.applyGain(wetBuffer, numSamples);

        // Temp Buffer feeding for applying afxDistribution
        for (auto channel = 0; channel < 2; channel++)
            tempBuffer.copyFrom(channel, 0, wetBuffer, channel, 0, numSamples);

        dsp::AudioBlock<float> ab(wetBuffer);
        dsp::ProcessContextReplacing<float> context(ab);
        spectrumProcessor.process(context);

        effectDistribution(tempBuffer, wetBuffer, fxDistribution.getNextValue(), bias.getNextValue(), numSamples);

        applySymmetry(tempBuffer, symmetry.getNextValue(), numSamples);

        if (DCoffsetRemoveEnabled) {
            juce::dsp::AudioBlock<float> block(tempBuffer);
            auto leftBlock = block.getSingleChannelBlock(0);
            auto rightBlock = block.getSingleChannelBlock(1);
            juce::dsp::ProcessContextReplacing<float> leftContext(leftBlock);
            juce::dsp::ProcessContextReplacing<float> rightContext(rightBlock);
            leftDCoffsetRemoveHPF.process(leftContext);
            rightDCoffsetRemoveHPF.process(rightContext);
        }

        // Mixing buffers
        dryGain.applyGain(buffer, numSamples);
        wetGain.applyGain(tempBuffer, numSamples);

        for (auto channel = 0; channel < 2; channel++)
            buffer.addFrom(channel, 0, tempBuffer, channel, 0, numSamples);
    }
}

void SpectrumBitcrusherModuleDSP::addParameters(juce::AudioProcessorValueTreeState::ParameterLayout& layout)
{
    using namespace juce;

    for (int i = 1; i < 9; ++i) {
        layout.add(std::make_unique<AudioParameterFloat>(SPECTRUM_BITCRUSHER_ID + "Drive " + std::to_string(i), "Spectrum Bitcrusher Drive " + std::to_string(i), NormalisableRange<float>(0.f, 40.f, 0.01f), 0.f, "Spectrum Bitcrusher " + std::to_string(i)));
        layout.add(std::make_unique<AudioParameterFloat>(SPECTRUM_BITCRUSHER_ID + "Mix " + std::to_string(i), "Spectrum Bitcrusher Mix " + std::to_string(i), NormalisableRange<float>(0.f, 100.f, 0.01f), 100.f, "Spectrum Bitcrusher " + std::to_string(i)));
        layout.add(std::make_unique<AudioParameterFloat>(SPECTRUM_BITCRUSHER_ID + "Fx Distribution " + std::to_string(i), "Spectrum Bitcrusher Fx Distribution " + std::to_string(i), NormalisableRange<float>(-100.f, 100.f, 1.f), 0.f, "Spectrum Bitcrusher " + std::to_string(i)));
        layout.add(std::make_unique<AudioParameterFloat>(SPECTRUM_BITCRUSHER_ID + "Bias " + std::to_string(i), "Spectrum Bitcrusher Bias " + std::to_string(i), NormalisableRange<float>(-0.9f, 0.9f, 0.01f), 0.f, "Spectrum Bitcrusher " + std::to_string(i)));
        layout.add(std::make_unique<AudioParameterFloat>(SPECTRUM_BITCRUSHER_ID + "Symmetry " + std::to_string(i), "Spectrum Bitcrusher Symmetry " + std::to_string(i), NormalisableRange<float>(-100.f, 100.f, 1.f), 0.f, "Spectrum Bitcrusher " + std::to_string(i)));
        layout.add(std::make_unique<AudioParameterFloat>(SPECTRUM_BITCRUSHER_ID + "Bins Redux " + std::to_string(i), "Spectrum Bitcrusher Bins Redux " + std::to_string(i), NormalisableRange<float>(16.f, 1024.f, 1.f, 0.25f), 1024.f, "Spectrum Bitcrusher " + std::to_string(i)));
        layout.add(std::make_unique<AudioParameterFloat>(SPECTRUM_BITCRUSHER_ID + "Robotisation " + std::to_string(i), "Spectrum Bitcrusher Robotisation " + std::to_string(i), NormalisableRange<float>(0.f, 100.f, 1.f), 0.f, "Spectrum Bitcrusher " + std::to_string(i)));
        layout.add(std::make_unique<AudioParameterBool>(SPECTRUM_BITCRUSHER_ID + "DCoffset Enabled " + std::to_string(i), "Spectrum Bitcrusher DCoffset Enabled " + std::to_string(i), true, "Spectrum Bitcrusher " + std::to_string(i)));
        layout.add(std::make_unique<AudioParameterBool>(SPECTRUM_BITCRUSHER_ID + "Bypassed " + std::to_string(i), "Spectrum Bitcrusher Bypassed " + std::to_string(i), false, "Spectrum Bitcrusher " + std::to_string(i)));
    }
}

SpectrumBitcrusherSettings SpectrumBitcrusherModuleDSP::getSettings(juce::AudioProcessorValueTreeState& apvts, unsigned int parameterNumber)
{
    SpectrumBitcrusherSettings settings;

    settings.drive = apvts.getRawParameterValue(SPECTRUM_BITCRUSHER_ID + "Drive " + std::to_string(parameterNumber))->load();
    settings.mix = apvts.getRawParameterValue(SPECTRUM_BITCRUSHER_ID + "Mix " + std::to_string(parameterNumber))->load();
    settings.fxDistribution = apvts.getRawParameterValue(SPECTRUM_BITCRUSHER_ID + "Fx Distribution " + std::to_string(parameterNumber))->load();
    settings.symmetry = apvts.getRawParameterValue(SPECTRUM_BITCRUSHER_ID + "Symmetry " + std::to_string(parameterNumber))->load();
    settings.bias = apvts.getRawParameterValue(SPECTRUM_BITCRUSHER_ID + "Bias " + std::to_string(parameterNumber))->load();
    settings.rateRedux = apvts.getRawParameterValue(SPECTRUM_BITCRUSHER_ID + "Bins Redux " + std::to_string(parameterNumber))->load();
    settings.robotisation = apvts.getRawParameterValue(SPECTRUM_BITCRUSHER_ID + "Robotisation " + std::to_string(parameterNumber))->load();
    settings.DCoffsetRemove = apvts.getRawParameterValue(SPECTRUM_BITCRUSHER_ID + "DCoffset Enabled " + std::to_string(parameterNumber))->load() > 0.5f;
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
    fxDistributionSlider(*pluginState.apvts.getParameter(SPECTRUM_BITCRUSHER_ID + "Fx Distribution " + std::to_string(parameterNumber)), "%"),
    biasSlider(*pluginState.apvts.getParameter(SPECTRUM_BITCRUSHER_ID + "Bias " + std::to_string(parameterNumber)), ""),
    symmetrySlider(*pluginState.apvts.getParameter(SPECTRUM_BITCRUSHER_ID + "Symmetry " + std::to_string(parameterNumber)), "%"),
    binsReduxSlider(*pluginState.apvts.getParameter(SPECTRUM_BITCRUSHER_ID + "Bins Redux " + std::to_string(parameterNumber)), ""),
    robotisationSlider(*pluginState.apvts.getParameter(SPECTRUM_BITCRUSHER_ID + "Robotisation " + std::to_string(parameterNumber)), ""),
    driveSliderAttachment(pluginState.apvts, SPECTRUM_BITCRUSHER_ID + "Drive " + std::to_string(parameterNumber), driveSlider),
    mixSliderAttachment(pluginState.apvts, SPECTRUM_BITCRUSHER_ID + "Mix " + std::to_string(parameterNumber), mixSlider),
    fxDistributionSliderAttachment(pluginState.apvts, SPECTRUM_BITCRUSHER_ID + "Fx Distribution " + std::to_string(parameterNumber), fxDistributionSlider),
    biasSliderAttachment(pluginState.apvts, SPECTRUM_BITCRUSHER_ID + "Bias " + std::to_string(parameterNumber), biasSlider),
    symmetrySliderAttachment(pluginState.apvts, SPECTRUM_BITCRUSHER_ID + "Symmetry " + std::to_string(parameterNumber), symmetrySlider),
    binsReduxSliderAttachment(pluginState.apvts, SPECTRUM_BITCRUSHER_ID + "Bins Redux " + std::to_string(parameterNumber), binsReduxSlider),
    robotisationSliderAttachment(pluginState.apvts, SPECTRUM_BITCRUSHER_ID + "Robotisation " + std::to_string(parameterNumber), robotisationSlider),
    DCoffsetEnabledButtonAttachment(pluginState.apvts, SPECTRUM_BITCRUSHER_ID + "DCoffset Enabled " + std::to_string(parameterNumber), DCoffsetEnabledButton),
    bypassButtonAttachment(pluginState.apvts, SPECTRUM_BITCRUSHER_ID + "Bypassed " + std::to_string(parameterNumber), bypassButton)
{
    // title setup
    title.setText("Spectrum Bitcrusher", juce::dontSendNotification);
    title.setFont(ModuleLookAndFeel::getTitlesFont());

    moduleColor = moduleType_colors.at(ModuleType::SpectrumBitcrusher);

    // labels
    driveLabel.setText("Drive", juce::dontSendNotification);
    driveLabel.setFont(ModuleLookAndFeel::getLabelsFont());
    mixLabel.setText("Mix", juce::dontSendNotification);
    mixLabel.setFont(ModuleLookAndFeel::getLabelsFont());
    fxDistributionLabel.setText("Fx Distribution", juce::dontSendNotification);
    fxDistributionLabel.setFont(ModuleLookAndFeel::getLabelsFont());
    biasLabel.setText("Fx Bias", juce::dontSendNotification);
    biasLabel.setFont(ModuleLookAndFeel::getLabelsFont());
    symmetryLabel.setText("Symmetry", juce::dontSendNotification);
    symmetryLabel.setFont(ModuleLookAndFeel::getLabelsFont());
    binsReduxLabel.setText("Freq Resolution", juce::dontSendNotification);
    binsReduxLabel.setFont(ModuleLookAndFeel::getLabelsFont());
    robotisationLabel.setText("Robotisation", juce::dontSendNotification);
    robotisationLabel.setFont(ModuleLookAndFeel::getLabelsFont());
    DCoffsetEnabledButtonLabel.setText("DC Filter", juce::dontSendNotification);
    DCoffsetEnabledButtonLabel.setFont(ModuleLookAndFeel::getLabelsFont());

    driveSlider.labels.add({ 0.f, "0dB" });
    driveSlider.labels.add({ 1.f, "40dB" });
    mixSlider.labels.add({ 0.f, "0%" });
    mixSlider.labels.add({ 1.f, "100%" });
    fxDistributionSlider.labels.add({ 0.f, "-100%" });
    fxDistributionSlider.labels.add({ 1.f, "+100%" });
    biasSlider.labels.add({ 0.f, "-0.9" });
    biasSlider.labels.add({ 1.f, "+0.9" });
    symmetrySlider.labels.add({ 0.f, "-100%" });
    symmetrySlider.labels.add({ 1.f, "+100%" });
    binsReduxSlider.labels.add({ 0.f, "16" });
    binsReduxSlider.labels.add({ 1.f, "1024" });
    robotisationSlider.labels.add({ 0.f, "0%" });
    robotisationSlider.labels.add({ 1.f, "100%" });

    // bypass button
    lnf.color = moduleType_colors.at(ModuleType::SpectrumBitcrusher);
    bypassButton.setLookAndFeel(&lnf);

    // dcFilter
    DCoffsetEnabledButton.setLookAndFeel(&dcOffsetLnf);

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
    fxDistributionSlider.setTooltip("Apply the signal processing to the positive or negative area of the waveform");
    biasSlider.setTooltip("Set the value which determines the bias between the positive or negative area of the waveform");
    symmetrySlider.setTooltip("Apply symmetry to the waveform moving it from the center");
    binsReduxSlider.setTooltip("Set the bins rate for the reduction of the spectrum resolution");
    robotisationSlider.setTooltip("Select the amount of robotisation of the input signal");
    DCoffsetEnabledButtonLabel.setTooltip("Enable a low-frequency highpass filter to remove any DC offset in the signal");

    for (auto* comp : getAllComps())
    {
        addAndMakeVisible(comp);
    }

    handleParamCompsEnablement(bypassButton.getToggleState());
}

SpectrumBitcrusherModuleGUI::~SpectrumBitcrusherModuleGUI()
{
    bypassButton.setLookAndFeel(nullptr);
    DCoffsetEnabledButton.setLookAndFeel(nullptr);
}

std::vector<juce::Component*> SpectrumBitcrusherModuleGUI::getAllComps()
{
    return {
        &title,
        &driveSlider,
        &mixSlider,
        &fxDistributionSlider,
        &biasSlider,
        &symmetrySlider,
        &binsReduxSlider,
        &robotisationSlider,
        &DCoffsetEnabledButton,
        // labels
        &driveLabel,
        &mixLabel,
        &fxDistributionLabel,
        &biasLabel,
        &symmetryLabel,
        &binsReduxLabel,
        &robotisationLabel,
        &DCoffsetEnabledButtonLabel,
        // bypass
        &bypassButton
    };
}

std::vector<juce::Component*> SpectrumBitcrusherModuleGUI::getParamComps()
{
    return {
        &driveSlider,
        &mixSlider,
        &fxDistributionSlider,
        &biasSlider,
        &symmetrySlider,
        &binsReduxSlider,
        &robotisationSlider,
        &DCoffsetEnabledButton
    };
}

void SpectrumBitcrusherModuleGUI::updateParameters(const juce::Array<juce::var>& values)
{
    auto value = values.begin();

    bypassButton.setToggleState(*(value++), juce::NotificationType::sendNotificationSync);
    driveSlider.setValue(*(value++), juce::NotificationType::sendNotificationSync);
    mixSlider.setValue(*(value++), juce::NotificationType::sendNotificationSync);
    fxDistributionSlider.setValue(*(value++), juce::NotificationType::sendNotificationSync);
    biasSlider.setValue(*(value++), juce::NotificationType::sendNotificationSync);
    symmetrySlider.setValue(*(value++), juce::NotificationType::sendNotificationSync);
    binsReduxSlider.setValue(*(value++), juce::NotificationType::sendNotificationSync);
    robotisationSlider.setValue(*(value++), juce::NotificationType::sendNotificationSync);
    DCoffsetEnabledButton.setToggleState(*(value++), juce::NotificationType::sendNotificationSync);
}

void SpectrumBitcrusherModuleGUI::resetParameters(unsigned int parameterNumber)
{
    auto drive = pluginState.apvts.getParameter(SPECTRUM_BITCRUSHER_ID + "Drive " + std::to_string(parameterNumber));
    auto mix = pluginState.apvts.getParameter(SPECTRUM_BITCRUSHER_ID + "Mix " + std::to_string(parameterNumber));
    auto fxDistribution = pluginState.apvts.getParameter(SPECTRUM_BITCRUSHER_ID + "Fx Distribution " + std::to_string(parameterNumber));
    auto bias = pluginState.apvts.getParameter(SPECTRUM_BITCRUSHER_ID + "Bias " + std::to_string(parameterNumber));
    auto symmetry = pluginState.apvts.getParameter(SPECTRUM_BITCRUSHER_ID + "Symmetry " + std::to_string(parameterNumber));
    auto binsRedux = pluginState.apvts.getParameter(SPECTRUM_BITCRUSHER_ID + "Bins Redux " + std::to_string(parameterNumber));
    auto robotisation = pluginState.apvts.getParameter(SPECTRUM_BITCRUSHER_ID + "Robotisation " + std::to_string(parameterNumber));
    auto dcOffset = pluginState.apvts.getParameter(SPECTRUM_BITCRUSHER_ID + "DCoffset Enabled " + std::to_string(parameterNumber));
    auto bypassed = pluginState.apvts.getParameter(SPECTRUM_BITCRUSHER_ID + "Bypassed " + std::to_string(parameterNumber));

    drive->setValueNotifyingHost(drive->getDefaultValue());
    mix->setValueNotifyingHost(mix->getDefaultValue());
    fxDistribution->setValueNotifyingHost(fxDistribution->getDefaultValue());
    bias->setValueNotifyingHost(bias->getDefaultValue());
    symmetry->setValueNotifyingHost(symmetry->getDefaultValue());
    binsRedux->setValueNotifyingHost(binsRedux->getDefaultValue());
    robotisation->setValueNotifyingHost(robotisation->getDefaultValue());
    dcOffset->setValueNotifyingHost(dcOffset->getDefaultValue());
    bypassed->setValueNotifyingHost(bypassed->getDefaultValue());
}

juce::Array<juce::var> SpectrumBitcrusherModuleGUI::getParamValues()
{
    juce::Array<juce::var> values;

    values.add(juce::var(bypassButton.getToggleState()));
    values.add(juce::var(driveSlider.getValue()));
    values.add(juce::var(mixSlider.getValue()));
    values.add(juce::var(fxDistributionSlider.getValue()));
    values.add(juce::var(biasSlider.getValue()));
    values.add(juce::var(symmetrySlider.getValue()));
    values.add(juce::var(binsReduxSlider.getValue()));
    values.add(juce::var(robotisationSlider.getValue()));
    values.add(juce::var(DCoffsetEnabledButton.getToggleState()));

    return values;
}

void SpectrumBitcrusherModuleGUI::resized()
{
    auto bitcrusherArea = getContentRenderArea();

    // bypass
    auto temp = bitcrusherArea;
    auto bypassButtonArea = temp.removeFromTop(25);

    auto size = 32.f;
    bypassButtonArea.setSize(size, size);
    bypassButton.setBounds(bypassButtonArea);
    bypassButton.setCentreRelative(0.23f, 0.088f);

    auto titleAndBypassArea = bitcrusherArea.removeFromTop(30);
    titleAndBypassArea.translate(0, 4);

    bitcrusherArea.translate(0, 8);

    auto topArea = bitcrusherArea.removeFromTop(bitcrusherArea.getHeight() * (1.f / 3.f));
    auto middleArea = bitcrusherArea.removeFromTop(bitcrusherArea.getHeight() * (1.f / 2.f));
    auto bottomArea = bitcrusherArea;

    auto topLabelsArea = topArea.removeFromTop(14);
    auto middleLabelsArea = middleArea.removeFromTop(14);
    auto bottomLabelsArea = bottomArea.removeFromTop(14);

    // label areas
    auto driveLabelArea = topLabelsArea.removeFromLeft(topLabelsArea.getWidth() * (1.f / 2.f));
    auto mixLabelArea = topLabelsArea;
    temp = middleLabelsArea.removeFromLeft(middleLabelsArea.getWidth() * (1.f / 2.f));
    auto fxDistributionLabelArea = temp.removeFromLeft(temp.getWidth() * (1.f / 2.f));
    auto biasLabelArea = temp;
    auto symmetryLabelArea = middleLabelsArea.removeFromLeft(middleLabelsArea.getWidth() * (1.f / 2.f));
    auto DCoffsetRemoveLabelArea = middleLabelsArea;
    auto rateLabelArea = bottomLabelsArea.removeFromLeft(bottomLabelsArea.getWidth() * (1.f / 2.f));
    auto bitLabelArea = bottomLabelsArea;

    // slider areas
    auto driveArea = topArea.removeFromLeft(topArea.getWidth() * (1.f / 2.f));
    auto mixArea = topArea;
    temp = middleArea.removeFromLeft(middleArea.getWidth() * (1.f / 2.f));
    auto fxDistributionArea = temp.removeFromLeft(temp.getWidth() * (1.f / 2.f));
    auto biasArea = temp;
    auto symmetryArea = middleArea.removeFromLeft(middleArea.getWidth() * (1.f / 2.f));
    auto DCoffsetRemoveArea = middleArea;
    auto rateArea = bottomArea.removeFromLeft(bottomArea.getWidth() * (1.f / 2.f));
    auto bitArea = bottomArea;

    juce::Rectangle<int> renderArea;
    renderArea.setSize(fxDistributionArea.getWidth(), fxDistributionArea.getWidth());
    renderArea.reduce(25, 25);

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

    renderArea.setCentre(fxDistributionArea.getCentre());
    renderArea.setY(fxDistributionArea.getTopLeft().getY());
    fxDistributionSlider.setBounds(renderArea);
    fxDistributionLabel.setBounds(fxDistributionLabelArea);
    fxDistributionLabel.setJustificationType(juce::Justification::centred);

    renderArea.setCentre(biasArea.getCentre());
    renderArea.setY(biasArea.getTopLeft().getY());
    biasSlider.setBounds(renderArea);
    biasLabel.setBounds(biasLabelArea);
    biasLabel.setJustificationType(juce::Justification::centred);

    renderArea.setCentre(symmetryArea.getCentre());
    renderArea.setY(symmetryArea.getTopLeft().getY());
    symmetrySlider.setBounds(renderArea);
    symmetryLabel.setBounds(symmetryLabelArea);
    symmetryLabel.setJustificationType(juce::Justification::centred);

    DCoffsetRemoveArea.reduce(46.f, 19.f);
    //DCoffsetRemoveArea.reduce(JUCE_LIVE_CONSTANT(85.f), 
    //    JUCE_LIVE_CONSTANT(65.f));
    DCoffsetEnabledButton.setBounds(DCoffsetRemoveArea);
    /*DCoffsetEnabledButton.setTransform(juce::AffineTransform::scale(JUCE_LIVE_CONSTANT(1.f)).translated(
        JUCE_LIVE_CONSTANT(-8.45953f),
        JUCE_LIVE_CONSTANT(-10.31)));*/
    DCoffsetEnabledButton.setTransform(juce::AffineTransform::scale(1.f).translated(-8.45953f, -8.90813f));
    DCoffsetEnabledButtonLabel.setBounds(DCoffsetRemoveLabelArea);
    DCoffsetEnabledButtonLabel.setJustificationType(juce::Justification::centred);
    /*DCoffsetEnabledButtonLabel.setCentreRelative(JUCE_LIVE_CONSTANT(0.9f),
        JUCE_LIVE_CONSTANT(0.4327f));*/
    DCoffsetEnabledButtonLabel.setCentreRelative(0.843f, 0.4327f);

    // renderArea.setSize(rateArea.getHeight(), rateArea.getHeight());

    renderArea.setCentre(rateArea.getCentre());
    renderArea.setY(rateArea.getTopLeft().getY());
    binsReduxSlider.setBounds(renderArea);
    binsReduxLabel.setBounds(rateLabelArea);
    binsReduxLabel.setJustificationType(juce::Justification::centred);

    renderArea.setCentre(bitArea.getCentre());
    renderArea.setY(bitArea.getTopLeft().getY());
    robotisationSlider.setBounds(renderArea);
    robotisationLabel.setBounds(bitLabelArea);
    robotisationLabel.setJustificationType(juce::Justification::centred);

}