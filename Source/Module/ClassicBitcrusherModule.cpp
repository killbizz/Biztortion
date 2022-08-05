/*
  ==============================================================================

    ClassicBitcrusherModule.cpp

    Copyright (c) 2021 KillBizz - Gabriel Bizzo

  ==============================================================================
*/

/*
  ==============================================================================

    Copyright (c) 2018 Joshua Hodge
    Content: the original Time-Domain Bitcrusher Algorithm
    Source: https://github.com/theaudioprogrammer/bitcrusherDemo

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


#include "ClassicBitcrusherModule.h"

//==============================================================================

/* ClassicBitcrusherModule DSP */

//==============================================================================

ClassicBitcrusherModuleDSP::ClassicBitcrusherModuleDSP(juce::AudioProcessorValueTreeState& _apvts)
    : DSPModule(_apvts)
{
}

Array<float> ClassicBitcrusherModuleDSP::getWhiteNoise(int numSamples) {

    Array<float> noise;

    float z0 = 0;
    float z1 = 0;
    bool generate = false;

    float mu = 0; // center (0)
    float sigma = 1; // spread -1 <-> 1

    float output = 0;
    float u1 = 0;
    float u2 = 0;

    Random r = Random::getSystemRandom();
    r.setSeed(Time::getCurrentTime().getMilliseconds());
    const float epsilon = std::numeric_limits<float>::min();

    for (int s = 0; s < numSamples; s++)
    {
        // box muller method
        // https://en.wikipedia.org/wiki/Box%E2%80%93Muller_transform
        generate = !generate;

        if (!generate)
            output = z1 * sigma + mu;
        else
        {
            do
            {
                u1 = r.nextFloat();
                u2 = r.nextFloat();
            } while (u1 <= epsilon);

            z0 = sqrtf(-2.0 * logf(u1)) * cosf(2 * float(double_Pi) * u2);
            z1 = sqrtf(-2.0 * logf(u1)) * sinf(2 * float(double_Pi) * u2);

            output = z0 * sigma + mu;
        }

        // NAN check
        jassert(output == output);
        jassert(output > -50 && output < 50);

        noise.add(output);

    }

    return noise;

}

void ClassicBitcrusherModuleDSP::updateDSPState(double sampleRate)
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
    dither.setTargetValue(settings.dither * 0.01f);
}

void ClassicBitcrusherModuleDSP::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    wetBuffer.setSize(2, samplesPerBlock, false, true, true); // clears
    noiseBuffer.setSize(2, samplesPerBlock, false, true, true); // clears
    tempBuffer.setSize(2, samplesPerBlock, false, true, true); // clears
    updateDSPState(sampleRate);
}

