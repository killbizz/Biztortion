/*
  ==============================================================================

    FilterModule.cpp
    Created: 17 Aug 2021 4:12:53pm
    Author:  gabri

  ==============================================================================
*/

//==============================================================================

/* FilterModule DSP */

//==============================================================================

#include "FilterModule.h"

FilterModule::FilterModule(juce::AudioProcessorValueTreeState& _apvts)
    : apvts(_apvts) {}

void updateCoefficients(Coefficients& old, const Coefficients& replacements) {
    // JUCE allocates coefficients on the HEAP
    *old = *replacements;
}

Coefficients FilterModule::makePeakFilter(const FilterChainSettings& chainSettings, double sampleRate) {
    return juce::dsp::IIR::Coefficients<float>::makePeakFilter(sampleRate,
        chainSettings.peakFreq, chainSettings.peakQuality,
        juce::Decibels::decibelsToGain(chainSettings.peakGainInDecibels));
}

FilterChainSettings FilterModule::getFilterChainSettings(juce::AudioProcessorValueTreeState& apvts) {
    FilterChainSettings settings;

    settings.lowCutFreq = apvts.getRawParameterValue("LowCut Freq")->load();
    settings.highCutFreq = apvts.getRawParameterValue("HighCut Freq")->load();
    settings.peakFreq = apvts.getRawParameterValue("Peak Freq")->load();
    settings.peakGainInDecibels = apvts.getRawParameterValue("Peak Gain")->load();
    settings.peakQuality = apvts.getRawParameterValue("Peak Quality")->load();
    settings.lowCutSlope = apvts.getRawParameterValue("LowCut Slope")->load();
    settings.highCutSlope = apvts.getRawParameterValue("HighCut Slope")->load();

    return settings;
}

void FilterModule::addFilterParameters(juce::AudioProcessorValueTreeState::ParameterLayout& layout)
{
    auto lowCutFreq = std::make_unique<juce::AudioParameterFloat>(
        "LowCut Freq",
        "LowCut Freq",
        juce::NormalisableRange<float>(20.f, 20000.f, 1.f, 0.25f),
        20.f,
        "Filter"
        );
    auto highCutFreq = std::make_unique<juce::AudioParameterFloat>(
        "HighCut Freq",
        "HighCut Freq",
        juce::NormalisableRange<float>(20.f, 20000.f, 1.f, 0.25f),
        20000.f,
        "Filter"
        );
    auto peakFreq = std::make_unique<juce::AudioParameterFloat>(
        "Peak Freq",
        "Peak Freq",
        juce::NormalisableRange<float>(20.f, 20000.f, 1.f, 0.25f),
        800.f,
        "Filter"
        );
    auto peakGain = std::make_unique<juce::AudioParameterFloat>(
        "Peak Gain",
        "Peak Gain",
        juce::NormalisableRange<float>(-24.f, 24.f, 0.5, 1.f),
        0.0f,
        "Filter"
        );
    auto peakQuality = std::make_unique<juce::AudioParameterFloat>(
        "Peak Quality",
        "Peak Quality",
        juce::NormalisableRange<float>(0.1f, 10.f, 0.05f, 1.f),
        1.f,
        "Filter"
        );

    juce::StringArray stringArray;

    for (int i = 0; i < 4; ++i) {
        juce::String string;
        string << (12 + i * 12);
        string << " db/Octave";
        stringArray.add(string);
    }

    auto lowCutSlope = std::make_unique<juce::AudioParameterChoice>(
        "LowCut Slope",
        "LowCut Slope",
        stringArray,
        0,
        "Filter"
        );
    auto highCutSlope = std::make_unique<juce::AudioParameterChoice>(
        "HighCut Slope",
        "HighCut Slope",
        stringArray,
        0,
        "Filter"
        );

    auto lowCutGroup = std::make_unique<juce::AudioProcessorParameterGroup>("LowCut", "LowCut", 
        "|", std::move(lowCutFreq), std::move(lowCutSlope));
    auto highCutGroup = std::make_unique<juce::AudioProcessorParameterGroup>("HighCut", "HighCut", 
        "|", std::move(highCutFreq), std::move(highCutSlope));
    auto peakGroup = std::make_unique<juce::AudioProcessorParameterGroup>("Peak", "Peak",
        "|", std::move(peakFreq), std::move(peakQuality), std::move(peakGain));

    layout.add(std::move(lowCutGroup));
    layout.add(std::move(highCutGroup));
    layout.add(std::move(peakGroup));
}

void FilterModule::updateFilters(double sampleRate) {
    auto chainSettings = getFilterChainSettings(apvts);
    updateLowCutFilter(chainSettings, sampleRate);
    updatePeakFilter(chainSettings, sampleRate);
    updateHighCutFilter(chainSettings, sampleRate);
}

