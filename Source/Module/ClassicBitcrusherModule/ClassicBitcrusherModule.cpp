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

#include "../EqualizerModule/EqualizerModule.h"

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

    auto symmetryValue = juce::jmap(settings.symmetry, -100.f, 100.f, 0.f, 2.f);

    auto mix = settings.mix * 0.01f;
    dryGain.setTargetValue(1.f - mix);
    wetGain.setTargetValue(mix);
    driveGain.setTargetValue(juce::Decibels::decibelsToGain(settings.drive));
    fxDistribution.setTargetValue(settings.fxDistribution * 0.01f);
    symmetry.setTargetValue(symmetryValue);
    bias.setTargetValue(settings.bias);

    rateRedux.setTargetValue(settings.rateRedux);
    bitRedux.setTargetValue(settings.bitRedux);
    dither.setTargetValue(settings.dither * 0.01f);

    DCoffsetRemoveEnabled = settings.DCoffsetRemove;
}

void ClassicBitcrusherModuleDSP::prepareToPlay(double sampleRate, int samplesPerBlock)
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

        // Temp Buffer feeding for applying fxDistribution
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

void ClassicBitcrusherModuleDSP::addParameters(juce::AudioProcessorValueTreeState::ParameterLayout& layout)
{
    using namespace juce;

    for (int i = 1; i < 9; ++i) {
        layout.add(std::make_unique<AudioParameterFloat>(CLASSIC_BITCRUSHER_ID + "Drive " + std::to_string(i), "Classic Bitcrusher Drive " + std::to_string(i), NormalisableRange<float>(0.f, 40.f, 0.01f), 0.f, "Classic Bitcrusher " + std::to_string(i)));
        layout.add(std::make_unique<AudioParameterFloat>(CLASSIC_BITCRUSHER_ID + "Mix " + std::to_string(i), "Classic Bitcrusher Mix " + std::to_string(i), NormalisableRange<float>(0.f, 100.f, 0.01f), 100.f, "Classic Bitcrusher " + std::to_string(i)));
        layout.add(std::make_unique<AudioParameterFloat>(CLASSIC_BITCRUSHER_ID + "Fx Distribution " + std::to_string(i), "Classic Bitcrusher Fx Distribution " + std::to_string(i), NormalisableRange<float>(-100.f, 100.f, 1.f), 0.f, "Classic Bitcrusher " + std::to_string(i)));
        layout.add(std::make_unique<AudioParameterFloat>(CLASSIC_BITCRUSHER_ID + "Symmetry " + std::to_string(i), "Classic Bitcrusher Symmetry " + std::to_string(i), NormalisableRange<float>(-100.f, 100.f, 1.f), 0.f, "Classic Bitcrusher " + std::to_string(i)));
        layout.add(std::make_unique<AudioParameterFloat>(CLASSIC_BITCRUSHER_ID + "Bias " + std::to_string(i), "Classic Bitcrusher Bias " + std::to_string(i), NormalisableRange<float>(-0.9f, 0.9f, 0.01f), 0.f, "Classic Bitcrusher " + std::to_string(i)));
        layout.add(std::make_unique<AudioParameterFloat>(CLASSIC_BITCRUSHER_ID + "Rate Redux " + std::to_string(i), "Classic Bitcrusher Rate Redux " + std::to_string(i), NormalisableRange<float>(100.f, 44100.f, 10.f, 0.25f), 44100.f, "Classic Bitcrusher " + std::to_string(i)));
        layout.add(std::make_unique<AudioParameterFloat>(CLASSIC_BITCRUSHER_ID + "Bit Redux " + std::to_string(i), "Classic Bitcrusher Bit Redux " + std::to_string(i), NormalisableRange<float>(1.f, 16.f, 0.01f, 0.25f), 16.f, "Classic Bitcrusher " + std::to_string(i)));
        layout.add(std::make_unique<AudioParameterFloat>(CLASSIC_BITCRUSHER_ID + "Dither " + std::to_string(i), "Classic Bitcrusher Dither " + std::to_string(i), NormalisableRange<float>(0.f, 100.f, 0.01f), 0.f, "Classic Bitcrusher " + std::to_string(i)));
        layout.add(std::make_unique<AudioParameterBool>(CLASSIC_BITCRUSHER_ID + "DCoffset Enabled " + std::to_string(i), "Classic Bitcrusher DCoffset Enabled " + std::to_string(i), true, "Classic Bitcrusher " + std::to_string(i)));
        layout.add(std::make_unique<AudioParameterBool>(CLASSIC_BITCRUSHER_ID + "Bypassed " + std::to_string(i), "Classic Bitcrusher Bypassed " + std::to_string(i), false, "Classic Bitcrusher " + std::to_string(i)));
    }
}