void ClassicBitcrusherModuleDSP::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages, double sampleRate)
{

    updateDSPState(sampleRate);

    if (!bypassed) {

        // SAFETY CHECK :::: since some hosts will change buffer sizes without calling prepToPlay (ex: Bitwig)
        int numSamples = buffer.getNumSamples();
        if (wetBuffer.getNumSamples() != numSamples)
        {
            wetBuffer.setSize(2, numSamples, false, true, true); // clears
            noiseBuffer.setSize(2, numSamples, false, true, true); // clears
            tempBuffer.setSize(2, numSamples, false, true, true); // clears
        }

        // PROCESSING
        
        // Wet Buffer feeding
        for (auto channel = 0; channel < 2; channel++)
            wetBuffer.copyFrom(channel, 0, buffer, channel, 0, numSamples);

        // Drive
        driveGain.applyGain(wetBuffer, numSamples);

        // Temp Buffer feeding for applying asymmetry
        for (auto channel = 0; channel < 2; channel++)
            tempBuffer.copyFrom(channel, 0, wetBuffer, channel, 0, numSamples);

        // Noise building
        noiseBuffer.clear();
        Array<float> noise = getWhiteNoise(numSamples);
        // ADD the noise
        FloatVectorOperations::add(noiseBuffer.getWritePointer(0), noise.getRawDataPointer(), numSamples);
        FloatVectorOperations::add(noiseBuffer.getWritePointer(1), noise.getRawDataPointer(), numSamples);
        // Multiply the noise by the signal ... so 0 signal -> 0 noise
        FloatVectorOperations::multiply(noiseBuffer.getWritePointer(0), wetBuffer.getWritePointer(0), numSamples);
        FloatVectorOperations::multiply(noiseBuffer.getWritePointer(1), wetBuffer.getWritePointer(1), numSamples);
        // Applying gain to the noiseBuffer
        dither.applyGain(noiseBuffer, numSamples);

        // Resampling
        for (int chan = 0; chan < wetBuffer.getNumChannels(); chan++)
        {
            float* data = wetBuffer.getWritePointer(chan);

            for (int i = 0; i < numSamples; i++)
            {
                // REDUCE BIT DEPTH
                float totalQLevels = powf(2.f, bitRedux.getNextValue());
                float val = data[i];
                float remainder = fmodf(val, 1.f / totalQLevels);

                // Quantize
                data[i] = val - remainder;

                // Rate reduction
                int rateReductionRatio = sampleRate / rateRedux.getNextValue();
                if (rateReductionRatio > 1)
                {
                    if (i % rateReductionRatio != 0) data[i] = data[i - i % rateReductionRatio];
                }
            }
        }

        // Add noise to the processed audio (dithering/further distortion)
        wetBuffer.addFrom(0, 0, noiseBuffer.getReadPointer(0), numSamples);
        wetBuffer.addFrom(1, 0, noiseBuffer.getReadPointer(1), numSamples);

        applyAsymmetry(tempBuffer, wetBuffer, symmetry.getNextValue(), bias.getNextValue(), numSamples);

        // Mixing buffers
        dryGain.applyGain(buffer, numSamples);
        wetGain.applyGain(tempBuffer, numSamples);

        for (auto channel = 0; channel < 2; channel++)
            buffer.addFrom(channel, 0, tempBuffer, channel, 0, numSamples);
    }
}

void ClassicBitcrusherModuleDSP::addParameters(juce::AudioProcessorValueTreeState::ParameterLayout& layout)
{
    using namespace juce;

    for (int i = 1; i < 9; ++i) {
        layout.add(std::make_unique<AudioParameterFloat>("CBitcrusher Drive " + std::to_string(i), "Classic Bitcrusher Drive " + std::to_string(i), NormalisableRange<float>(0.f, 40.f, 0.01f), 0.f, "Classic Bitcrusher " + std::to_string(i)));
        layout.add(std::make_unique<AudioParameterFloat>("CBitcrusher Mix " + std::to_string(i), "Classic Bitcrusher Mix " + std::to_string(i), NormalisableRange<float>(0.f, 100.f, 0.01f), 100.f, "Classic Bitcrusher " + std::to_string(i)));
        layout.add(std::make_unique<AudioParameterFloat>("CBitcrusher Symmetry " + std::to_string(i), "Classic Bitcrusher Symmetry " + std::to_string(i), NormalisableRange<float>(-100.f, 100.f, 1.f), 0.f, "Classic Bitcrusher " + std::to_string(i)));
        layout.add(std::make_unique<AudioParameterFloat>("CBitcrusher Bias " + std::to_string(i), "Classic Bitcrusher Bias " + std::to_string(i), NormalisableRange<float>(-0.9f, 0.9f, 0.01f), 0.f, "Classic Bitcrusher " + std::to_string(i)));
        layout.add(std::make_unique<AudioParameterFloat>("CBitcrusher Rate Redux " + std::to_string(i), "Classic Bitcrusher Rate Redux " + std::to_string(i), NormalisableRange<float>(100.f, 44100.f, 10.f, 0.25f), 44100.f, "Classic Bitcrusher " + std::to_string(i)));
        layout.add(std::make_unique<AudioParameterFloat>("CBitcrusher Bit Redux " + std::to_string(i), "Classic Bitcrusher Bit Redux " + std::to_string(i), NormalisableRange<float>(1.f, 16.f, 0.01f, 0.25f), 16.f, "Classic Bitcrusher " + std::to_string(i)));
        layout.add(std::make_unique<AudioParameterFloat>("CBitcrusher Dither " + std::to_string(i), "Classic Bitcrusher Dither " + std::to_string(i), NormalisableRange<float>(0.f, 100.f, 0.01f), 0.f, "Classic Bitcrusher " + std::to_string(i)));
        layout.add(std::make_unique<AudioParameterBool>("CBitcrusher Bypassed " + std::to_string(i), "Classic Bitcrusher Bypassed " + std::to_string(i), false, "Classic Bitcrusher " + std::to_string(i)));
    }
}