void FilterModule::updatePeakFilter(const FilterChainSettings& chainSettings, double sampleRate) {
    auto peakCoefficients = makePeakFilter(chainSettings, sampleRate);
    // JUCE allocates coefficients on the HEAP
    updateCoefficients(leftChain.get<ChainPositions::Peak>().coefficients, peakCoefficients);
    updateCoefficients(rightChain.get<ChainPositions::Peak>().coefficients, peakCoefficients);
}

void FilterModule::updateLowCutFilter(const FilterChainSettings& chainSettings, double sampleRate) {
    auto lowCutCoefficients = makeLowCutFilter(chainSettings, sampleRate);
    auto& leftLowCut = leftChain.get<ChainPositions::LowCut>();
    updateCutFilter(leftLowCut, lowCutCoefficients, static_cast<FilterSlope>(chainSettings.lowCutSlope));
    auto& rightLowCut = rightChain.get<ChainPositions::LowCut>();
    updateCutFilter(rightLowCut, lowCutCoefficients, static_cast<FilterSlope>(chainSettings.lowCutSlope));
}

void FilterModule::updateHighCutFilter(const FilterChainSettings& chainSettings, double sampleRate) {
    auto highCutCoefficients = makeHighCutFilter(chainSettings, sampleRate);
    auto& leftHighCut = leftChain.get<ChainPositions::HighCut>();
    updateCutFilter(leftHighCut, highCutCoefficients, static_cast<FilterSlope>(chainSettings.highCutSlope));
    auto& rightHighCut = rightChain.get<ChainPositions::HighCut>();
    updateCutFilter(rightHighCut, highCutCoefficients, static_cast<FilterSlope>(chainSettings.highCutSlope));
}

void FilterModule::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    juce::dsp::ProcessSpec spec;

    spec.maximumBlockSize = samplesPerBlock;
    spec.numChannels = 1;
    spec.sampleRate = sampleRate;

    leftChain.prepare(spec);
    rightChain.prepare(spec);

    updateFilters(sampleRate);
}

void FilterModule::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages, double sampleRate)
{
    updateFilters(sampleRate);

    juce::dsp::AudioBlock<float> block(buffer);
    auto leftBlock = block.getSingleChannelBlock(0);
    auto rightBlock = block.getSingleChannelBlock(1);

    juce::dsp::ProcessContextReplacing<float> leftContext(leftBlock);
    juce::dsp::ProcessContextReplacing<float> rightContext(rightBlock);

    leftChain.process(leftContext);
    rightChain.process(rightContext);
}

//==============================================================================

/* FilterModule GUI */

//==============================================================================

void LookAndFeel::drawRotarySlider(juce::Graphics& g,
    int x,
    int y,
    int width,
    int height,
    float sliderPosProportional,
    float rotaryStartAngle,
    float rotaryEndAngle,
    juce::Slider& slider)
{
    using namespace juce;

    auto bounds = Rectangle<float>(x, y, width, height);

    auto enabled = slider.isEnabled();

    // drawing the slider circle
    g.setColour(enabled ? Colour(97u, 18u, 167u) : Colours::darkgrey);
    g.fillEllipse(bounds);

    // drawing the slider border
    g.setColour(enabled ? Colour(255u, 154u, 1u) : Colours::grey);
    g.drawEllipse(bounds, 1.f);

    if (auto* rswl = dynamic_cast<RotarySliderWithLabels*>(&slider))
    {
        // drawing the slider value pointer rectangle
        auto center = bounds.getCentre();
        Path p;

        Rectangle<float> r;
        r.setLeft(center.getX() - 2);
        r.setRight(center.getX() + 2);
        r.setTop(bounds.getY());
        r.setBottom(center.getY() - rswl->getTextHeight() * 1.5);

        p.addRoundedRectangle(r, 2.f);

        jassert(rotaryStartAngle < rotaryEndAngle);

        // mapping the normalized parameter value to a radiant angle value
        auto sliderAngRad = jmap(sliderPosProportional, 0.f, 1.f, rotaryStartAngle, rotaryEndAngle);

        p.applyTransform(AffineTransform().rotated(sliderAngRad, center.getX(), center.getY()));

        g.fillPath(p);

        // drawing slider value label
        g.setFont(rswl->getTextHeight());
        auto text = rswl->getDisplayString();
        auto strWidth = g.getCurrentFont().getStringWidth(text);

        r.setSize(strWidth + 4, rswl->getTextHeight() + 2);
        r.setCentre(bounds.getCentre());

        g.setColour(enabled ? Colours::black : Colours::darkgrey);
        g.fillRect(r);

        g.setColour(enabled ? Colours::white : Colours::lightgrey);
        g.drawFittedText(text, r.toNearestInt(), juce::Justification::centred, 1);
    }
}

