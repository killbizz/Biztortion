/*
  ==============================================================================

    SlewLimiterModule.cpp
    Created: 8 Oct 2021 3:54:07pm
    Author:  gabri

  ==============================================================================
*/

/*
  ==============================================================================

    CREDITS for the original Slew Limiter Algorithm
    Author: Ivan Cohen
    Source: https://www.youtube.com/watch?v=oIChUOV_0w4&t=1787s&ab_channel=JUCE

  ==============================================================================
*/

#include "SlewLimiterModule.h"

#include "../PluginProcessor.h"

//==============================================================================

/* SlewLimiterModule DSP */

//==============================================================================

SlewLimiterModuleDSP::SlewLimiterModuleDSP(juce::AudioProcessorValueTreeState& _apvts)
    : DSPModule(_apvts)
{
}

void SlewLimiterModuleDSP::setModuleType()
{
    moduleType = ModuleType::SlewLimiter;
}

void SlewLimiterModuleDSP::updateDSPState(double sampleRate)
{
    auto settings = getSettings(apvts, getChainPosition());

    bypassed = settings.bypassed;
    DCoffsetRemoveEnabled = settings.DCoffsetRemove;

    symmetry.setTargetValue(settings.symmetry * 0.01f);
    bias.setTargetValue(settings.bias * 0.01f);
    rise.setTargetValue(settings.rise * 0.01f);
    fall.setTargetValue(settings.fall * 0.01f);
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
        }

        // Wet Buffer feeding
        for (auto channel = 0; channel < 2; channel++)
            wetBuffer.copyFrom(channel, 0, buffer, channel, 0, numSamples);

        // internal variables
        auto Ts = 1.f / sampleRate;
        float slewRise = slewMax * Ts * std::pow(slewMin / slewMax, rise.getNextValue());
        float slewFall = slewMax * Ts * std::pow(slewMin / slewMax, fall.getNextValue());

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

        applyAsymmetry(buffer, wetBuffer, symmetry.getNextValue(), bias.getNextValue(), numSamples);
        
        if (DCoffsetRemoveEnabled) {
            juce::dsp::AudioBlock<float> block(buffer);
            auto leftBlock = block.getSingleChannelBlock(0);
            auto rightBlock = block.getSingleChannelBlock(1);
            juce::dsp::ProcessContextReplacing<float> leftContext(leftBlock);
            juce::dsp::ProcessContextReplacing<float> rightContext(rightBlock);
            leftDCoffsetRemoveHPF.process(leftContext);
            rightDCoffsetRemoveHPF.process(rightContext);
        }

        // Mixing buffers
        /*buffer.clear();
        for (auto channel = 0; channel < 2; channel++)
            buffer.addFrom(channel, 0, wetBuffer, channel, 0, numSamples);*/
    }
}

void SlewLimiterModuleDSP::addParameters(juce::AudioProcessorValueTreeState::ParameterLayout& layout)
{
    using namespace juce;

    for (int i = 1; i < 9; ++i) {
        layout.add(std::make_unique<AudioParameterFloat>("SlewLimiter Rise " + std::to_string(i), "SlewLimiter Rise " + std::to_string(i), NormalisableRange<float>(0.f, 100.f, 0.01f), 0.f, "SlewLimiter " + std::to_string(i)));
        layout.add(std::make_unique<AudioParameterFloat>("SlewLimiter Fall " + std::to_string(i), "SlewLimiter Fall " + std::to_string(i), NormalisableRange<float>(0.f, 100.f, 0.01f), 0.f, "SlewLimiter " + std::to_string(i)));
        layout.add(std::make_unique<AudioParameterFloat>("SlewLimiter Symmetry " + std::to_string(i), "SlewLimiter Symmetry " + std::to_string(i), NormalisableRange<float>(-100.f, 100.f, 1.f), 0.f, "SlewLimiter " + std::to_string(i)));
        layout.add(std::make_unique<AudioParameterFloat>("SlewLimiter Bias " + std::to_string(i), "SlewLimiter Bias " + std::to_string(i), NormalisableRange<float>(-90.f, 90.f, 1.f), 0.f, "SlewLimiter " + std::to_string(i)));
        layout.add(std::make_unique<AudioParameterBool>("SlewLimiter DCoffset Enabled " + std::to_string(i), "SlewLimiter DCoffset Enabled " + std::to_string(i), false, "SlewLimiter " + std::to_string(i)));
        layout.add(std::make_unique<AudioParameterBool>("SlewLimiter Bypassed " + std::to_string(i), "SlewLimiter Bypassed " + std::to_string(i), false, "SlewLimiter " + std::to_string(i)));
    }
}

