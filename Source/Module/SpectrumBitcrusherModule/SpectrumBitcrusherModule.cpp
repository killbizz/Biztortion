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

namespace
{
constexpr float spectrumResolutionBypassValue = 1024.0f;

juce::NormalisableRange<float> makeSpectrumResolutionRange()
{
    return {
        16.0f,
        spectrumResolutionBypassValue,
        [](float start, float end, float proportion) {
            return start * std::pow(end / start, proportion);
        },
        [](float start, float end, float value) {
            return std::log(value / start) / std::log(end / start);
        },
        [](float start, float end, float value) {
            return juce::jlimit(start, end, std::round(value));
        }
    };
}
}

//==============================================================================

/* SpectrumBitcrusherModule DSP */

//==============================================================================

SpectrumBitcrusherModuleDSP::SpectrumBitcrusherModuleDSP(juce::AudioProcessorValueTreeState& _apvts)
    : DSPModule(_apvts)
{
    spectrumProcessingLambda = [this](juce::AudioSampleBuffer& fftBuffer) {

        const auto numBins = fftBuffer.getNumSamples() / 4;
        const auto binsToProcess = numBins - 1;
        const auto targetBandCount = juce::jlimit(1, binsToProcess,
            juce::roundToInt(rateRedux.getNextValue()));
        const auto bitDepth = juce::jlimit(1, 16,
            juce::roundToInt(magnitudeBits.getNextValue()));
        const auto tilt = spectralTilt.getNextValue();
        const auto robotMix = juce::jlimit(0.0f, 1.0f, robotisation.getNextValue());
        const auto pitch = juce::jmax(20.0f, robotPitch.getNextValue());
        const auto binWidth = currentSampleRate / spectrumProcessor.getFFTSize();

        if (static_cast<int>(magnitudeScratch.size()) < numBins)
            return;

        for (int chan = 0; chan < fftBuffer.getNumChannels(); chan++) {

            float* floatData = fftBuffer.getWritePointer(chan);
            auto* data = reinterpret_cast<std::complex<float>*>(floatData);

            for (int band = 0; band < targetBandCount; ++band) {
                const auto startBin = 1 + band * binsToProcess / targetBandCount;
                const auto endBin = 1 + (band + 1) * binsToProcess / targetBandCount;
                double groupEnergy = 0.0;

                for (int bin = startBin; bin < endBin; ++bin) {
                    const auto magnitude = std::abs(data[bin]);
                    groupEnergy += magnitude * magnitude;
                }

                const auto groupMagnitude = static_cast<float>(std::sqrt(
                    groupEnergy / juce::jmax(1, endBin - startBin)));

                for (int bin = startBin; bin < endBin; ++bin)
                    magnitudeScratch[static_cast<size_t>(bin)] = groupMagnitude;
            }

            float peakMagnitude = 0.0f;
            for (int bin = 1; bin < numBins; ++bin) {
                const auto normalisedFrequency = static_cast<float>(bin) / binsToProcess;
                const auto tiltGain = juce::Decibels::decibelsToGain(
                    tilt * (2.0f * normalisedFrequency - 1.0f));
                auto& magnitude = magnitudeScratch[static_cast<size_t>(bin)];
                magnitude *= tiltGain;
                peakMagnitude = juce::jmax(peakMagnitude, magnitude);
            }

            if (bitDepth < 16 && peakMagnitude > 0.0f) {
                const auto quantisationLevels = static_cast<float>((1u << bitDepth) - 1u);

                for (int bin = 1; bin < numBins; ++bin) {
                    auto& magnitude = magnitudeScratch[static_cast<size_t>(bin)];
                    magnitude = std::round(magnitude / peakMagnitude * quantisationLevels)
                        / quantisationLevels * peakMagnitude;
                }
            }

            for (int bin = 1; bin < numBins; ++bin) {
                auto processedMagnitude = magnitudeScratch[static_cast<size_t>(bin)];
                const auto originalPhase = std::arg(data[bin]);
                auto processedPhase = originalPhase;

                if (robotMix > 0.0f) {
                    const auto frequency = static_cast<double>(bin) * binWidth;
                    const auto harmonic = juce::jmax(1, juce::roundToInt(frequency / pitch));
                    const auto harmonicFrequency = static_cast<double>(harmonic) * pitch;
                    const auto distanceFromHarmonic = frequency - harmonicFrequency;
                    const auto combWidth = juce::jmax(binWidth * 0.75, static_cast<double>(pitch) * 0.12);
                    const auto combGain = 0.05 + 0.95 * std::exp(-0.5
                        * distanceFromHarmonic * distanceFromHarmonic
                        / (combWidth * combWidth));
                    processedMagnitude *= static_cast<float>(1.0 - robotMix + robotMix * combGain);

                    const auto targetPhase = static_cast<float>(std::remainder(
                        static_cast<double>(harmonic) * robotPhase,
                        juce::MathConstants<double>::twoPi));
                    const auto originalUnit = std::polar(1.0f, originalPhase);
                    const auto robotUnit = std::polar(1.0f, targetPhase);
                    const auto mixedUnit = originalUnit * (1.0f - robotMix) + robotUnit * robotMix;
                    processedPhase = std::abs(mixedUnit) > 0.0001f
                        ? std::arg(mixedUnit)
                        : targetPhase;
                }

                data[bin] = std::polar(processedMagnitude, processedPhase);

            }
        }

        robotPhase = std::remainder(robotPhase
            + juce::MathConstants<double>::twoPi * pitch
                * spectrumProcessor.getHopSize() / currentSampleRate,
            juce::MathConstants<double>::twoPi);
    };
}

