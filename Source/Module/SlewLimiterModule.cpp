/*
  ==============================================================================

    SlewLimiterModule.cpp

    Copyright (c) 2021 KillBizz - Gabriel Bizzo

  ==============================================================================
*/

/*
  ==============================================================================

    Copyright (c) 2017 Ivan Cohen
    Content: the original Slew Limiter Algorithm
    Source: https://www.dropbox.com/s/cjq4t08u6pqkaas/ADC17FiftyShadesDistortion.zip?dl=0

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

#include "SlewLimiterModule.h"

#include "FilterModule.h"

//==============================================================================

/* SlewLimiterModule DSP */

//==============================================================================

SlewLimiterModuleDSP::SlewLimiterModuleDSP(juce::AudioProcessorValueTreeState& _apvts)
    : DSPModule(_apvts)
{
}

void SlewLimiterModuleDSP::updateDSPState(double sampleRate)
{
    auto settings = getSettings(apvts, parameterNumber);

    bypassed = settings.bypassed;

    auto symmetryValue = juce::jmap(settings.symmetry, -100.f, 100.f, 0.f, 2.f);

    auto mix = settings.mix * 0.01f;
    dryGain.setTargetValue(1.f - mix);
    wetGain.setTargetValue(mix);
    driveGain.setTargetValue(juce::Decibels::decibelsToGain(settings.drive));
    fxDistribution.setTargetValue(settings.fxDistribution * 0.01f);
    symmetry.setTargetValue(symmetryValue);
    bias.setTargetValue(settings.bias);

    rise.setTargetValue(settings.rise * 0.01f);
    fall.setTargetValue(settings.fall * 0.01f);

    DCoffsetRemoveEnabled = settings.DCoffsetRemove;
}

void SlewLimiterModuleDSP::prepareToPlay(double sampleRate, int samplesPerBlock)
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

    updateDSPState(sampleRate);

    wetBuffer.setSize(2, samplesPerBlock, false, true, true); // clears
    tempBuffer.setSize(2, samplesPerBlock, false, true, true); // clears
}

void SlewLimiterModuleDSP::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages, double sampleRate)
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

        // Drive
        driveGain.applyGain(wetBuffer, numSamples);

        // Temp Buffer feeding for applying fxDistribution
        for (auto channel = 0; channel < 2; channel++)
            tempBuffer.copyFrom(channel, 0, wetBuffer, channel, 0, numSamples);

        // internal variables
        auto Ts = 1.f / sampleRate;
        float slewRise = slewMax * Ts * std::pow(slewMin / slewMax, rise.getNextValue());
        float slewFall = slewMax * Ts * std::pow(slewMin / slewMax, fall.getNextValue());

        // TODO : aggiungo MIX tra 2 diverse computazioni dei valori slewRise e slewFall per diversi effetti

        // trying to compute slewRise/Fall values with different functions (exponential is the best)
        //float slewRise = 0.000001f / rise.getNextValue();
        //float slewFall = 0.000001f / fall.getNextValue();

        // temporary variable to handle the last output at the end of the processing
        float temp = lastOutput;

        // Processing
        for (auto channel = 0; channel < 2; ++channel)
        {
            auto* channelData = wetBuffer.getWritePointer(channel);
            float output = lastOutput;
            for (auto i = 0; i < numSamples; ++i) {

                auto input = channelData[i];

                // Rise limiting
                if (input > output) {
                    output = jmin(input, output + slewFall);
                }
                // Fall limiting
                else {
                    output = jmax(input, output - slewRise);
                }
                channelData[i] = output;
            }
            temp = output;
        }
        lastOutput = temp;

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