//void LookAndFeel::drawToggleButton(juce::Graphics& g,
//    juce::ToggleButton& toggleButton,
//    bool shouldDrawButtonAsHighlighted,
//    bool shouldDrawButtonAsDown)
//{
//    using namespace juce;
//
//    if (auto* pb = dynamic_cast<PowerButton*>(&toggleButton))
//    {
//        Path powerButton;
//
//        auto bounds = toggleButton.getLocalBounds();
//
//        auto size = jmin(bounds.getWidth(), bounds.getHeight()) - 6;
//        auto r = bounds.withSizeKeepingCentre(size, size).toFloat();
//
//        float ang = 30.f; //30.f;
//
//        size -= 6;
//
//        powerButton.addCentredArc(r.getCentreX(),
//            r.getCentreY(),
//            size * 0.5,
//            size * 0.5,
//            0.f,
//            degreesToRadians(ang),
//            degreesToRadians(360.f - ang),
//            true);
//
//        powerButton.startNewSubPath(r.getCentreX(), r.getY());
//        powerButton.lineTo(r.getCentre());
//
//        PathStrokeType pst(2.f, PathStrokeType::JointStyle::curved);
//
//        auto color = toggleButton.getToggleState() ? Colours::dimgrey : Colour(0u, 172u, 1u);
//
//        g.setColour(color);
//        g.strokePath(powerButton, pst);
//        g.drawEllipse(r, 2);
//    }
//    else if (auto* analyzerButton = dynamic_cast<AnalyzerButton*>(&toggleButton))
//    {
//        auto color = !toggleButton.getToggleState() ? Colours::dimgrey : Colour(0u, 172u, 1u);
//
//        g.setColour(color);
//
//        auto bounds = toggleButton.getLocalBounds();
//        g.drawRect(bounds);
//
//        g.strokePath(analyzerButton->randomPath, PathStrokeType(1.f));
//    }
//}

void RotarySliderWithLabels::paint(juce::Graphics& g)
{
    using namespace juce;

    // angle in radiants representing start/end of the slider range
    auto startAng = degreesToRadians(180.f + 45.f);
    auto endAng = degreesToRadians(180.f - 45.f) + MathConstants<float>::twoPi;

    auto range = getRange();

    auto sliderBounds = getSliderBounds();

    //    g.setColour(Colours::red);
    //    g.drawRect(getLocalBounds());
    //    g.setColour(Colours::yellow);
    //    g.drawRect(sliderBounds);

    // drawing custom rotatory slider
    getLookAndFeel().drawRotarySlider(g,
        sliderBounds.getX(),
        sliderBounds.getY(),
        sliderBounds.getWidth(),
        sliderBounds.getHeight(),
        // mapping the slider value to a normalized value
        jmap(getValue(), range.getStart(), range.getEnd(), 0.0, 1.0),
        startAng,
        endAng,
        *this);

    // drawing min/max range value labels
    auto center = sliderBounds.toFloat().getCentre();
    auto radius = sliderBounds.getWidth() * 0.5f;

    g.setColour(Colour(0u, 172u, 1u));
    g.setFont(getTextHeight());

    auto numChoices = labels.size();
    for (int i = 0; i < numChoices; ++i)
    {
        auto pos = labels[i].pos;
        jassert(0.f <= pos);
        jassert(pos <= 1.f);

        auto ang = jmap(pos, 0.f, 1.f, startAng, endAng);

        auto c = center.getPointOnCircumference(radius + getTextHeight() * 0.5f + 1, ang);

        Rectangle<float> r;
        auto str = labels[i].label;
        r.setSize(g.getCurrentFont().getStringWidth(str), getTextHeight());
        r.setCentre(c);
        r.setY(r.getY() + getTextHeight());

        g.drawFittedText(str, r.toNearestInt(), juce::Justification::centred, 1);
    }

}

// returns a square representing the slider bounds
juce::Rectangle<int> RotarySliderWithLabels::getSliderBounds() const
{
    auto bounds = getLocalBounds();

    auto size = juce::jmin(bounds.getWidth(), bounds.getHeight());

    size -= getTextHeight() * 2;
    juce::Rectangle<int> r;
    r.setSize(size, size);
    r.setCentre(bounds.getCentreX(), 0);
    r.setY(2);

    return r;

}

// returns the parameter value or the choice name associated to the parameter as string
juce::String RotarySliderWithLabels::getDisplayString() const
{
    if (auto* choiceParam = dynamic_cast<juce::AudioParameterChoice*>(param))
        return choiceParam->getCurrentChoiceName();

    juce::String str;
    bool addK = false;

    if (auto* floatParam = dynamic_cast<juce::AudioParameterFloat*>(param))
    {
        float val = getValue();

        if (val > 999.f)
        {
            val /= 1000.f; //1001 / 1000 = 1.001
            addK = true;
        }

        str = juce::String(val, (addK ? 2 : 0));
    }
    else
    {
        jassertfalse; //this shouldn't happen!
    }

    if (suffix.isNotEmpty())
    {
        str << " ";
        if (addK)
            str << "k";

        str << suffix;
    }

    return str;
}