SlewLimiterSettings SlewLimiterModuleDSP::getSettings(juce::AudioProcessorValueTreeState& apvts, unsigned int chainPosition)
{
    SlewLimiterSettings settings;

    settings.rise = apvts.getRawParameterValue("SlewLimiter Rise " + std::to_string(chainPosition))->load();
    settings.fall = apvts.getRawParameterValue("SlewLimiter Fall " + std::to_string(chainPosition))->load();
    settings.symmetry = apvts.getRawParameterValue("SlewLimiter Symmetry " + std::to_string(chainPosition))->load();
    settings.bias = apvts.getRawParameterValue("SlewLimiter Bias " + std::to_string(chainPosition))->load();
    settings.DCoffsetRemove = apvts.getRawParameterValue("SlewLimiter DCoffset Enabled " + std::to_string(chainPosition))->load() > 0.5f;
    settings.bypassed = apvts.getRawParameterValue("SlewLimiter Bypassed " + std::to_string(chainPosition))->load() > 0.5f;

    return settings;
}

//==============================================================================

/* SlewLimiterModule GUI */

//==============================================================================

SlewLimiterModuleGUI::SlewLimiterModuleGUI(BiztortionAudioProcessor& p, unsigned int chainPosition)
    : GUIModule(), audioProcessor(p),
    slewLimiterRiseSlider(*audioProcessor.apvts.getParameter("SlewLimiter Rise " + std::to_string(chainPosition)), "%"),
    slewLimiterFallSlider(*audioProcessor.apvts.getParameter("SlewLimiter Fall " + std::to_string(chainPosition)), "%"),
    slewLimiterSymmetrySlider(*audioProcessor.apvts.getParameter("SlewLimiter Symmetry " + std::to_string(chainPosition)), "%"),
    slewLimiterBiasSlider(*audioProcessor.apvts.getParameter("SlewLimiter Bias " + std::to_string(chainPosition)), ""),
    slewLimiterRiseSliderAttachment(audioProcessor.apvts, "SlewLimiter Rise " + std::to_string(chainPosition), slewLimiterRiseSlider),
    slewLimiterFallSliderAttachment(audioProcessor.apvts, "SlewLimiter Fall " + std::to_string(chainPosition), slewLimiterFallSlider),
    slewLimiterSymmetrySliderAttachment(audioProcessor.apvts, "SlewLimiter Symmetry " + std::to_string(chainPosition), slewLimiterSymmetrySlider),
    slewLimiterBiasSliderAttachment(audioProcessor.apvts, "SlewLimiter Bias " + std::to_string(chainPosition), slewLimiterBiasSlider),
    DCoffsetEnabledButtonAttachment(audioProcessor.apvts, "SlewLimiter DCoffset Enabled " + std::to_string(chainPosition), DCoffsetEnabledButton),
    bypassButtonAttachment(audioProcessor.apvts, "SlewLimiter Bypassed " + std::to_string(chainPosition), bypassButton)
{
    // title setup
    title.setText("Slew Limiter", juce::dontSendNotification);
    title.setFont(24);

    // labels
    slewLimiterRiseLabel.setText("Rise", juce::dontSendNotification);
    slewLimiterRiseLabel.setFont(10);
    slewLimiterFallLabel.setText("Fall", juce::dontSendNotification);
    slewLimiterFallLabel.setFont(10);
    slewLimiterSymmetryLabel.setText("Symmetry", juce::dontSendNotification);
    slewLimiterSymmetryLabel.setFont(10);
    slewLimiterBiasLabel.setText("Bias", juce::dontSendNotification);
    slewLimiterBiasLabel.setFont(10);
    DCoffsetEnabledButtonLabel.setText("DC Filter", juce::dontSendNotification);
    DCoffsetEnabledButtonLabel.setFont(10);

    slewLimiterRiseSlider.labels.add({ 0.f, "0%" });
    slewLimiterRiseSlider.labels.add({ 1.f, "100%" });
    slewLimiterFallSlider.labels.add({ 0.f, "0%" });
    slewLimiterFallSlider.labels.add({ 1.f, "100%" });
    slewLimiterSymmetrySlider.labels.add({ 0.f, "-100%" });
    slewLimiterSymmetrySlider.labels.add({ 1.f, "+100%" });
    slewLimiterBiasSlider.labels.add({ 0.f, "-0.9" });
    slewLimiterBiasSlider.labels.add({ 1.f, "+0.9" });

    bypassButton.setLookAndFeel(&lnf);

    auto safePtr = juce::Component::SafePointer<SlewLimiterModuleGUI>(this);
    bypassButton.onClick = [safePtr]()
    {
        if (auto* comp = safePtr.getComponent())
        {
            auto bypassed = comp->bypassButton.getToggleState();

            comp->slewLimiterRiseSlider.setEnabled(!bypassed);
            comp->slewLimiterFallSlider.setEnabled(!bypassed);
            comp->slewLimiterSymmetrySlider.setEnabled(!bypassed);
            comp->slewLimiterBiasSlider.setEnabled(!bypassed);
            comp->DCoffsetEnabledButton.setEnabled(!bypassed);
        }
    };

    for (auto* comp : getComps())
    {
        addAndMakeVisible(comp);
    }
}