ClassicBitcrusherSettings ClassicBitcrusherModuleDSP::getSettings(juce::AudioProcessorValueTreeState& apvts, unsigned int parameterNumber)
{
    ClassicBitcrusherSettings settings;

    settings.drive = apvts.getRawParameterValue(CLASSIC_BITCRUSHER_ID + "Drive " + std::to_string(parameterNumber))->load();
    settings.mix = apvts.getRawParameterValue(CLASSIC_BITCRUSHER_ID + "Mix " + std::to_string(parameterNumber))->load();
    settings.fxDistribution = apvts.getRawParameterValue(CLASSIC_BITCRUSHER_ID + "Fx Distribution " + std::to_string(parameterNumber))->load();
    settings.symmetry = apvts.getRawParameterValue(CLASSIC_BITCRUSHER_ID + "Symmetry " + std::to_string(parameterNumber))->load();
    settings.bias = apvts.getRawParameterValue(CLASSIC_BITCRUSHER_ID + "Bias " + std::to_string(parameterNumber))->load();
    settings.rateRedux = apvts.getRawParameterValue(CLASSIC_BITCRUSHER_ID + "Rate Redux " + std::to_string(parameterNumber))->load();
    settings.bitRedux = apvts.getRawParameterValue(CLASSIC_BITCRUSHER_ID + "Bit Redux " + std::to_string(parameterNumber))->load();
    settings.dither = apvts.getRawParameterValue(CLASSIC_BITCRUSHER_ID + "Dither " + std::to_string(parameterNumber))->load();
    settings.DCoffsetRemove = apvts.getRawParameterValue(CLASSIC_BITCRUSHER_ID + "DCoffset Enabled " + std::to_string(parameterNumber))->load() > 0.5f;
    settings.bypassed = apvts.getRawParameterValue(CLASSIC_BITCRUSHER_ID + "Bypassed " + std::to_string(parameterNumber))->load() > 0.5f;

    return settings;
}

//==============================================================================

/* ClassicBitcrusherModule GUI */

//==============================================================================

