/*
  ==============================================================================

    BitcrusherModule.cpp

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


#include "BitcrusherModule.h"
#include "../PluginProcessor.h"

//==============================================================================

/* BitcrusherModule DSP */

//==============================================================================

BitcrusherModuleDSP::BitcrusherModuleDSP(juce::AudioProcessorValueTreeState& _apvts)
    : DSPModule(_apvts)
{
}

Array<float> BitcrusherModuleDSP::getWhiteNoise(int numSamples) {

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

void BitcrusherModuleDSP::setModuleType()
{
    moduleType = ModuleType::Bitcrusher;
}

void BitcrusherModuleDSP::updateDSPState(double sampleRate)
{
    auto settings = getSettings(apvts, getChainPosition());

    bypassed = settings.bypassed;

    auto mix = settings.mix * 0.01f;
    dryGain.setTargetValue(1.f - mix);
    wetGain.setTargetValue(mix);
    driveGain.setTargetValue(juce::Decibels::decibelsToGain(settings.drive));
    symmetry.setTargetValue(settings.symmetry * 0.01f);
    bias.setTargetValue(settings.bias * 0.01f);

    rateRedux.setTargetValue(settings.rateRedux);
    bitRedux.setTargetValue(settings.bitRedux);
    dither.setTargetValue(settings.dither * 0.01f);
}

void BitcrusherModuleDSP::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    wetBuffer.setSize(2, samplesPerBlock, false, true, true); // clears
    noiseBuffer.setSize(2, samplesPerBlock, false, true, true); // clears
    tempBuffer.setSize(2, samplesPerBlock, false, true, true); // clears
    updateDSPState(sampleRate);
}

void BitcrusherModuleDSP::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages, double sampleRate)
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
        // Applying gain to the noiseBuffer
        dither.applyGain(noiseBuffer, numSamples);
        // Multiply the noise by the signal ... so 0 signal -> 0 noise
        FloatVectorOperations::multiply(noiseBuffer.getWritePointer(0), wetBuffer.getWritePointer(0), numSamples);
        FloatVectorOperations::multiply(noiseBuffer.getWritePointer(1), wetBuffer.getWritePointer(1), numSamples);
        // Add noise to the incoming audio
        wetBuffer.addFrom(0, 0, noiseBuffer.getReadPointer(0), numSamples);
        wetBuffer.addFrom(1, 0, noiseBuffer.getReadPointer(1), numSamples);


        // TODO : frequency-domain bitcrushing (and a way to switch between time/frequency-domain)
        // parameters: time/freq-domain switch; FFTorder selector; rate/bitRedux performs in the same way in each type of bitcrushing.
        // see FFTAnalyzerComponent for FFT-related stuff 
        // https://forum.juce.com/t/issue-with-fft-plugin-inverse-transformation/16630/3
        // https://forum.juce.com/t/fft-amplitude/28574
        // tips:
        // - gestisco analisi FFT come nel FilterModule (possibilità di scelta all'utente tra diversi FFT-order, inizialmente fisso 1024)
        // - faccio algoritmo che in base al bufferSize e al FFT-order effettui un delay iniziale della riproduzione in quanto non c'è abbastanza informazione per 
        //   effettuare una corretta analisi FFT
        // - delay del segnale = aggiunta di 0.f in tutti i sample del buffer in uscita
        // - nel caso in cui il param mix > 0% devo ritardare la riproduzione anche del segnale dry !!
        // - minore FFT-order - bufferSize => minor delay del segnale
        // - outputBuffer[i+(2^fft-order / bufferSIze)] = modifiedBuffer[i] -> considerando outputBuffer e modifiedBuffer come array di buffer
        // - risorse utili: FFTAnalyzerComponent + https://docs.juce.com/master/tutorial_simple_fft.html


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

        applyAsymmetry(tempBuffer, wetBuffer, symmetry.getNextValue(), bias.getNextValue(), numSamples);

        // Mixing buffers
        dryGain.applyGain(buffer, numSamples);
        wetGain.applyGain(tempBuffer, numSamples);

        for (auto channel = 0; channel < 2; channel++)
            buffer.addFrom(channel, 0, tempBuffer, channel, 0, numSamples);
    }
}