SlewLimiterModuleGUI::~SlewLimiterModuleGUI()
{
    bypassButton.setLookAndFeel(nullptr);
}

void SlewLimiterModuleGUI::paint(juce::Graphics& g)
{
    drawContainer(g);
}

void SlewLimiterModuleGUI::resized()
{
    auto slewLimiterArea = getContentRenderArea();

    // bypass
    auto temp = slewLimiterArea;
    auto bypassButtonArea = temp.removeFromTop(25);

    bypassButtonArea.setWidth(35.f);
    bypassButtonArea.setX(145.f);
    bypassButtonArea.setY(20.f);

    bypassButton.setBounds(bypassButtonArea);

    auto titleAndBypassArea = slewLimiterArea.removeFromTop(30);

    //auto DCoffsetEnabledButtonArea = slewLimiterArea.removeFromBottom(slewLimiterArea.getHeight() * (1.f / 2.f));
    auto topArea = slewLimiterArea.removeFromTop(slewLimiterArea.getHeight() * (1.f / 2.f));
    auto bottomArea = slewLimiterArea;

    auto slewLimiterTopLabelsArea = topArea.removeFromTop(12);
    auto slewLimiterBottomLabelsArea = bottomArea.removeFromTop(12);

    auto symmetryLabelArea = slewLimiterTopLabelsArea.removeFromLeft(slewLimiterTopLabelsArea.getWidth() * (1.f / 2.f));
    auto biasLabelArea = slewLimiterTopLabelsArea;
    auto riseLabelArea = slewLimiterBottomLabelsArea.removeFromLeft(slewLimiterBottomLabelsArea.getWidth() * (1.f / 3.f));
    auto fallLabelArea = slewLimiterBottomLabelsArea.removeFromLeft(slewLimiterBottomLabelsArea.getWidth() * (1.f / 2.f));
    auto DCoffsetRemoveLabelArea = slewLimiterBottomLabelsArea;

    title.setBounds(titleAndBypassArea);
    title.setJustificationType(juce::Justification::centred);

    slewLimiterSymmetrySlider.setBounds(topArea.removeFromLeft(topArea.getWidth() * (1.f / 2.f)));
    slewLimiterSymmetryLabel.setBounds(symmetryLabelArea);
    slewLimiterSymmetryLabel.setJustificationType(juce::Justification::centred);

    slewLimiterBiasSlider.setBounds(topArea);
    slewLimiterBiasLabel.setBounds(biasLabelArea);
    slewLimiterBiasLabel.setJustificationType(juce::Justification::centred);

    slewLimiterRiseSlider.setBounds(bottomArea.removeFromLeft(bottomArea.getWidth() * (1.f / 3.f)));
    slewLimiterRiseLabel.setBounds(riseLabelArea);
    slewLimiterRiseLabel.setJustificationType(juce::Justification::centred);

    slewLimiterFallSlider.setBounds(bottomArea.removeFromLeft(bottomArea.getWidth() * (1.f / 2.f)));
    slewLimiterFallLabel.setBounds(fallLabelArea);
    slewLimiterFallLabel.setJustificationType(juce::Justification::centred);

    DCoffsetEnabledButton.setBounds(bottomArea);
    DCoffsetEnabledButton.setTransform(juce::AffineTransform::scale(2.f).translated(-347.f, -315.f));
    DCoffsetEnabledButtonLabel.setBounds(DCoffsetRemoveLabelArea);
    DCoffsetEnabledButtonLabel.setJustificationType(juce::Justification::centred);
    DCoffsetEnabledButtonLabel.setCentreRelative(0.804f, 0.68f);

}

std::vector<juce::Component*> SlewLimiterModuleGUI::getComps()
{
    return {
        &title,
        &slewLimiterRiseSlider,
        &slewLimiterFallSlider,
        &slewLimiterSymmetrySlider,
        &slewLimiterBiasSlider,
        &DCoffsetEnabledButton,
        // labels
        &slewLimiterRiseLabel,
        &slewLimiterFallLabel,
        &slewLimiterSymmetryLabel,
        &slewLimiterBiasLabel,
        &DCoffsetEnabledButtonLabel,
        // bypass
        &bypassButton
    };
}