ClassicBitcrusherModuleGUI::ClassicBitcrusherModuleGUI(PluginState& p, unsigned int parameterNumber)
    : GUIModule(), pluginState(p), parameterNumber(parameterNumber),
    driveSlider(*pluginState.apvts.getParameter(CLASSIC_BITCRUSHER_ID + "Drive " + std::to_string(parameterNumber)), "dB"),
    mixSlider(*pluginState.apvts.getParameter(CLASSIC_BITCRUSHER_ID + "Mix " + std::to_string(parameterNumber)), "%"),
    fxDistributionSlider(*pluginState.apvts.getParameter(CLASSIC_BITCRUSHER_ID + "Fx Distribution " + std::to_string(parameterNumber)), "%"),
    symmetrySlider(*pluginState.apvts.getParameter(CLASSIC_BITCRUSHER_ID + "Symmetry " + std::to_string(parameterNumber)), "%"),
    biasSlider(*pluginState.apvts.getParameter(CLASSIC_BITCRUSHER_ID + "Bias " + std::to_string(parameterNumber)), ""),
    bitcrusherDitherSlider(*pluginState.apvts.getParameter(CLASSIC_BITCRUSHER_ID + "Dither " + std::to_string(parameterNumber)), "%"),
    bitcrusherRateReduxSlider(*pluginState.apvts.getParameter(CLASSIC_BITCRUSHER_ID + "Rate Redux " + std::to_string(parameterNumber)), "Hz"),
    bitcrusherBitReduxSlider(*pluginState.apvts.getParameter(CLASSIC_BITCRUSHER_ID + "Bit Redux " + std::to_string(parameterNumber)), ""),
    driveSliderAttachment(pluginState.apvts, CLASSIC_BITCRUSHER_ID + "Drive " + std::to_string(parameterNumber), driveSlider),
    mixSliderAttachment(pluginState.apvts, CLASSIC_BITCRUSHER_ID + "Mix " + std::to_string(parameterNumber), mixSlider),
    fxDistributionSliderAttachment(pluginState.apvts, CLASSIC_BITCRUSHER_ID + "Fx Distribution " + std::to_string(parameterNumber), fxDistributionSlider),
    symmetrySliderAttachment(pluginState.apvts, CLASSIC_BITCRUSHER_ID + "Symmetry " + std::to_string(parameterNumber), symmetrySlider),
    biasSliderAttachment(pluginState.apvts, CLASSIC_BITCRUSHER_ID + "Bias " + std::to_string(parameterNumber), biasSlider),
    bitcrusherDitherSliderAttachment(pluginState.apvts, CLASSIC_BITCRUSHER_ID + "Dither " + std::to_string(parameterNumber), bitcrusherDitherSlider),
    bitcrusherRateReduxSliderAttachment(pluginState.apvts, CLASSIC_BITCRUSHER_ID + "Rate Redux " + std::to_string(parameterNumber), bitcrusherRateReduxSlider),
    bitcrusherBitReduxSliderAttachment(pluginState.apvts, CLASSIC_BITCRUSHER_ID + "Bit Redux " + std::to_string(parameterNumber), bitcrusherBitReduxSlider),
    DCoffsetEnabledButtonAttachment(pluginState.apvts, CLASSIC_BITCRUSHER_ID + "DCoffset Enabled " + std::to_string(parameterNumber), DCoffsetEnabledButton),
    bypassButtonAttachment(pluginState.apvts, CLASSIC_BITCRUSHER_ID + "Bypassed " + std::to_string(parameterNumber), bypassButton)
{
    // title setup
    title.setText("Classic Bitcrusher", juce::dontSendNotification);
    title.setFont(ModuleLookAndFeel::getTitlesFont());

    moduleColor = moduleType_colors.at(ModuleType::ClassicBitcrusher);
    setModuleHelp("Classic Bitcrusher",
        "Reduces sample rate and bit depth for familiar digital crunch. Use Dither to add noise and soften or exaggerate the quantisation texture.");

    // --- graphs ---
    // Visual model of the bitcrusher: the signal is held for several samples (rate redux) and
    // snapped to a coarse grid of levels (bit redux), with dither noise on top.
    moduleGraph = std::make_unique<WaveShapeVisualiser>(moduleColor,
        [this](std::vector<float>& samples) {
            const auto settings = ClassicBitcrusherModuleDSP::getSettings(pluginState.apvts, this->parameterNumber);

            // map the 100..44100 Hz control onto a sample-and-hold length that is visible in a
            // graph of a few hundred points
            const auto rateProportion = juce::jlimit(0.0f, 1.0f,
                (settings.rateRedux - 100.0f) / (44100.0f - 100.0f));
            const auto holdLength = juce::jmax(1,
                juce::roundToInt(juce::jmap(1.0f - std::pow(rateProportion, 0.35f), 1.0f, 44.0f)));

            const auto levels = std::pow(2.0f, juce::jlimit(1.0f, 16.0f, settings.bitRedux));
            const auto step = 2.0f / juce::jmax(2.0f, levels);
            const auto ditherAmount = juce::jlimit(0.0f, 1.0f, settings.dither * 0.01f);

            juce::Random random(1234);
            auto held = samples.empty() ? 0.0f : samples[0];

            for (size_t index = 0; index < samples.size(); ++index)
            {
                if (index % (size_t) holdLength == 0)
                    held = samples[index];

                // quantise to the reduced bit depth
                auto value = std::round(held / step) * step;

                // The module multiplies white noise by the signal and adds it, so dither is
                // signal dependent and stays visible at any bit depth. Adding it before the
                // quantiser instead would vanish at 16 bits, where the step is tiny.
                if (ditherAmount > 0.0f)
                    value += (random.nextFloat() - 0.5f) * 0.5f * ditherAmount * std::abs(held);

                samples[index] = juce::jlimit(-1.0f, 1.0f, value);
            }
        }, WaveShapeVisualiser::Style::steppedWaveform);
    moduleGraph->setTooltip(
        "What this module does to the waveform: the signal is held for several samples "
        "(sampling rate redux), snapped to a coarse grid of levels (bit depth redux) and "
        "sprinkled with signal dependent noise (dither).");
    moduleGraph->setCaption("crush");

    sharedParamsGraph = std::make_unique<WaveShapeVisualiser>(moduleColor,
        [this](std::vector<float>& samples) {
            const auto settings = ClassicBitcrusherModuleDSP::getSettings(pluginState.apvts, this->parameterNumber);
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
    bitcrusherDitherLabel.setText("Dither", juce::dontSendNotification);
    bitcrusherDitherLabel.setFont(ModuleLookAndFeel::getLabelsFont());
    bitcrusherRateReduxLabel.setText("Sampling Rate Redux", juce::dontSendNotification);
    bitcrusherRateReduxLabel.setFont(ModuleLookAndFeel::getLabelsFont());
    bitcrusherBitReduxLabel.setText("Bit Depth Redux", juce::dontSendNotification);
    bitcrusherBitReduxLabel.setFont(ModuleLookAndFeel::getLabelsFont());
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
    bitcrusherDitherSlider.labels.add({ 0.f, "0%" });
    bitcrusherDitherSlider.labels.add({ 1.f, "100%" });
    bitcrusherRateReduxSlider.labels.add({ 0.f, "100Hz" });
    bitcrusherRateReduxSlider.labels.add({ 1.f, "44.1kHz" });
    bitcrusherBitReduxSlider.labels.add({ 0.f, "1" });
    bitcrusherBitReduxSlider.labels.add({ 1.f, "16" });

    // bypass button
    lnf.color = moduleType_colors.at(ModuleType::ClassicBitcrusher);
    bypassButton.setLookAndFeel(&lnf);

    // dcFilter
    DCoffsetEnabledButton.setLookAndFeel(&dcOffsetLnf);

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
    fxDistributionSlider.setTooltip("Apply the signal processing to the positive or negative area of the waveform");
    symmetrySlider.setTooltip("Apply symmetry to the waveform moving it from the center");
    biasSlider.setTooltip("Set the value which determines the bias between the positive or negative area of the waveform");
    bitcrusherDitherSlider.setTooltip("Set the amount of white noise applied to the processed signal to add dithering or further noisy distortion");
    bitcrusherRateReduxSlider.setTooltip("Set the sampling rate for the reduction of signal resolution");
    bitcrusherBitReduxSlider.setTooltip("Set the bit depth for the reduction of signal resolution");
    DCoffsetEnabledButtonLabel.setTooltip("Enable a low-frequency highpass filter to remove any DC offset in the signal");
    DCoffsetEnabledButton.setTooltip("Enable a low-frequency highpass filter to remove any DC offset in the signal");

    for (auto* comp : getAllComps())
    {
        addAndMakeVisible(comp);
    }

    handleParamCompsEnablement(bypassButton.getToggleState());
}

ClassicBitcrusherModuleGUI::~ClassicBitcrusherModuleGUI()
{
    bypassButton.setLookAndFeel(nullptr);
    DCoffsetEnabledButton.setLookAndFeel(nullptr);
}

std::vector<juce::Component*> ClassicBitcrusherModuleGUI::getAllComps()
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
        &bitcrusherDitherSlider,
        &bitcrusherRateReduxSlider,
        &bitcrusherBitReduxSlider,
        &DCoffsetEnabledButton,
        // labels
        &driveLabel,
        &mixLabel,
        &fxDistributionLabel,
        &symmetryLabel,
        &biasLabel,
        &bitcrusherDitherLabel,
        &bitcrusherRateReduxLabel,
        &bitcrusherBitReduxLabel,
        &DCoffsetEnabledButtonLabel,
        // bypass
        &bypassButton
    };
}