ClassicBitcrusherSettings ClassicBitcrusherModuleDSP::getSettings(juce::AudioProcessorValueTreeState& apvts, unsigned int parameterNumber)
{
    ClassicBitcrusherSettings settings;

    settings.drive = apvts.getRawParameterValue("CBitcrusher Drive " + std::to_string(parameterNumber))->load();
    settings.mix = apvts.getRawParameterValue("CBitcrusher Mix " + std::to_string(parameterNumber))->load();
    settings.symmetry = apvts.getRawParameterValue("CBitcrusher Symmetry " + std::to_string(parameterNumber))->load();
    settings.bias = apvts.getRawParameterValue("CBitcrusher Bias " + std::to_string(parameterNumber))->load();
    settings.rateRedux = apvts.getRawParameterValue("CBitcrusher Rate Redux " + std::to_string(parameterNumber))->load();
    settings.bitRedux = apvts.getRawParameterValue("CBitcrusher Bit Redux " + std::to_string(parameterNumber))->load();
    settings.dither = apvts.getRawParameterValue("CBitcrusher Dither " + std::to_string(parameterNumber))->load();
    settings.bypassed = apvts.getRawParameterValue("CBitcrusher Bypassed " + std::to_string(parameterNumber))->load() > 0.5f;

    return settings;
}

//==============================================================================

/* ClassicBitcrusherModule GUI */

//==============================================================================

ClassicBitcrusherModuleGUI::ClassicBitcrusherModuleGUI(PluginState& p, unsigned int parameterNumber)
    : GUIModule(), pluginState(p),
    driveSlider(*pluginState.apvts.getParameter(CLASSIC_BITCRUSHER_ID + "Drive " + std::to_string(parameterNumber)), "dB"),
    mixSlider(*pluginState.apvts.getParameter(CLASSIC_BITCRUSHER_ID + "Mix " + std::to_string(parameterNumber)), "%"),
    symmetrySlider(*pluginState.apvts.getParameter(CLASSIC_BITCRUSHER_ID + "Symmetry " + std::to_string(parameterNumber)), "%"),
    biasSlider(*pluginState.apvts.getParameter(CLASSIC_BITCRUSHER_ID + "Bias " + std::to_string(parameterNumber)), ""),
    bitcrusherDitherSlider(*pluginState.apvts.getParameter(CLASSIC_BITCRUSHER_ID + "Dither " + std::to_string(parameterNumber)), "%"),
    bitcrusherRateReduxSlider(*pluginState.apvts.getParameter(CLASSIC_BITCRUSHER_ID + "Rate Redux " + std::to_string(parameterNumber)), "Hz"),
    bitcrusherBitReduxSlider(*pluginState.apvts.getParameter(CLASSIC_BITCRUSHER_ID + "Bit Redux " + std::to_string(parameterNumber)), ""),
    driveSliderAttachment(pluginState.apvts, CLASSIC_BITCRUSHER_ID + "Drive " + std::to_string(parameterNumber), driveSlider),
    mixSliderAttachment(pluginState.apvts, CLASSIC_BITCRUSHER_ID + "Mix " + std::to_string(parameterNumber), mixSlider),
    symmetrySliderAttachment(pluginState.apvts, CLASSIC_BITCRUSHER_ID + "Symmetry " + std::to_string(parameterNumber), symmetrySlider),
    biasSliderAttachment(pluginState.apvts, CLASSIC_BITCRUSHER_ID + "Bias " + std::to_string(parameterNumber), biasSlider),
    bitcrusherDitherSliderAttachment(pluginState.apvts, CLASSIC_BITCRUSHER_ID + "Dither " + std::to_string(parameterNumber), bitcrusherDitherSlider),
    bitcrusherRateReduxSliderAttachment(pluginState.apvts, CLASSIC_BITCRUSHER_ID + "Rate Redux " + std::to_string(parameterNumber), bitcrusherRateReduxSlider),
    bitcrusherBitReduxSliderAttachment(pluginState.apvts, CLASSIC_BITCRUSHER_ID + "Bit Redux " + std::to_string(parameterNumber), bitcrusherBitReduxSlider),
    bypassButtonAttachment(pluginState.apvts, CLASSIC_BITCRUSHER_ID + "Bypassed " + std::to_string(parameterNumber), bypassButton)
{
    // title setup
    title.setText("Classic Bitcrusher", juce::dontSendNotification);
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
    bitcrusherDitherLabel.setText("Dither", juce::dontSendNotification);
    bitcrusherDitherLabel.setFont(ModuleLookAndFeel::getLabelsFont());
    bitcrusherRateReduxLabel.setText("Sampling Rate Redux", juce::dontSendNotification);
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
    bitcrusherDitherSlider.labels.add({ 0.f, "0%" });
    bitcrusherDitherSlider.labels.add({ 1.f, "100%" });
    bitcrusherRateReduxSlider.labels.add({ 0.f, "100Hz" });
    bitcrusherRateReduxSlider.labels.add({ 1.f, "44.1kHz" });
    bitcrusherBitReduxSlider.labels.add({ 0.f, "1" });
    bitcrusherBitReduxSlider.labels.add({ 1.f, "16" });

    bypassButton.setLookAndFeel(&lnf);

    auto safePtr = juce::Component::SafePointer<ClassicBitcrusherModuleGUI>(this);
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
    bitcrusherDitherSlider.setTooltip("Set the amount of white noise applied to the processed signal to add dithering or further noisy distortion");
    bitcrusherRateReduxSlider.setTooltip("Set the sampling rate for the reduction of signal resolution");
    bitcrusherBitReduxSlider.setTooltip("Set the bit depth for the reduction of signal resolution");

    for (auto* comp : getAllComps())
    {
        addAndMakeVisible(comp);
    }

    handleParamCompsEnablement(bypassButton.getToggleState());
}