void BitcrusherModuleDSP::addParameters(juce::AudioProcessorValueTreeState::ParameterLayout& layout)
{
    using namespace juce;

    for (int i = 1; i < 9; ++i) {
        layout.add(std::make_unique<AudioParameterFloat>("Bitcrusher Drive " + std::to_string(i), "Bitcrusher Drive " + std::to_string(i), NormalisableRange<float>(0.f, 40.f, 0.01f), 0.f, "Bitcrusher " + std::to_string(i)));
        layout.add(std::make_unique<AudioParameterFloat>("Bitcrusher Mix " + std::to_string(i), "Bitcrusher Mix " + std::to_string(i), NormalisableRange<float>(0.f, 100.f, 0.01f), 100.f, "Bitcrusher " + std::to_string(i)));
        layout.add(std::make_unique<AudioParameterFloat>("Bitcrusher Symmetry " + std::to_string(i), "Bitcrusher Symmetry " + std::to_string(i), NormalisableRange<float>(-100.f, 100.f, 1.f), 0.f, "Bitcrusher " + std::to_string(i)));
        layout.add(std::make_unique<AudioParameterFloat>("Bitcrusher Bias " + std::to_string(i), "Bitcrusher Bias " + std::to_string(i), NormalisableRange<float>(-90.f, 90.f, 1.f), 0.f, "Bitcrusher " + std::to_string(i)));
        layout.add(std::make_unique<AudioParameterFloat>("Bitcrusher Rate Redux " + std::to_string(i), "Bitcrusher Rate Redux " + std::to_string(i), NormalisableRange<float>(100.f, 44100.f, 10.f, 0.25f), 44100.f, "Bitcrusher " + std::to_string(i)));
        layout.add(std::make_unique<AudioParameterFloat>("Bitcrusher Bit Redux " + std::to_string(i), "Bitcrusher Bit Redux " + std::to_string(i), NormalisableRange<float>(1.f, 16.f, 0.01f, 0.25f), 16.f, "Bitcrusher " + std::to_string(i)));
        layout.add(std::make_unique<AudioParameterFloat>("Bitcrusher Dither " + std::to_string(i), "Bitcrusher Dither " + std::to_string(i), NormalisableRange<float>(0.f, 100.f, 0.01f), 0.f, "Bitcrusher " + std::to_string(i)));
        layout.add(std::make_unique<AudioParameterBool>("Bitcrusher Bypassed " + std::to_string(i), "Bitcrusher Bypassed " + std::to_string(i), false, "Bitcrusher " + std::to_string(i)));
    }
}

BitcrusherSettings BitcrusherModuleDSP::getSettings(juce::AudioProcessorValueTreeState& apvts, unsigned int chainPosition)
{
    BitcrusherSettings settings;

    settings.drive = apvts.getRawParameterValue("Bitcrusher Drive " + std::to_string(chainPosition))->load();
    settings.mix = apvts.getRawParameterValue("Bitcrusher Mix " + std::to_string(chainPosition))->load();
    settings.symmetry = apvts.getRawParameterValue("Bitcrusher Symmetry " + std::to_string(chainPosition))->load();
    settings.bias = apvts.getRawParameterValue("Bitcrusher Bias " + std::to_string(chainPosition))->load();
    settings.rateRedux = apvts.getRawParameterValue("Bitcrusher Rate Redux " + std::to_string(chainPosition))->load();
    settings.bitRedux = apvts.getRawParameterValue("Bitcrusher Bit Redux " + std::to_string(chainPosition))->load();
    settings.dither = apvts.getRawParameterValue("Bitcrusher Dither " + std::to_string(chainPosition))->load();
    settings.bypassed = apvts.getRawParameterValue("Bitcrusher Bypassed " + std::to_string(chainPosition))->load() > 0.5f;

    return settings;
}

//==============================================================================

/* BitcrusherModule GUI */

//==============================================================================

