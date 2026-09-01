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

#include "../EqualizerModule/EqualizerModule.h"

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
        float slewRise = slewMax * Ts * std::pow(slewMin / slewMax, fall.getNextValue());
        float slewFall = slewMax * Ts * std::pow(slewMin / slewMax, rise.getNextValue());

        // TODO : aggiungo MIX tra 2 diverse computazioni dei valori slewRise e slewFall per diversi effetti

        // trying to compute slewRise/Fall values with different functions (exponential is the best)
        //float slewRise = 0.000001f / rise.getNextValue();
        //float slewFall = 0.000001f / fall.getNextValue();

        // Processing.
        // The running output is kept per channel: sharing one value meant the left channel
        // started every block from the right channel's last sample, which slewed the two
        // channels differently and pulled the stereo image apart.
        for (auto channel = 0; channel < 2; ++channel)
        {
            auto* channelData = wetBuffer.getWritePointer(channel);
            float output = lastOutput[channel];
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
            lastOutput[channel] = output;
        }

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
    : GUIModule(), pluginState(p), parameterNumber(parameterNumber),
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
    setModuleHelp("Slew Limiter",
        "Limits how quickly the waveform can rise and fall. Slow settings round transients into smooth distortion; different Rise and Fall values create asymmetry.");

    // --- graphs ---
    // Visual model of the slew limiter: the rise and fall knobs cap how much the signal may
    // change from one sample to the next, which turns a sine into a triangle-like shape.
    moduleGraph = std::make_unique<WaveShapeVisualiser>(moduleColor,
        [this](std::vector<float>& samples) {
            const auto settings = SlewLimiterModuleDSP::getSettings(pluginState.apvts, this->parameterNumber);
            // Mirrors the DSP, where the allowed slope is slewMax * Ts * (slewMin/slewMax)^amount:
            // 0% lets the signal through untouched and 100% limits hardest. The range is scaled
            // to this graph, whose reference cycle has a maximum slope of
            // 2*pi*cycles/resolution (~0.049) per sample - a linear map saturated near 30%.
            const auto slopeFor = [](float percent) {
                const auto normalised = juce::jlimit(0.0f, 1.0f, percent * 0.01f);
                return 0.06f * std::pow(1.0f / 150.0f, normalised);
            };
            const auto riseLimit = slopeFor(settings.rise);
            const auto fallLimit = slopeFor(settings.fall);

            auto last = samples.empty() ? 0.0f : samples[0];
            for (auto& sample : samples)
            {
                const auto delta = sample - last;
                const auto limit = delta >= 0.0f ? riseLimit : fallLimit;
                last += juce::jlimit(-limit, limit, delta);
                sample = last;
            }
        });
    moduleGraph->setTooltip(
        "What this module does to the waveform: the signal can only rise and fall at a "
        "limited rate, so steep slopes turn into ramps. Lower rise/fall values limit harder.");
    moduleGraph->setCaption("slew");

    sharedParamsGraph = std::make_unique<WaveShapeVisualiser>(moduleColor,
        [this](std::vector<float>& samples) {
            const auto settings = SlewLimiterModuleDSP::getSettings(pluginState.apvts, this->parameterNumber);
            SharedParamsLayout::applySharedParameters(samples, settings.drive, settings.mix,
                settings.fxDistribution, settings.bias, settings.symmetry, settings.DCoffsetRemove);
        });
    sharedParamsGraph->setTooltip(
        "The transfer function these controls apply to the signal, drawn over the faint input "
        "curve. Drive sets how hard the signal is pushed into the effect, fx bias marks the "
        "threshold that splits the waveform and fx distribution chooses how much of the effect "
        "each side of it receives, symmetry weights the positive against the negative half, the "
        "dc filter recentres the result on zero, and mix blends it back with the clean signal.");

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
    DCoffsetEnabledButton.setTooltip("Enable a low-frequency highpass filter to remove any DC offset in the signal");

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
        moduleGraph.get(),
        sharedParamsGraph.get(),
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
    auto area = getContentRenderArea();

    auto titleAndBypassArea = area.removeFromTop(36);
    const auto titleArea = titleAndBypassArea;
    bypassButton.setBounds(juce::Rectangle<int>(headerButtonSize, headerButtonSize).withCentre(
        { titleAndBypassArea.getX() + 20, titleAndBypassArea.getCentreY() }));

    title.setBounds(titleArea);
    title.setJustificationType(juce::Justification::centred);

    area.reduce(6, 8);

    // left: this module's own graph and parameters - right: the shared parameters hexagon
    auto moduleSide = area.removeFromLeft(juce::roundToInt(area.getWidth() * 0.44f));
    area.removeFromLeft(10);
    auto sharedSide = area;

    // --- module side ---
    moduleGraph->setBounds(moduleSide.removeFromTop(juce::roundToInt(moduleSide.getHeight() * 0.46f)));
    moduleSide.removeFromTop(8);

    // the module's own parameters keep their subtle plate
    clearParameterGroups();
    addParameterGroup(moduleSide);

    const auto placeKnob = [](juce::Rectangle<int> cell, juce::Slider& slider, juce::Label& label) {
        label.setBounds(cell.removeFromTop(14));
        label.setJustificationType(juce::Justification::centred);
        // capped so that having only two module parameters does not blow the knobs up out of
        // proportion with the rest of the plugin
        const auto size = juce::jmin(104, juce::jmin(cell.getWidth() - 8, cell.getHeight() - 6));
        slider.setBounds(juce::Rectangle<int>(size, size).withCentre(cell.getCentre()));
    };

    auto moduleKnobs = moduleSide.reduced(6, 8);
    placeKnob(moduleKnobs.removeFromLeft(moduleKnobs.getWidth() / 2), slewLimiterRiseSlider, slewLimiterRiseLabel);
    placeKnob(moduleKnobs, slewLimiterFallSlider, slewLimiterFallLabel);

    // --- shared side: the six common parameters on a hexagon, with their graph in the middle ---
    const auto hexagon = SharedParamsLayout::build(sharedSide);
    sharedParamsGraph->setBounds(hexagon.centre);

    const auto placeSlot = [](const SharedParamsLayout::Slot& slot, juce::Slider& slider, juce::Label& label) {
        label.setBounds(slot.label);
        label.setJustificationType(juce::Justification::centred);
        slider.setBounds(slot.control);
    };

    placeSlot(hexagon.slots[0], driveSlider, driveLabel);
    placeSlot(hexagon.slots[1], mixSlider, mixLabel);
    placeSlot(hexagon.slots[2], symmetrySlider, symmetryLabel);
    placeSlot(hexagon.slots[3], biasSlider, biasLabel);
    placeSlot(hexagon.slots[4], fxDistributionSlider, fxDistributionLabel);

    // the dc filter toggle takes the last vertex
    const auto& dcSlot = hexagon.slots[5];
    DCoffsetEnabledButtonLabel.setBounds(dcSlot.label);
    DCoffsetEnabledButtonLabel.setJustificationType(juce::Justification::centred);
    // the tick box is drawn at the left of the button, so size the button around it to centre it
    DCoffsetEnabledButton.setTransform({});
    const auto toggleHeight = juce::jmin(28, dcSlot.control.getHeight());
    const auto toggleWidth = juce::roundToInt(toggleHeight * 0.825f) + 8;
    DCoffsetEnabledButton.setBounds(
        juce::Rectangle<int>(toggleWidth, toggleHeight).withCentre(dcSlot.control.getCentre()));
}