void SlewLimiterModuleDSP::addParameters(juce::AudioProcessorValueTreeState::ParameterLayout& layout)
{
    using namespace juce;

    for (int i = 1; i < 9; ++i) {
        layout.add(std::make_unique<AudioParameterFloat>("SlewLimiter Drive " + std::to_string(i), "SlewLimiter Drive " + std::to_string(i), NormalisableRange<float>(0.f, 40.f, 0.01f), 0.f, "SlewLimiter " + std::to_string(i)));
        layout.add(std::make_unique<AudioParameterFloat>("SlewLimiter Mix " + std::to_string(i), "SlewLimiter Mix " + std::to_string(i), NormalisableRange<float>(0.f, 100.f, 0.01f), 100.f, "SlewLimiter " + std::to_string(i)));
        layout.add(std::make_unique<AudioParameterFloat>("SlewLimiter Fx Distribution " + std::to_string(i), "SlewLimiter Fx Distribution " + std::to_string(i), NormalisableRange<float>(-100.f, 100.f, 1.f), 0.f, "SlewLimiter " + std::to_string(i)));
        layout.add(std::make_unique<AudioParameterFloat>("SlewLimiter Symmetry " + std::to_string(i), "SlewLimiter Symmetry " + std::to_string(i), NormalisableRange<float>(-100.f, 100.f, 1.f), 0.f, "SlewLimiter " + std::to_string(i)));
        layout.add(std::make_unique<AudioParameterFloat>("SlewLimiter Bias " + std::to_string(i), "SlewLimiter Bias " + std::to_string(i), NormalisableRange<float>(-0.9f, 0.9f, 0.01f), 0.f, "SlewLimiter " + std::to_string(i)));
        layout.add(std::make_unique<AudioParameterFloat>("SlewLimiter Rise " + std::to_string(i), "SlewLimiter Rise " + std::to_string(i), NormalisableRange<float>(0.f, 100.f, 0.01f), 0.f, "SlewLimiter " + std::to_string(i)));
        layout.add(std::make_unique<AudioParameterFloat>("SlewLimiter Fall " + std::to_string(i), "SlewLimiter Fall " + std::to_string(i), NormalisableRange<float>(0.f, 100.f, 0.01f), 0.f, "SlewLimiter " + std::to_string(i)));
        layout.add(std::make_unique<AudioParameterBool>("SlewLimiter DCoffset Enabled " + std::to_string(i), "SlewLimiter DCoffset Enabled " + std::to_string(i), true, "SlewLimiter " + std::to_string(i)));
        layout.add(std::make_unique<AudioParameterBool>("SlewLimiter Bypassed " + std::to_string(i), "SlewLimiter Bypassed " + std::to_string(i), false, "SlewLimiter " + std::to_string(i)));
    }
}

SlewLimiterSettings SlewLimiterModuleDSP::getSettings(juce::AudioProcessorValueTreeState& apvts, unsigned int parameterNumber)
{
    SlewLimiterSettings settings;

    settings.drive = apvts.getRawParameterValue("SlewLimiter Drive " + std::to_string(parameterNumber))->load();
    settings.mix = apvts.getRawParameterValue("SlewLimiter Mix " + std::to_string(parameterNumber))->load();
    settings.fxDistribution = apvts.getRawParameterValue("SlewLimiter Fx Distribution " + std::to_string(parameterNumber))->load();
    settings.symmetry = apvts.getRawParameterValue("SlewLimiter Symmetry " + std::to_string(parameterNumber))->load();
    settings.bias = apvts.getRawParameterValue("SlewLimiter Bias " + std::to_string(parameterNumber))->load();
    settings.rise = apvts.getRawParameterValue("SlewLimiter Rise " + std::to_string(parameterNumber))->load();
    settings.fall = apvts.getRawParameterValue("SlewLimiter Fall " + std::to_string(parameterNumber))->load();
    settings.DCoffsetRemove = apvts.getRawParameterValue("SlewLimiter DCoffset Enabled " + std::to_string(parameterNumber))->load() > 0.5f;
    settings.bypassed = apvts.getRawParameterValue("SlewLimiter Bypassed " + std::to_string(parameterNumber))->load() > 0.5f;

    return settings;
}

//==============================================================================

/* SlewLimiterModule GUI */

//==============================================================================