ClassicBitcrusherModuleGUI::~ClassicBitcrusherModuleGUI()
{
    bypassButton.setLookAndFeel(nullptr);
}

std::vector<juce::Component*> ClassicBitcrusherModuleGUI::getAllComps()
{
    return {
        &title,
        &driveSlider,
        &mixSlider,
        &symmetrySlider,
        &biasSlider,
        &bitcrusherDitherSlider,
        &bitcrusherRateReduxSlider,
        &bitcrusherBitReduxSlider,
        // labels
        &driveLabel,
        &mixLabel,
        &symmetryLabel,
        &biasLabel,
        &bitcrusherDitherLabel,
        &bitcrusherRateReduxLabel,
        &bitcrusherBitReduxLabel,
        // bypass
        &bypassButton
    };
}

std::vector<juce::Component*> ClassicBitcrusherModuleGUI::getParamComps()
{
    return {
        &driveSlider,
        &mixSlider,
        &symmetrySlider,
        &biasSlider,
        &bitcrusherDitherSlider,
        &bitcrusherRateReduxSlider,
        &bitcrusherBitReduxSlider
    };
}

void ClassicBitcrusherModuleGUI::updateParameters(const juce::Array<juce::var>& values)
{
    auto value = values.begin();

    bypassButton.setToggleState(*(value++), juce::NotificationType::sendNotificationSync);
    driveSlider.setValue(*(value++), juce::NotificationType::sendNotificationSync);
    mixSlider.setValue(*(value++), juce::NotificationType::sendNotificationSync);
    symmetrySlider.setValue(*(value++), juce::NotificationType::sendNotificationSync);
    biasSlider.setValue(*(value++), juce::NotificationType::sendNotificationSync);
    bitcrusherDitherSlider.setValue(*(value++), juce::NotificationType::sendNotificationSync);
    bitcrusherRateReduxSlider.setValue(*(value++), juce::NotificationType::sendNotificationSync);
    bitcrusherBitReduxSlider.setValue(*(value++), juce::NotificationType::sendNotificationSync);
}