BitcrusherModuleGUI::BitcrusherModuleGUI(BiztortionAudioProcessor& p, unsigned int chainPosition)
    : GUIModule(), audioProcessor(p),
    driveSlider(*audioProcessor.apvts.getParameter("Bitcrusher Drive " + std::to_string(chainPosition)), "dB"),
    mixSlider(*audioProcessor.apvts.getParameter("Bitcrusher Mix " + std::to_string(chainPosition)), "%"),
    symmetrySlider(*audioProcessor.apvts.getParameter("Bitcrusher Symmetry " + std::to_string(chainPosition)), "%"),
    biasSlider(*audioProcessor.apvts.getParameter("Bitcrusher Bias " + std::to_string(chainPosition)), ""),
    bitcrusherDitherSlider(*audioProcessor.apvts.getParameter("Bitcrusher Dither " + std::to_string(chainPosition)), "%"),
    bitcrusherRateReduxSlider(*audioProcessor.apvts.getParameter("Bitcrusher Rate Redux " + std::to_string(chainPosition)), "Hz"),
    bitcrusherBitReduxSlider(*audioProcessor.apvts.getParameter("Bitcrusher Bit Redux " + std::to_string(chainPosition)), ""),
    driveSliderAttachment(audioProcessor.apvts, "Bitcrusher Drive " + std::to_string(chainPosition), driveSlider),
    mixSliderAttachment(audioProcessor.apvts, "Bitcrusher Mix " + std::to_string(chainPosition), mixSlider),
    symmetrySliderAttachment(audioProcessor.apvts, "Bitcrusher Symmetry " + std::to_string(chainPosition), symmetrySlider),
    biasSliderAttachment(audioProcessor.apvts, "Bitcrusher Bias " + std::to_string(chainPosition), biasSlider),
    bitcrusherDitherSliderAttachment(audioProcessor.apvts, "Bitcrusher Dither " + std::to_string(chainPosition), bitcrusherDitherSlider),
    bitcrusherRateReduxSliderAttachment(audioProcessor.apvts, "Bitcrusher Rate Redux " + std::to_string(chainPosition), bitcrusherRateReduxSlider),
    bitcrusherBitReduxSliderAttachment(audioProcessor.apvts, "Bitcrusher Bit Redux " + std::to_string(chainPosition), bitcrusherBitReduxSlider),
    bypassButtonAttachment(audioProcessor.apvts, "Bitcrusher Bypassed " + std::to_string(chainPosition), bypassButton)
{
    // title setup
    title.setText("Bitcrusher", juce::dontSendNotification);
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
    bitcrusherDitherLabel.setText("Dither", juce::dontSendNotification);
    bitcrusherDitherLabel.setFont(10);
    bitcrusherRateReduxLabel.setText("SampleRate Redux", juce::dontSendNotification);
    bitcrusherRateReduxLabel.setFont(10);
    bitcrusherBitReduxLabel.setText("Bit Depht Redux", juce::dontSendNotification);
    bitcrusherBitReduxLabel.setFont(10);

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

    auto safePtr = juce::Component::SafePointer<BitcrusherModuleGUI>(this);
    bypassButton.onClick = [safePtr]()
    {
        if (auto* comp = safePtr.getComponent())
        {
            auto bypassed = comp->bypassButton.getToggleState();

            comp->driveSlider.setEnabled(!bypassed);
            comp->mixSlider.setEnabled(!bypassed);
            comp->symmetrySlider.setEnabled(!bypassed);
            comp->biasSlider.setEnabled(!bypassed);
            comp->bitcrusherDitherSlider.setEnabled(!bypassed);
            comp->bitcrusherRateReduxSlider.setEnabled(!bypassed);
            comp->bitcrusherBitReduxSlider.setEnabled(!bypassed);
        }
    };

    for (auto* comp : getComps())
    {
        addAndMakeVisible(comp);
    }
}

BitcrusherModuleGUI::~BitcrusherModuleGUI()
{
    bypassButton.setLookAndFeel(nullptr);
}

void BitcrusherModuleGUI::paint(juce::Graphics& g)
{
    drawContainer(g);
}

void BitcrusherModuleGUI::resized()
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

    auto topArea = bitcrusherArea.removeFromTop(bitcrusherArea.getHeight() * (1.f / 2.f));
    auto bottomArea = bitcrusherArea;

    auto topLabelsArea = topArea.removeFromTop(12);
    auto bottomLabelsArea = bottomArea.removeFromTop(12);

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

    title.setBounds(titleAndBypassArea);
    title.setJustificationType(juce::Justification::centred);

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

    bitcrusherRateReduxSlider.setBounds(rateArea);
    bitcrusherRateReduxLabel.setBounds(rateLabelArea);
    bitcrusherRateReduxLabel.setJustificationType(juce::Justification::centred);

    bitcrusherBitReduxSlider.setBounds(bitArea);
    bitcrusherBitReduxLabel.setBounds(bitLabelArea);
    bitcrusherBitReduxLabel.setJustificationType(juce::Justification::centred);

    bitcrusherDitherSlider.setBounds(ditherArea);
    bitcrusherDitherLabel.setBounds(ditherLabelArea);
    bitcrusherDitherLabel.setJustificationType(juce::Justification::centred);
}

std::vector<juce::Component*> BitcrusherModuleGUI::getComps()
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