SlewLimiterModuleGUI::SlewLimiterModuleGUI(PluginState& p, unsigned int parameterNumber)
    : GUIModule(), pluginState(p),
    driveSlider(*pluginState.apvts.getParameter("SlewLimiter Drive " + std::to_string(parameterNumber)), "dB"),
    mixSlider(*pluginState.apvts.getParameter("SlewLimiter Mix " + std::to_string(parameterNumber)), "%"),
    fxDistributionSlider(*pluginState.apvts.getParameter("SlewLimiter Fx Distribution " + std::to_string(parameterNumber)), "%"),
    symmetrySlider(*pluginState.apvts.getParameter("SlewLimiter Symmetry " + std::to_string(parameterNumber)), "%"),
    biasSlider(*pluginState.apvts.getParameter("SlewLimiter Bias " + std::to_string(parameterNumber)), ""),
    slewLimiterRiseSlider(*pluginState.apvts.getParameter("SlewLimiter Rise " + std::to_string(parameterNumber)), "%"),
    slewLimiterFallSlider(*pluginState.apvts.getParameter("SlewLimiter Fall " + std::to_string(parameterNumber)), "%"),
    driveSliderAttachment(pluginState.apvts, "SlewLimiter Drive " + std::to_string(parameterNumber), driveSlider),
    mixSliderAttachment(pluginState.apvts, "SlewLimiter Mix " + std::to_string(parameterNumber), mixSlider),
    fxDistributionSliderAttachment(pluginState.apvts, "SlewLimiter Fx Distribution " + std::to_string(parameterNumber), fxDistributionSlider),
    symmetrySliderAttachment(pluginState.apvts, "SlewLimiter Symmetry " + std::to_string(parameterNumber), symmetrySlider),
    biasSliderAttachment(pluginState.apvts, "SlewLimiter Bias " + std::to_string(parameterNumber), biasSlider),
    slewLimiterRiseSliderAttachment(pluginState.apvts, "SlewLimiter Rise " + std::to_string(parameterNumber), slewLimiterRiseSlider),
    slewLimiterFallSliderAttachment(pluginState.apvts, "SlewLimiter Fall " + std::to_string(parameterNumber), slewLimiterFallSlider),
    DCoffsetEnabledButtonAttachment(pluginState.apvts, "SlewLimiter DCoffset Enabled " + std::to_string(parameterNumber), DCoffsetEnabledButton),
    bypassButtonAttachment(pluginState.apvts, "SlewLimiter Bypassed " + std::to_string(parameterNumber), bypassButton)
{
    // title setup
    title.setText("Slew Limiter", juce::dontSendNotification);
    title.setFont(ModuleLookAndFeel::getTitlesFont());

    moduleColor = moduleType_colors.at(ModuleType::SlewLimiter);

    // labels
    driveLabel.setText("Drive", juce::dontSendNotification);
    driveLabel.setFont(ModuleLookAndFeel::getLabelsFont());
    mixLabel.setText("Mix", juce::dontSendNotification);
    mixLabel.setFont(ModuleLookAndFeel::getLabelsFont());
    fxDistributionLabel.setText("Fx Distribution", juce::dontSendNotification);
    fxDistributionLabel.setFont(ModuleLookAndFeel::getLabelsFont());
    symmetryLabel.setText("Symmetry", juce::dontSendNotification);
    symmetryLabel.setFont(ModuleLookAndFeel::getLabelsFont());
    biasLabel.setText("Fx Bias", juce::dontSendNotification);
    biasLabel.setFont(ModuleLookAndFeel::getLabelsFont());
    slewLimiterRiseLabel.setText("Rise", juce::dontSendNotification);
    slewLimiterRiseLabel.setFont(ModuleLookAndFeel::getLabelsFont());
    slewLimiterFallLabel.setText("Fall", juce::dontSendNotification);
    slewLimiterFallLabel.setFont(ModuleLookAndFeel::getLabelsFont());
    DCoffsetEnabledButtonLabel.setText("DC Filter", juce::dontSendNotification);
    DCoffsetEnabledButtonLabel.setFont(ModuleLookAndFeel::getLabelsFont());

    driveSlider.labels.add({ 0.f, "0dB" });
    driveSlider.labels.add({ 1.f, "40dB" });
    mixSlider.labels.add({ 0.f, "0%" });
    mixSlider.labels.add({ 1.f, "100%" });
    fxDistributionSlider.labels.add({ 0.f, "-100%" });
    fxDistributionSlider.labels.add({ 1.f, "+100%" });
    symmetrySlider.labels.add({ 0.f, "-100%" });
    symmetrySlider.labels.add({ 1.f, "+100%" });
    biasSlider.labels.add({ 0.f, "-0.9" });
    biasSlider.labels.add({ 1.f, "+0.9" });
    slewLimiterRiseSlider.labels.add({ 0.f, "0%" });
    slewLimiterRiseSlider.labels.add({ 1.f, "100%" });
    slewLimiterFallSlider.labels.add({ 0.f, "0%" });
    slewLimiterFallSlider.labels.add({ 1.f, "100%" });

    // bypass button
    lnf.color = moduleType_colors.at(ModuleType::SlewLimiter);
    bypassButton.setLookAndFeel(&lnf);

    // dcFilter
    DCoffsetEnabledButton.setLookAndFeel(&dcOffsetLnf);

    auto safePtr = juce::Component::SafePointer<SlewLimiterModuleGUI>(this);
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
    symmetrySlider.setTooltip("Apply symmetry to the waveform moving it from the center");
    biasSlider.setTooltip("Set the value which determines the bias between the positive or negative area of the waveform");
    slewLimiterRiseSlider.setTooltip("Set the maximum change speed of the waveform during the rising phase");
    slewLimiterFallSlider.setTooltip("Set the maximum change speed of the waveform during the falling phase");
    DCoffsetEnabledButtonLabel.setTooltip("Enable a low-frequency highpass filter to remove any DC offset in the signal");

    for (auto* comp : getAllComps())
    {
        addAndMakeVisible(comp);
    }

    handleParamCompsEnablement(bypassButton.getToggleState());
}