std::vector<juce::Component*> ClassicBitcrusherModuleGUI::getParamComps()
{
    return {
        &driveSlider,
        &mixSlider,
        &fxDistributionSlider,
        &symmetrySlider,
        &biasSlider,
        &bitcrusherDitherSlider,
        &bitcrusherRateReduxSlider,
        &bitcrusherBitReduxSlider,
        &DCoffsetEnabledButton
    };
}

void ClassicBitcrusherModuleGUI::updateParameters(const juce::Array<juce::var>& values)
{
    auto value = values.begin();

    bypassButton.setToggleState(*(value++), juce::NotificationType::sendNotificationSync);
    driveSlider.setValue(*(value++), juce::NotificationType::sendNotificationSync);
    mixSlider.setValue(*(value++), juce::NotificationType::sendNotificationSync);
    fxDistributionSlider.setValue(*(value++), juce::NotificationType::sendNotificationSync);
    symmetrySlider.setValue(*(value++), juce::NotificationType::sendNotificationSync);
    biasSlider.setValue(*(value++), juce::NotificationType::sendNotificationSync);
    bitcrusherDitherSlider.setValue(*(value++), juce::NotificationType::sendNotificationSync);
    bitcrusherRateReduxSlider.setValue(*(value++), juce::NotificationType::sendNotificationSync);
    bitcrusherBitReduxSlider.setValue(*(value++), juce::NotificationType::sendNotificationSync);
    DCoffsetEnabledButton.setToggleState(*(value++), juce::NotificationType::sendNotificationSync);
}