void SpectrumBitcrusherModuleDSP::updateDSPState(double sampleRate)
{
    auto settings = getSettings(apvts, parameterNumber);

    bypassed = settings.bypassed;
    spectrumProcessingEnabled = settings.rateRedux < spectrumResolutionBypassValue
        || settings.magnitudeBits < 16.0f
        || std::abs(settings.spectralTilt) > 0.001f
        || settings.robotisation > 0.001f;
    
    auto symmetryValue = juce::jmap(settings.symmetry, -100.f, 100.f, 0.f, 2.f);

    auto mix = settings.mix * 0.01f;
    dryGain.setTargetValue(1.f - mix);
    wetGain.setTargetValue(mix);
    driveGain.setTargetValue(juce::Decibels::decibelsToGain(settings.drive));
    fxDistribution.setTargetValue(settings.fxDistribution * 0.01f);
    bias.setTargetValue(settings.bias);
    symmetry.setTargetValue(symmetryValue);

    rateRedux.setTargetValue(settings.rateRedux);
    magnitudeBits.setTargetValue(settings.magnitudeBits);
    spectralTilt.setTargetValue(settings.spectralTilt);
    robotisation.setTargetValue(settings.robotisation * 0.01f);
    robotPitch.setTargetValue(settings.robotPitch);

    DCoffsetRemoveEnabled = settings.DCoffsetRemove;
}

void SpectrumBitcrusherModuleDSP::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    currentSampleRate = sampleRate;
    robotPhase = 0.0;
    magnitudeScratch.resize(static_cast<size_t>(spectrumProcessor.getFFTSize() / 2));

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

        if (spectrumProcessingEnabled)
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
        layout.add(std::make_unique<AudioParameterFloat>(SPECTRUM_BITCRUSHER_ID + "Bins Redux " + std::to_string(i), "Spectrum Bitcrusher Bins Redux " + std::to_string(i), makeSpectrumResolutionRange(), spectrumResolutionBypassValue, "Spectrum Bitcrusher " + std::to_string(i)));
        layout.add(std::make_unique<AudioParameterInt>(SPECTRUM_BITCRUSHER_ID + "Magnitude Bits " + std::to_string(i), "Spectrum Bitcrusher Magnitude Bits " + std::to_string(i), 1, 16, 16, "Spectrum Bitcrusher " + std::to_string(i)));
        layout.add(std::make_unique<AudioParameterFloat>(SPECTRUM_BITCRUSHER_ID + "Spectral Tilt " + std::to_string(i), "Spectrum Bitcrusher Spectral Tilt " + std::to_string(i), NormalisableRange<float>(-18.f, 18.f, 0.1f), 0.f, "Spectrum Bitcrusher " + std::to_string(i)));
        layout.add(std::make_unique<AudioParameterFloat>(SPECTRUM_BITCRUSHER_ID + "Robotisation " + std::to_string(i), "Spectrum Bitcrusher Robotisation " + std::to_string(i), NormalisableRange<float>(0.f, 100.f, 1.f), 0.f, "Spectrum Bitcrusher " + std::to_string(i)));
        layout.add(std::make_unique<AudioParameterFloat>(SPECTRUM_BITCRUSHER_ID + "Robot Pitch " + std::to_string(i), "Spectrum Bitcrusher Robot Pitch " + std::to_string(i), NormalisableRange<float>(30.f, 1000.f, 0.1f, 0.35f), 110.f, "Spectrum Bitcrusher " + std::to_string(i)));
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
    settings.magnitudeBits = apvts.getRawParameterValue(SPECTRUM_BITCRUSHER_ID + "Magnitude Bits " + std::to_string(parameterNumber))->load();
    settings.spectralTilt = apvts.getRawParameterValue(SPECTRUM_BITCRUSHER_ID + "Spectral Tilt " + std::to_string(parameterNumber))->load();
    settings.robotisation = apvts.getRawParameterValue(SPECTRUM_BITCRUSHER_ID + "Robotisation " + std::to_string(parameterNumber))->load();
    settings.robotPitch = apvts.getRawParameterValue(SPECTRUM_BITCRUSHER_ID + "Robot Pitch " + std::to_string(parameterNumber))->load();
    settings.DCoffsetRemove = apvts.getRawParameterValue(SPECTRUM_BITCRUSHER_ID + "DCoffset Enabled " + std::to_string(parameterNumber))->load() > 0.5f;
    settings.bypassed = apvts.getRawParameterValue(SPECTRUM_BITCRUSHER_ID + "Bypassed " + std::to_string(parameterNumber))->load() > 0.5f;

    return settings;
}