SlewLimiterModuleGUI::~SlewLimiterModuleGUI()
{
    bypassButton.setLookAndFeel(nullptr);
    DCoffsetEnabledButton.setLookAndFeel(nullptr);
}

std::vector<juce::Component*> SlewLimiterModuleGUI::getAllComps()
{
    return {
        &title,
        &driveSlider,
        &mixSlider,
        &fxDistributionSlider,
        &symmetrySlider,
        &biasSlider,
        &slewLimiterRiseSlider,
        &slewLimiterFallSlider,
        &DCoffsetEnabledButton,
        // labels
        &driveLabel,
        &mixLabel,
        &fxDistributionLabel,
        &symmetryLabel,
        &biasLabel,
        &slewLimiterRiseLabel,
        &slewLimiterFallLabel,
        &DCoffsetEnabledButtonLabel,
        // bypass
        &bypassButton
    };
}

std::vector<juce::Component*> SlewLimiterModuleGUI::getParamComps()
{
    return {
        &driveSlider,
        &mixSlider,
        &fxDistributionSlider,
        &symmetrySlider,
        &biasSlider,
        &slewLimiterRiseSlider,
        &slewLimiterFallSlider,
        &DCoffsetEnabledButton
    };
}

void SlewLimiterModuleGUI::updateParameters(const juce::Array<juce::var>& values)
{
    auto value = values.begin();

    bypassButton.setToggleState(*(value++), juce::NotificationType::sendNotificationSync);
    driveSlider.setValue(*(value++), juce::NotificationType::sendNotificationSync);
    mixSlider.setValue(*(value++), juce::NotificationType::sendNotificationSync);
    fxDistributionSlider.setValue(*(value++), juce::NotificationType::sendNotificationSync);
    symmetrySlider.setValue(*(value++), juce::NotificationType::sendNotificationSync);
    biasSlider.setValue(*(value++), juce::NotificationType::sendNotificationSync);
    slewLimiterRiseSlider.setValue(*(value++), juce::NotificationType::sendNotificationSync);
    slewLimiterFallSlider.setValue(*(value++), juce::NotificationType::sendNotificationSync);
    DCoffsetEnabledButton.setToggleState(*(value++), juce::NotificationType::sendNotificationSync);
}

void SlewLimiterModuleGUI::resetParameters(unsigned int parameterNumber)
{
    auto drive = pluginState.apvts.getParameter("SlewLimiter Drive " + std::to_string(parameterNumber));
    auto mix = pluginState.apvts.getParameter("SlewLimiter Mix " + std::to_string(parameterNumber));
    auto fxDistribution = pluginState.apvts.getParameter("SlewLimiter Fx Distribution " + std::to_string(parameterNumber));
    auto symmetry = pluginState.apvts.getParameter("SlewLimiter Symmetry " + std::to_string(parameterNumber));
    auto bias = pluginState.apvts.getParameter("SlewLimiter Bias " + std::to_string(parameterNumber));
    auto rise = pluginState.apvts.getParameter("SlewLimiter Rise " + std::to_string(parameterNumber));
    auto fall = pluginState.apvts.getParameter("SlewLimiter Fall " + std::to_string(parameterNumber));
    auto dcOffset = pluginState.apvts.getParameter("SlewLimiter DCoffset Enabled " + std::to_string(parameterNumber));
    auto bypassed = pluginState.apvts.getParameter("SlewLimiter Bypassed " + std::to_string(parameterNumber));

    drive->setValueNotifyingHost(drive->getDefaultValue());
    mix->setValueNotifyingHost(mix->getDefaultValue());
    fxDistribution->setValueNotifyingHost(fxDistribution->getDefaultValue());
    symmetry->setValueNotifyingHost(symmetry->getDefaultValue());
    bias->setValueNotifyingHost(bias->getDefaultValue());
    rise->setValueNotifyingHost(rise->getDefaultValue());
    fall->setValueNotifyingHost(fall->getDefaultValue());
    dcOffset->setValueNotifyingHost(dcOffset->getDefaultValue());
    bypassed->setValueNotifyingHost(bypassed->getDefaultValue());
}