void ClassicBitcrusherModuleGUI::resetParameters(unsigned int parameterNumber)
{
    auto drive = pluginState.apvts.getParameter(CLASSIC_BITCRUSHER_ID + "Drive " + std::to_string(parameterNumber));
    auto mix = pluginState.apvts.getParameter(CLASSIC_BITCRUSHER_ID + "Mix " + std::to_string(parameterNumber));
    auto fxDistribution = pluginState.apvts.getParameter(CLASSIC_BITCRUSHER_ID + "Fx Distribution " + std::to_string(parameterNumber));
    auto symmetry = pluginState.apvts.getParameter(CLASSIC_BITCRUSHER_ID + "Symmetry " + std::to_string(parameterNumber));
    auto bias = pluginState.apvts.getParameter(CLASSIC_BITCRUSHER_ID + "Bias " + std::to_string(parameterNumber));
    auto rateRedux = pluginState.apvts.getParameter(CLASSIC_BITCRUSHER_ID + "Rate Redux " + std::to_string(parameterNumber));
    auto bitRedux = pluginState.apvts.getParameter(CLASSIC_BITCRUSHER_ID + "Bit Redux " + std::to_string(parameterNumber));
    auto dither = pluginState.apvts.getParameter(CLASSIC_BITCRUSHER_ID + "Dither " + std::to_string(parameterNumber));
    auto dcOffset = pluginState.apvts.getParameter(CLASSIC_BITCRUSHER_ID + "DCoffset Enabled " + std::to_string(parameterNumber));
    auto bypassed = pluginState.apvts.getParameter(CLASSIC_BITCRUSHER_ID + "Bypassed " + std::to_string(parameterNumber));

    drive->setValueNotifyingHost(drive->getDefaultValue());
    mix->setValueNotifyingHost(mix->getDefaultValue());
    fxDistribution->setValueNotifyingHost(fxDistribution->getDefaultValue());
    symmetry->setValueNotifyingHost(symmetry->getDefaultValue());
    bias->setValueNotifyingHost(bias->getDefaultValue());
    rateRedux->setValueNotifyingHost(rateRedux->getDefaultValue());
    bitRedux->setValueNotifyingHost(bitRedux->getDefaultValue());
    dither->setValueNotifyingHost(dither->getDefaultValue());
    dcOffset->setValueNotifyingHost(dcOffset->getDefaultValue());
    bypassed->setValueNotifyingHost(bypassed->getDefaultValue());
}

juce::Array<juce::var> ClassicBitcrusherModuleGUI::getParamValues()
{
    juce::Array<juce::var> values;

    values.add(juce::var(bypassButton.getToggleState()));
    values.add(juce::var(driveSlider.getValue()));
    values.add(juce::var(mixSlider.getValue()));
    values.add(juce::var(fxDistributionSlider.getValue()));
    values.add(juce::var(symmetrySlider.getValue()));
    values.add(juce::var(biasSlider.getValue()));
    values.add(juce::var(bitcrusherDitherSlider.getValue()));
    values.add(juce::var(bitcrusherRateReduxSlider.getValue()));
    values.add(juce::var(bitcrusherBitReduxSlider.getValue()));
    values.add(juce::var(DCoffsetEnabledButton.getToggleState()));

    return values;
}

void ClassicBitcrusherModuleGUI::resized()
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
        const auto size = juce::jmin(cell.getWidth() - 6, cell.getHeight() - 6);
        slider.setBounds(juce::Rectangle<int>(size, size).withCentre(cell.getCentre()));
    };

    auto moduleKnobs = moduleSide.reduced(4, 8);
    const auto knobWidth = moduleKnobs.getWidth() / 3;
    placeKnob(moduleKnobs.removeFromLeft(knobWidth), bitcrusherRateReduxSlider, bitcrusherRateReduxLabel);
    placeKnob(moduleKnobs.removeFromLeft(knobWidth), bitcrusherBitReduxSlider, bitcrusherBitReduxLabel);
    placeKnob(moduleKnobs, bitcrusherDitherSlider, bitcrusherDitherLabel);

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