//==============================================================================

/* SpectrumBitcrusherModule GUI */

//==============================================================================

SpectrumBitcrusherModuleGUI::SpectrumBitcrusherModuleGUI(PluginState& p, unsigned int parameterNumber)
    : GUIModule(), pluginState(p), parameterNumber(parameterNumber),
    driveSlider(*pluginState.apvts.getParameter(SPECTRUM_BITCRUSHER_ID + "Drive " + std::to_string(parameterNumber)), "dB"),
    mixSlider(*pluginState.apvts.getParameter(SPECTRUM_BITCRUSHER_ID + "Mix " + std::to_string(parameterNumber)), "%"),
    fxDistributionSlider(*pluginState.apvts.getParameter(SPECTRUM_BITCRUSHER_ID + "Fx Distribution " + std::to_string(parameterNumber)), "%"),
    biasSlider(*pluginState.apvts.getParameter(SPECTRUM_BITCRUSHER_ID + "Bias " + std::to_string(parameterNumber)), ""),
    symmetrySlider(*pluginState.apvts.getParameter(SPECTRUM_BITCRUSHER_ID + "Symmetry " + std::to_string(parameterNumber)), "%"),
    binsReduxSlider(*pluginState.apvts.getParameter(SPECTRUM_BITCRUSHER_ID + "Bins Redux " + std::to_string(parameterNumber)), ""),
    magnitudeBitsSlider(*pluginState.apvts.getParameter(SPECTRUM_BITCRUSHER_ID + "Magnitude Bits " + std::to_string(parameterNumber)), ""),
    spectralTiltSlider(*pluginState.apvts.getParameter(SPECTRUM_BITCRUSHER_ID + "Spectral Tilt " + std::to_string(parameterNumber)), "dB"),
    robotisationSlider(*pluginState.apvts.getParameter(SPECTRUM_BITCRUSHER_ID + "Robotisation " + std::to_string(parameterNumber)), "%"),
    robotPitchSlider(*pluginState.apvts.getParameter(SPECTRUM_BITCRUSHER_ID + "Robot Pitch " + std::to_string(parameterNumber)), "Hz"),
    driveSliderAttachment(pluginState.apvts, SPECTRUM_BITCRUSHER_ID + "Drive " + std::to_string(parameterNumber), driveSlider),
    mixSliderAttachment(pluginState.apvts, SPECTRUM_BITCRUSHER_ID + "Mix " + std::to_string(parameterNumber), mixSlider),
    fxDistributionSliderAttachment(pluginState.apvts, SPECTRUM_BITCRUSHER_ID + "Fx Distribution " + std::to_string(parameterNumber), fxDistributionSlider),
    biasSliderAttachment(pluginState.apvts, SPECTRUM_BITCRUSHER_ID + "Bias " + std::to_string(parameterNumber), biasSlider),
    symmetrySliderAttachment(pluginState.apvts, SPECTRUM_BITCRUSHER_ID + "Symmetry " + std::to_string(parameterNumber), symmetrySlider),
    binsReduxSliderAttachment(pluginState.apvts, SPECTRUM_BITCRUSHER_ID + "Bins Redux " + std::to_string(parameterNumber), binsReduxSlider),
    magnitudeBitsSliderAttachment(pluginState.apvts, SPECTRUM_BITCRUSHER_ID + "Magnitude Bits " + std::to_string(parameterNumber), magnitudeBitsSlider),
    spectralTiltSliderAttachment(pluginState.apvts, SPECTRUM_BITCRUSHER_ID + "Spectral Tilt " + std::to_string(parameterNumber), spectralTiltSlider),
    robotisationSliderAttachment(pluginState.apvts, SPECTRUM_BITCRUSHER_ID + "Robotisation " + std::to_string(parameterNumber), robotisationSlider),
    robotPitchSliderAttachment(pluginState.apvts, SPECTRUM_BITCRUSHER_ID + "Robot Pitch " + std::to_string(parameterNumber), robotPitchSlider),
    DCoffsetEnabledButtonAttachment(pluginState.apvts, SPECTRUM_BITCRUSHER_ID + "DCoffset Enabled " + std::to_string(parameterNumber), DCoffsetEnabledButton),
    bypassButtonAttachment(pluginState.apvts, SPECTRUM_BITCRUSHER_ID + "Bypassed " + std::to_string(parameterNumber), bypassButton)
{
    // title setup
    title.setText("Spectrum Bitcrusher", juce::dontSendNotification);
    title.setFont(ModuleLookAndFeel::getTitlesFont());

    moduleColor = moduleType_colors.at(ModuleType::SpectrumBitcrusher);
    setModuleHelp("Spectrum Bitcrusher",
        "Crushes frequency resolution and FFT magnitudes instead of time-domain samples. Robotisation locks the spectrum to a controllable harmonic pitch.");

    // --- graphs ---
    // This module works on the spectrum, so its graph shows magnitude bars rather than a
    // waveform: bins redux merges neighbouring bars, magnitude bits quantises their height,
    // spectral tilt slopes the whole spectrum and robotisation pulls it towards a harmonic comb.
    moduleGraph = std::make_unique<WaveShapeVisualiser>(moduleColor,
        [this](std::vector<float>& magnitudes) {
            const auto settings = SpectrumBitcrusherModuleDSP::getSettings(pluginState.apvts, this->parameterNumber);
            const auto barCount = magnitudes.size();

            if (barCount == 0)
                return;

            // bins redux: 1024 = untouched, 16 = very coarse
            const auto resolutionProportion = juce::jlimit(0.0f, 1.0f,
                std::log(juce::jmax(16.0f, settings.rateRedux) / 16.0f) / std::log(1024.0f / 16.0f));
            const auto groupSize = juce::jmax<size_t>(1,
                (size_t) juce::roundToInt(juce::jmap(1.0f - resolutionProportion, 1.0f, 8.0f)));

            // spectral tilt in dB across the whole range
            for (size_t index = 0; index < barCount; ++index)
            {
                const auto proportion = (float) index / (float) juce::jmax<size_t>(1, barCount - 1);
                magnitudes[index] *= juce::Decibels::decibelsToGain(settings.spectralTilt * proportion);
            }

            // robotisation pulls energy onto a harmonic comb
            const auto robot = juce::jlimit(0.0f, 1.0f, settings.robotisation * 0.01f);
            if (robot > 0.0f)
            {
                const auto spacing = juce::jmax<size_t>(2,
                    (size_t) juce::roundToInt(juce::jmap(settings.robotPitch, 30.0f, 1000.0f, 2.0f, 7.0f)));

                for (size_t index = 0; index < barCount; ++index)
                {
                    const auto onHarmonic = (index % spacing) == 0;
                    const auto target = onHarmonic ? 1.0f : 0.08f;
                    magnitudes[index] = magnitudes[index] * (1.0f - robot) + target * robot;
                }
            }

            // bins redux: average each group so neighbouring bars merge
            for (size_t start = 0; start < barCount; start += groupSize)
            {
                const auto end = juce::jmin(barCount, start + groupSize);
                auto sum = 0.0f;

                for (auto index = start; index < end; ++index)
                    sum += magnitudes[index];

                const auto average = sum / (float) (end - start);

                for (auto index = start; index < end; ++index)
                    magnitudes[index] = average;
            }

            // magnitude bits: quantise the bar heights
            const auto levels = std::pow(2.0f, juce::jlimit(1.0f, 16.0f, settings.magnitudeBits));
            const auto step = 1.0f / juce::jmax(2.0f, levels);

            for (auto& magnitude : magnitudes)
                magnitude = juce::jlimit(0.0f, 1.0f, std::round(magnitude / step) * step);
        }, WaveShapeVisualiser::Style::spectrumBars);
    moduleGraph->setTooltip(
        "What this module does to the spectrum: neighbouring bars merge (freq resolution), "
        "their heights are quantised (magnitude bits), the whole spectrum is sloped "
        "(spectral tilt) and pulled onto a harmonic comb (robotisation).");
    moduleGraph->setCaption("spectrum");

    sharedParamsGraph = std::make_unique<WaveShapeVisualiser>(moduleColor,
        [this](std::vector<float>& samples) {
            const auto settings = SpectrumBitcrusherModuleDSP::getSettings(pluginState.apvts, this->parameterNumber);
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
    biasLabel.setText("Fx Bias", juce::dontSendNotification);
    biasLabel.setFont(ModuleLookAndFeel::getLabelsFont());
    symmetryLabel.setText("Symmetry", juce::dontSendNotification);
    symmetryLabel.setFont(ModuleLookAndFeel::getLabelsFont());
    binsReduxLabel.setText("Freq Resolution", juce::dontSendNotification);
    binsReduxLabel.setFont(ModuleLookAndFeel::getLabelsFont());
    magnitudeBitsLabel.setText("Magnitude Bits", juce::dontSendNotification);
    magnitudeBitsLabel.setFont(ModuleLookAndFeel::getLabelsFont());
    spectralTiltLabel.setText("Spectral Tilt", juce::dontSendNotification);
    spectralTiltLabel.setFont(ModuleLookAndFeel::getLabelsFont());
    robotisationLabel.setText("Robotisation", juce::dontSendNotification);
    robotisationLabel.setFont(ModuleLookAndFeel::getLabelsFont());
    robotPitchLabel.setText("Robot Pitch", juce::dontSendNotification);
    robotPitchLabel.setFont(ModuleLookAndFeel::getLabelsFont());
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
    binsReduxSlider.labels.add({ 1.f, "Off" });
    binsReduxSlider.displayStringForValue = [](double value) {
        return value >= spectrumResolutionBypassValue
            ? juce::String("Off")
            : juce::String(juce::roundToInt(value));
    };
    magnitudeBitsSlider.labels.add({ 0.f, "1" });
    magnitudeBitsSlider.labels.add({ 1.f, "16" });
    spectralTiltSlider.labels.add({ 0.f, "-18dB" });
    spectralTiltSlider.labels.add({ 1.f, "+18dB" });
    robotisationSlider.labels.add({ 0.f, "0%" });
    robotisationSlider.labels.add({ 1.f, "100%" });
    robotPitchSlider.labels.add({ 0.f, "30Hz" });
    robotPitchSlider.labels.add({ 1.f, "1kHz" });

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
    magnitudeBitsSlider.setTooltip("Quantize FFT magnitudes to create spectral bit crushing and gating");
    spectralTiltSlider.setTooltip("Tilt the crushed spectrum toward low or high frequencies");
    robotisationSlider.setTooltip("Blend toward a pitched harmonic comb with phase-locked robotisation");
    robotPitchSlider.setTooltip("Set the fundamental pitch of the robotized sound");
    DCoffsetEnabledButtonLabel.setTooltip("Enable a low-frequency highpass filter to remove any DC offset in the signal");
    DCoffsetEnabledButton.setTooltip("Enable a low-frequency highpass filter to remove any DC offset in the signal");

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
        moduleGraph.get(),
        sharedParamsGraph.get(),
        &driveSlider,
        &mixSlider,
        &fxDistributionSlider,
        &biasSlider,
        &symmetrySlider,
        &binsReduxSlider,
        &magnitudeBitsSlider,
        &spectralTiltSlider,
        &robotisationSlider,
        &robotPitchSlider,
        &DCoffsetEnabledButton,
        // labels
        &driveLabel,
        &mixLabel,
        &fxDistributionLabel,
        &biasLabel,
        &symmetryLabel,
        &binsReduxLabel,
        &magnitudeBitsLabel,
        &spectralTiltLabel,
        &robotisationLabel,
        &robotPitchLabel,
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
        &magnitudeBitsSlider,
        &spectralTiltSlider,
        &robotisationSlider,
        &robotPitchSlider,
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
    magnitudeBitsSlider.setValue(*(value++), juce::NotificationType::sendNotificationSync);
    spectralTiltSlider.setValue(*(value++), juce::NotificationType::sendNotificationSync);
    robotPitchSlider.setValue(*(value++), juce::NotificationType::sendNotificationSync);
}

void SpectrumBitcrusherModuleGUI::resetParameters(unsigned int parameterNumber)
{
    auto drive = pluginState.apvts.getParameter(SPECTRUM_BITCRUSHER_ID + "Drive " + std::to_string(parameterNumber));
    auto mix = pluginState.apvts.getParameter(SPECTRUM_BITCRUSHER_ID + "Mix " + std::to_string(parameterNumber));
    auto fxDistribution = pluginState.apvts.getParameter(SPECTRUM_BITCRUSHER_ID + "Fx Distribution " + std::to_string(parameterNumber));
    auto bias = pluginState.apvts.getParameter(SPECTRUM_BITCRUSHER_ID + "Bias " + std::to_string(parameterNumber));
    auto symmetry = pluginState.apvts.getParameter(SPECTRUM_BITCRUSHER_ID + "Symmetry " + std::to_string(parameterNumber));
    auto binsRedux = pluginState.apvts.getParameter(SPECTRUM_BITCRUSHER_ID + "Bins Redux " + std::to_string(parameterNumber));
    auto magnitudeBits = pluginState.apvts.getParameter(SPECTRUM_BITCRUSHER_ID + "Magnitude Bits " + std::to_string(parameterNumber));
    auto spectralTilt = pluginState.apvts.getParameter(SPECTRUM_BITCRUSHER_ID + "Spectral Tilt " + std::to_string(parameterNumber));
    auto robotisation = pluginState.apvts.getParameter(SPECTRUM_BITCRUSHER_ID + "Robotisation " + std::to_string(parameterNumber));
    auto robotPitch = pluginState.apvts.getParameter(SPECTRUM_BITCRUSHER_ID + "Robot Pitch " + std::to_string(parameterNumber));
    auto dcOffset = pluginState.apvts.getParameter(SPECTRUM_BITCRUSHER_ID + "DCoffset Enabled " + std::to_string(parameterNumber));
    auto bypassed = pluginState.apvts.getParameter(SPECTRUM_BITCRUSHER_ID + "Bypassed " + std::to_string(parameterNumber));

    drive->setValueNotifyingHost(drive->getDefaultValue());
    mix->setValueNotifyingHost(mix->getDefaultValue());
    fxDistribution->setValueNotifyingHost(fxDistribution->getDefaultValue());
    bias->setValueNotifyingHost(bias->getDefaultValue());
    symmetry->setValueNotifyingHost(symmetry->getDefaultValue());
    binsRedux->setValueNotifyingHost(binsRedux->getDefaultValue());
    magnitudeBits->setValueNotifyingHost(magnitudeBits->getDefaultValue());
    spectralTilt->setValueNotifyingHost(spectralTilt->getDefaultValue());
    robotisation->setValueNotifyingHost(robotisation->getDefaultValue());
    robotPitch->setValueNotifyingHost(robotPitch->getDefaultValue());
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
    values.add(juce::var(magnitudeBitsSlider.getValue()));
    values.add(juce::var(spectralTiltSlider.getValue()));
    values.add(juce::var(robotPitchSlider.getValue()));

    return values;
}

void SpectrumBitcrusherModuleGUI::resized()
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
    auto moduleSide = area.removeFromLeft(juce::roundToInt(area.getWidth() * 0.46f));
    area.removeFromLeft(10);
    auto sharedSide = area;

    // --- module side ---
    moduleGraph->setBounds(moduleSide.removeFromTop(juce::roundToInt(moduleSide.getHeight() * 0.40f)));
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

    // five module parameters: three on the first row, two on the second
    auto moduleKnobs = moduleSide.reduced(4, 6);
    auto firstRow = moduleKnobs.removeFromTop(moduleKnobs.getHeight() / 2);
    auto secondRow = moduleKnobs;

    const auto firstRowCell = firstRow.getWidth() / 3;
    placeKnob(firstRow.removeFromLeft(firstRowCell), binsReduxSlider, binsReduxLabel);
    placeKnob(firstRow.removeFromLeft(firstRowCell), magnitudeBitsSlider, magnitudeBitsLabel);
    placeKnob(firstRow, spectralTiltSlider, spectralTiltLabel);

    placeKnob(secondRow.removeFromLeft(secondRow.getWidth() / 2), robotisationSlider, robotisationLabel);
    placeKnob(secondRow, robotPitchSlider, robotPitchLabel);

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