juce::Array<juce::var> SlewLimiterModuleGUI::getParamValues()
{
    juce::Array<juce::var> values;

    values.add(juce::var(bypassButton.getToggleState()));
    values.add(juce::var(driveSlider.getValue()));
    values.add(juce::var(mixSlider.getValue()));
    values.add(juce::var(fxDistributionSlider.getValue()));
    values.add(juce::var(symmetrySlider.getValue()));
    values.add(juce::var(biasSlider.getValue()));
    values.add(juce::var(slewLimiterRiseSlider.getValue()));
    values.add(juce::var(slewLimiterFallSlider.getValue()));
    values.add(juce::var(DCoffsetEnabledButton.getToggleState()));

    return values;
}

void SlewLimiterModuleGUI::resized()
{
    auto slewLimiterArea = getContentRenderArea();

    // bypass
    auto temp = slewLimiterArea;
    auto bypassButtonArea = temp.removeFromTop(25);

    bypassButtonArea.setWidth(35.f);
    bypassButtonArea.setX(128.f);
    bypassButtonArea.setY(20.f);

    bypassButton.setBounds(bypassButtonArea);

    auto titleAndBypassArea = slewLimiterArea.removeFromTop(30);
    titleAndBypassArea.translate(0, 4);

    slewLimiterArea.translate(0, 8);

    auto topArea = slewLimiterArea.removeFromTop(slewLimiterArea.getHeight() * (1.f / 3.f));
    auto middleArea = slewLimiterArea.removeFromTop(slewLimiterArea.getHeight() * (1.f / 2.f));
    auto bottomArea = slewLimiterArea;

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
    auto riseLabelArea = bottomLabelsArea.removeFromLeft(bottomLabelsArea.getWidth() * (1.f / 2.f));
    auto fallLabelArea = bottomLabelsArea;

    // slider areas
    auto driveArea = topArea.removeFromLeft(topArea.getWidth() * (1.f / 2.f));
    auto mixArea = topArea;
    temp = middleArea.removeFromLeft(middleArea.getWidth() * (1.f / 2.f));
    auto fxDistributionArea = temp.removeFromLeft(temp.getWidth() * (1.f / 2.f));
    auto biasArea = temp;
    auto symmetryArea = middleArea.removeFromLeft(middleArea.getWidth() * (1.f / 2.f));
    auto DCoffsetRemoveArea = middleArea;
    auto riseArea = bottomArea.removeFromLeft(bottomArea.getWidth() * (1.f / 2.f));
    auto fallArea = bottomArea;

    juce::Rectangle<int> renderArea;
    renderArea.setSize(fxDistributionArea.getWidth(), fxDistributionArea.getWidth());
    renderArea.reduce(25, 0);

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

    // DCoffsetRemoveArea.reduce(85.f, 65.f);
    DCoffsetEnabledButton.setBounds(DCoffsetRemoveArea);
    //DCoffsetEnabledButton.setTransform(juce::AffineTransform::scale(2.2f).translated(JUCE_LIVE_CONSTANT(-615.f),
    //    JUCE_LIVE_CONSTANT(-289.f)));
    DCoffsetEnabledButton.setTransform(juce::AffineTransform::scale(2.2f).translated(-517.991f, -289.f));
    DCoffsetEnabledButtonLabel.setBounds(DCoffsetRemoveLabelArea);
    DCoffsetEnabledButtonLabel.setJustificationType(juce::Justification::centred);
    /*DCoffsetEnabledButtonLabel.setCentreRelative(JUCE_LIVE_CONSTANT(0.9f),
        JUCE_LIVE_CONSTANT(0.4327f));*/
    DCoffsetEnabledButtonLabel.setCentreRelative(0.843f, 0.4327f);

    // renderArea.setSize(riseArea.getHeight(), riseArea.getHeight());

    renderArea.setCentre(riseArea.getCentre());
    renderArea.setY(riseArea.getTopLeft().getY());
    slewLimiterRiseSlider.setBounds(renderArea);
    slewLimiterRiseLabel.setBounds(riseLabelArea);
    slewLimiterRiseLabel.setJustificationType(juce::Justification::centred);

    renderArea.setCentre(fallArea.getCentre());
    renderArea.setY(fallArea.getTopLeft().getY());
    slewLimiterFallSlider.setBounds(renderArea);
    slewLimiterFallLabel.setBounds(fallLabelArea);
    slewLimiterFallLabel.setJustificationType(juce::Justification::centred);

}