void ClassicBitcrusherModuleGUI::resetParameters(unsigned int parameterNumber)
{
    auto drive = pluginState.apvts.getParameter(CLASSIC_BITCRUSHER_ID + "Drive " + std::to_string(parameterNumber));
    auto mix = pluginState.apvts.getParameter(CLASSIC_BITCRUSHER_ID + "Mix " + std::to_string(parameterNumber));
    auto symmetry = pluginState.apvts.getParameter(CLASSIC_BITCRUSHER_ID + "Symmetry " + std::to_string(parameterNumber));
    auto bias = pluginState.apvts.getParameter(CLASSIC_BITCRUSHER_ID + "Bias " + std::to_string(parameterNumber));
    auto rateRedux = pluginState.apvts.getParameter(CLASSIC_BITCRUSHER_ID + "Rate Redux " + std::to_string(parameterNumber));
    auto bitRedux = pluginState.apvts.getParameter(CLASSIC_BITCRUSHER_ID + "Bit Redux " + std::to_string(parameterNumber));
    auto dither = pluginState.apvts.getParameter(CLASSIC_BITCRUSHER_ID + "Dither " + std::to_string(parameterNumber));
    auto bypassed = pluginState.apvts.getParameter(CLASSIC_BITCRUSHER_ID + "Bypassed " + std::to_string(parameterNumber));

    drive->setValueNotifyingHost(drive->getDefaultValue());
    mix->setValueNotifyingHost(mix->getDefaultValue());
    symmetry->setValueNotifyingHost(symmetry->getDefaultValue());
    bias->setValueNotifyingHost(bias->getDefaultValue());
    rateRedux->setValueNotifyingHost(rateRedux->getDefaultValue());
    bitRedux->setValueNotifyingHost(bitRedux->getDefaultValue());
    dither->setValueNotifyingHost(dither->getDefaultValue());
    bypassed->setValueNotifyingHost(bypassed->getDefaultValue());
}

juce::Array<juce::var> ClassicBitcrusherModuleGUI::getParamValues()
{
    juce::Array<juce::var> values;

    values.add(juce::var(bypassButton.getToggleState()));
    values.add(juce::var(driveSlider.getValue()));
    values.add(juce::var(mixSlider.getValue()));
    values.add(juce::var(symmetrySlider.getValue()));
    values.add(juce::var(biasSlider.getValue()));
    values.add(juce::var(bitcrusherDitherSlider.getValue()));
    values.add(juce::var(bitcrusherRateReduxSlider.getValue()));
    values.add(juce::var(bitcrusherBitReduxSlider.getValue()));

    return values;
}

void ClassicBitcrusherModuleGUI::resized()
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
    auto rateLabelArea = bottomLabelsArea.removeFromLeft(bottomLabelsArea.getWidth() * (1.f / 3.f));
    auto bitLabelArea = bottomLabelsArea.removeFromLeft(bottomLabelsArea.getWidth() * (1.f / 2.f));
    auto ditherLabelArea = bottomLabelsArea;

    // slider areas
    temp = topArea.removeFromLeft(topArea.getWidth() * (1.f / 2.f));
    auto driveArea = temp.removeFromLeft(temp.getWidth() * (1.f / 2.f));
    auto mixArea = temp;
    auto symmetryArea = topArea.removeFromLeft(topArea.getWidth() * (1.f / 2.f));
    auto biasArea = topArea;
    auto rateArea = bottomArea.removeFromLeft(bottomArea.getWidth() * (1.f / 3.f));
    auto bitArea = bottomArea.removeFromLeft(bottomArea.getWidth() * (1.f / 2.f));
    auto ditherArea = bottomArea;

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

    renderArea.setCentre(ditherArea.getCentre());
    renderArea.setY(ditherArea.getTopLeft().getY());
    bitcrusherDitherSlider.setBounds(renderArea);
    bitcrusherDitherLabel.setBounds(ditherLabelArea);
    bitcrusherDitherLabel.setJustificationType(juce::Justification::centred);
}