/*
  ==============================================================================

    GUIStuff.cpp
    Created: 29 Aug 2021 9:54:43pm
    Author:  gabri

  ==============================================================================
*/

#include "GUIStuff.h"

void SliderLookAndFeel::drawRotarySlider(juce::Graphics& g,
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
    g.setColour(enabled ? Colours::white : Colours::darkgrey);
    g.fillEllipse(bounds);

    // drawing the slider border
    g.setColour(enabled ? Colours::black : Colours::grey);
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
        r.setBottom(center.getY() - rswl->getTextHeight() * 0.8);

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

        r.setSize(strWidth + 2, rswl->getTextHeight() + 2);
        r.setCentre(bounds.getCentre());

        g.setColour(enabled ? Colours::black.withAlpha(0.6f) : Colours::darkgrey);
        g.fillRect(r);

        g.setColour(enabled ? Colours::white : Colours::lightgrey);
        g.drawFittedText(text, r.toNearestInt(), juce::Justification::centred, 1);
    }
}

//void SliderLookAndFeel::drawToggleButton(juce::Graphics& g,
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

    g.setColour(Colours::black);
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