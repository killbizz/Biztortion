/*
  ==============================================================================

    BitcrusherModule.cpp
    Created: 22 Sep 2021 10:28:48am
    Author:  gabri

  ==============================================================================
*/

/*
  ==============================================================================

    CREDITS for the original Bitcrusher Algorithm
    Author: Aaron Leese
    Source: https://youtu.be/1PLn8IAKEb4

  ==============================================================================
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

    rateRedux.setTargetValue(settings.rateRedux);
    bitRedux.setTargetValue(settings.bitRedux);
    dither.setTargetValue(settings.dither * 0.01f);

    auto mix = settings.mix * 0.01f;
    dryGain.setTargetValue(1.f - mix);
    wetGain.setTargetValue(mix);
}

void BitcrusherModuleDSP::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    wetBuffer.setSize(2, samplesPerBlock, false, true, true); // clears
    noiseBuffer.setSize(2, samplesPerBlock, false, true, true); // clears
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
        }

        // PROCESSING
        
        // Wet Buffer feeding
        for (auto channel = 0; channel < 2; channel++)
            wetBuffer.copyFrom(channel, 0, buffer, channel, 0, numSamples);

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

        // Mixing buffers
        dryGain.applyGain(buffer, numSamples);
        wetGain.applyGain(wetBuffer, numSamples);

        for (auto channel = 0; channel < 2; channel++)
            buffer.addFrom(channel, 0, wetBuffer, channel, 0, numSamples);
    }
}

void BitcrusherModuleDSP::addParameters(juce::AudioProcessorValueTreeState::ParameterLayout& layout)
{
    using namespace juce;

    for (int i = 1; i < 9; ++i) {
        layout.add(std::make_unique<AudioParameterFloat>("Bitcrusher Mix " + std::to_string(i), "Bitcrusher Mix " + std::to_string(i), NormalisableRange<float>(0.f, 100.f, 0.01f), 100.f, "Bitcrusher " + std::to_string(i)));
        layout.add(std::make_unique<AudioParameterFloat>("Bitcrusher Rate Redux " + std::to_string(i), "Bitcrusher Rate Redux " + std::to_string(i), NormalisableRange<float>(100.f, 44100.f, 10.f, 0.25f), 44100.f, "Bitcrusher " + std::to_string(i)));
        layout.add(std::make_unique<AudioParameterFloat>("Bitcrusher Bit Redux " + std::to_string(i), "Bitcrusher Bit Redux " + std::to_string(i), NormalisableRange<float>(1.f, 16.f, 0.01f, 0.25f), 16.f, "Bitcrusher " + std::to_string(i)));
        layout.add(std::make_unique<AudioParameterFloat>("Bitcrusher Dither " + std::to_string(i), "Bitcrusher Dither " + std::to_string(i), NormalisableRange<float>(0.f, 100.f, 0.01f), 0.f, "Bitcrusher " + std::to_string(i)));
        // bypass button
        layout.add(std::make_unique<AudioParameterBool>("Bitcrusher Bypassed " + std::to_string(i), "Bitcrusher Bypassed " + std::to_string(i), false, "Bitcrusher " + std::to_string(i)));
    }
}

BitcrusherSettings BitcrusherModuleDSP::getSettings(juce::AudioProcessorValueTreeState& apvts, unsigned int chainPosition)
{
    BitcrusherSettings settings;

    settings.mix = apvts.getRawParameterValue("Bitcrusher Mix " + std::to_string(chainPosition))->load();
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
    bitcrusherMixSlider(*audioProcessor.apvts.getParameter("Bitcrusher Mix " + std::to_string(chainPosition)), "%"),
    bitcrusherDitherSlider(*audioProcessor.apvts.getParameter("Bitcrusher Dither " + std::to_string(chainPosition)), "%"),
    bitcrusherRateReduxSlider(*audioProcessor.apvts.getParameter("Bitcrusher Rate Redux " + std::to_string(chainPosition)), "Hz"),
    bitcrusherBitReduxSlider(*audioProcessor.apvts.getParameter("Bitcrusher Bit Redux " + std::to_string(chainPosition)), ""),
    bitcrusherMixSliderAttachment(audioProcessor.apvts, "Bitcrusher Mix " + std::to_string(chainPosition), bitcrusherMixSlider),
    bitcrusherDitherSliderAttachment(audioProcessor.apvts, "Bitcrusher Dither " + std::to_string(chainPosition), bitcrusherDitherSlider),
    bitcrusherRateReduxSliderAttachment(audioProcessor.apvts, "Bitcrusher Rate Redux " + std::to_string(chainPosition), bitcrusherRateReduxSlider),
    bitcrusherBitReduxSliderAttachment(audioProcessor.apvts, "Bitcrusher Bit Redux " + std::to_string(chainPosition), bitcrusherBitReduxSlider),
    bypassButtonAttachment(audioProcessor.apvts, "Bitcrusher Bypassed " + std::to_string(chainPosition), bypassButton)
{
    // title setup
    title.setText("Bitcrusher", juce::dontSendNotification);
    title.setFont(24);

    // labels
    bitcrusherMixLabel.setText("Mix", juce::dontSendNotification);
    bitcrusherMixLabel.setFont(10);
    bitcrusherDitherLabel.setText("Dither", juce::dontSendNotification);
    bitcrusherDitherLabel.setFont(10);
    bitcrusherRateReduxLabel.setText("SampleRate Redux", juce::dontSendNotification);
    bitcrusherRateReduxLabel.setFont(10);
    bitcrusherBitReduxLabel.setText("Bit Depht Redux", juce::dontSendNotification);
    bitcrusherBitReduxLabel.setFont(10);

    bitcrusherMixSlider.labels.add({ 0.f, "0%" });
    bitcrusherMixSlider.labels.add({ 1.f, "100%" });
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

            comp->bitcrusherMixSlider.setEnabled(!bypassed);
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

    auto bitcrusherMixDitherControlsArea = bitcrusherArea.removeFromTop(bitcrusherArea.getHeight() * (1.f / 2.f));
    auto MixDitherControlsLabelsArea = bitcrusherMixDitherControlsArea.removeFromTop(12);
    auto mixLabelArea = MixDitherControlsLabelsArea.removeFromLeft(MixDitherControlsLabelsArea.getWidth() * (1.f / 2.f));
    auto ditherLabelArea = MixDitherControlsLabelsArea;

    auto bitcrusherRateBitControlsArea = bitcrusherArea;
    auto RateBitControlsLabelsArea = bitcrusherRateBitControlsArea.removeFromTop(12);
    auto rateLabelArea = RateBitControlsLabelsArea.removeFromLeft(RateBitControlsLabelsArea.getWidth() * (1.f / 2.f));
    auto bitLabelArea = RateBitControlsLabelsArea;

    title.setBounds(titleAndBypassArea);
    title.setJustificationType(juce::Justification::centred);

    bitcrusherMixSlider.setBounds(bitcrusherMixDitherControlsArea.removeFromLeft(bitcrusherMixDitherControlsArea.getWidth() * (1.f / 2.f)));
    bitcrusherMixLabel.setBounds(mixLabelArea);
    bitcrusherMixLabel.setJustificationType(juce::Justification::centred);

    bitcrusherDitherSlider.setBounds(bitcrusherMixDitherControlsArea);
    bitcrusherDitherLabel.setBounds(ditherLabelArea);
    bitcrusherDitherLabel.setJustificationType(juce::Justification::centred);

    bitcrusherRateReduxSlider.setBounds(bitcrusherRateBitControlsArea.removeFromLeft(bitcrusherRateBitControlsArea.getWidth() * (1.f / 2.f)));
    bitcrusherRateReduxLabel.setBounds(rateLabelArea);
    bitcrusherRateReduxLabel.setJustificationType(juce::Justification::centred);

    bitcrusherBitReduxSlider.setBounds(bitcrusherRateBitControlsArea);
    bitcrusherBitReduxLabel.setBounds(bitLabelArea);
    bitcrusherBitReduxLabel.setJustificationType(juce::Justification::centred);
}

std::vector<juce::Component*> BitcrusherModuleGUI::getComps()
{
    return {
        &title,
        &bitcrusherMixSlider,
        &bitcrusherDitherSlider,
        &bitcrusherRateReduxSlider,
        &bitcrusherBitReduxSlider,
        // labels
        &bitcrusherMixLabel,
        &bitcrusherDitherLabel,
        &bitcrusherRateReduxLabel,
        &bitcrusherBitReduxLabel,
        // bypass
        &bypassButton
    };
}
