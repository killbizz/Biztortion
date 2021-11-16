/*
  ==============================================================================

    GUIStuff.h

    Copyright (c) 2021 KillBizz - Gabriel Bizzo

  ==============================================================================
*/

/*
  ==============================================================================

    Copyright (c) 2021 Charles Schiermeyer
    Content: the original Buttons/Sliders LookAndFeels
    Source: https://github.com/matkatmusic/SimpleEQ

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

#pragma once

#include <JuceHeader.h>

//==============================================================================
/** Custom Look And Feel subclass.

    Simply override the methods you need to, anything else will be inherited from the base class.
    It's a good idea not to hard code your colours, use the findColour method along with appropriate
    ColourIds so you can set these on a per-component basis.
*/

using namespace juce;

struct ModuleLookAndFeel : public LookAndFeel_V4
{

    Font getTextButtonFont(TextButton&, int buttonHeight) override
    {
        return juce::Font("Prestige Elite Std", buttonHeight*0.35f, 0);
    }

    void drawRoundThumb(Graphics& g, float x, float y, float diameter, Colour colour, float outlineThickness)
    {
        auto halfThickness = outlineThickness * 0.5f;

        Path p;
        p.addEllipse(x + halfThickness,
            y + halfThickness,
            diameter - outlineThickness,
            diameter - outlineThickness);

        DropShadow(Colours::black, 1, {}).drawForPath(g, p);

        g.setColour(colour);
        g.fillPath(p);

        g.setColour(colour.brighter());
        g.strokePath(p, PathStrokeType(outlineThickness));
    }

    void drawButtonBackground(Graphics& g, Button& button, const Colour& backgroundColour,
        bool isMouseOverButton, bool isButtonDown) override
    {
        auto baseColour = backgroundColour.withMultipliedSaturation(button.hasKeyboardFocus(true) ? 1.3f : 0.9f)
            .withMultipliedAlpha(button.isEnabled() ? 0.9f : 0.5f);

        if (isButtonDown || isMouseOverButton)
            baseColour = baseColour.contrasting(isButtonDown ? 0.2f : 0.1f);

        auto flatOnLeft = button.isConnectedOnLeft();
        auto flatOnRight = button.isConnectedOnRight();
        auto flatOnTop = button.isConnectedOnTop();
        auto flatOnBottom = button.isConnectedOnBottom();

        auto width = (float)button.getWidth() - 1.0f;
        auto height = (float)button.getHeight() - 1.0f;

        if (width > 0 && height > 0)
        {
            auto cornerSize = jmin(15.0f, jmin(width, height) * 0.45f);
            auto lineThickness = cornerSize * 0.1f;
            auto halfThickness = lineThickness * 0.5f;

            Path outline;
            outline.addRectangle(0.5f + halfThickness, 0.5f + halfThickness, width - lineThickness, height - lineThickness);
                /*cornerSize, cornerSize,
                !(flatOnLeft || flatOnTop),
                !(flatOnRight || flatOnTop),
                !(flatOnLeft || flatOnBottom),
                !(flatOnRight || flatOnBottom));*/

            auto outlineColour = button.findColour(button.getToggleState() ? TextButton::textColourOnId
                : TextButton::textColourOffId);

            g.setColour(baseColour);
            g.fillPath(outline);

            if (!button.getToggleState())
            {
                g.setColour(outlineColour);
                g.strokePath(outline, PathStrokeType(lineThickness));
            }
        }
    }

    //void drawTickBox(Graphics& g, Component& component,
    //    float x, float y, float w, float h,
    //    bool ticked,
    //    bool isEnabled,
    //    bool isMouseOverButton,
    //    bool isButtonDown) override
    //{
    //    auto boxSize = w * 0.7f;

    //    auto isDownOrDragging = component.isEnabled() && (component.isMouseOverOrDragging() || component.isMouseButtonDown());

    //    auto colour = component.findColour(TextButton::buttonColourId)
    //        .withMultipliedSaturation((component.hasKeyboardFocus(false) || isDownOrDragging) ? 1.3f : 0.9f)
    //        .withMultipliedAlpha(component.isEnabled() ? 1.0f : 0.7f);

    //    drawRoundThumb(g, x, y + (h - boxSize) * 0.5f, boxSize, colour,
    //        isEnabled ? ((isButtonDown || isMouseOverButton) ? 1.1f : 0.5f) : 0.3f);

    //    if (ticked)
    //    {
    //        g.setColour(isEnabled ? findColour(TextButton::buttonOnColourId) : Colours::grey);

    //        auto scale = 9.0f;
    //        auto trans = AffineTransform::scale(w / scale, h / scale).translated(x - 2.5f, y + 1.0f);

    //        g.fillPath(LookAndFeel_V4::getTickShape(6.0f), trans);
    //    }
    //}

    //void drawLinearSliderThumb(Graphics& g, int x, int y, int width, int height,
    //    float sliderPos, float minSliderPos, float maxSliderPos,
    //    const Slider::SliderStyle style, Slider& slider) override
    //{
    //    auto sliderRadius = (float)(getSliderThumbRadius(slider) - 2);

    //    auto isDownOrDragging = slider.isEnabled() && (slider.isMouseOverOrDragging() || slider.isMouseButtonDown());

    //    auto knobColour = slider.findColour(Slider::thumbColourId)
    //        .withMultipliedSaturation((slider.hasKeyboardFocus(false) || isDownOrDragging) ? 1.3f : 0.9f)
    //        .withMultipliedAlpha(slider.isEnabled() ? 1.0f : 0.7f);

    //    if (style == Slider::LinearHorizontal || style == Slider::LinearVertical)
    //    {
    //        float kx, ky;

    //        if (style == Slider::LinearVertical)
    //        {
    //            kx = (float)x + (float)width * 0.5f;
    //            ky = sliderPos;
    //        }
    //        else
    //        {
    //            kx = sliderPos;
    //            ky = (float)y + (float)height * 0.5f;
    //        }

    //        auto outlineThickness = slider.isEnabled() ? 0.8f : 0.3f;

    //        drawRoundThumb(g,
    //            kx - sliderRadius,
    //            ky - sliderRadius,
    //            sliderRadius * 2.0f,
    //            knobColour, outlineThickness);
    //    }
    //    else
    //    {
    //        // Just call the base class for the demo
    //        LookAndFeel_V2::drawLinearSliderThumb(g, x, y, width, height, sliderPos, minSliderPos, maxSliderPos, style, slider);
    //    }
    //}

    //void drawLinearSlider(Graphics& g, int x, int y, int width, int height,
    //    float sliderPos, float minSliderPos, float maxSliderPos,
    //    const Slider::SliderStyle style, Slider& slider) override
    //{
    //    g.fillAll(slider.findColour(Slider::backgroundColourId));

    //    if (style == Slider::LinearBar || style == Slider::LinearBarVertical)
    //    {
    //        Path p;

    //        if (style == Slider::LinearBarVertical)
    //            p.addRectangle((float)x, sliderPos, (float)width, 1.0f + (float)height - sliderPos);
    //        else
    //            p.addRectangle((float)x, (float)y, sliderPos - (float)x, (float)height);

    //        auto baseColour = slider.findColour(Slider::rotarySliderFillColourId)
    //            .withMultipliedSaturation(slider.isEnabled() ? 1.0f : 0.5f)
    //            .withMultipliedAlpha(0.8f);

    //        g.setColour(baseColour);
    //        g.fillPath(p);

    //        auto lineThickness = jmin(15.0f, (float)jmin(width, height) * 0.45f) * 0.1f;
    //        g.drawRect(slider.getLocalBounds().toFloat(), lineThickness);
    //    }
    //    else
    //    {
    //        drawLinearSliderBackground(g, x, y, width, height, sliderPos, minSliderPos, maxSliderPos, style, slider);
    //        drawLinearSliderThumb(g, x, y, width, height, sliderPos, minSliderPos, maxSliderPos, style, slider);
    //    }
    //}

    //void drawLinearSliderBackground(Graphics& g, int x, int y, int width, int height,
    //    float /*sliderPos*/,
    //    float /*minSliderPos*/,
    //    float /*maxSliderPos*/,
    //    const Slider::SliderStyle /*style*/, Slider& slider) override
    //{
    //    auto sliderRadius = (float)getSliderThumbRadius(slider) - 5.0f;
    //    Path on, off;

    //    if (slider.isHorizontal())
    //    {
    //        auto iy = (float)y + (float)height * 0.5f - sliderRadius * 0.5f;
    //        Rectangle<float> r((float)x - sliderRadius * 0.5f, iy, (float)width + sliderRadius, sliderRadius);
    //        auto onW = r.getWidth() * ((float)slider.valueToProportionOfLength(slider.getValue()));

    //        on.addRectangle(r.removeFromLeft(onW));
    //        off.addRectangle(r);
    //    }
    //    else
    //    {
    //        auto ix = (float)x + (float)width * 0.5f - sliderRadius * 0.5f;
    //        Rectangle<float> r(ix, (float)y - sliderRadius * 0.5f, sliderRadius, (float)height + sliderRadius);
    //        auto onH = r.getHeight() * ((float)slider.valueToProportionOfLength(slider.getValue()));

    //        on.addRectangle(r.removeFromBottom(onH));
    //        off.addRectangle(r);
    //    }

    //    g.setColour(slider.findColour(Slider::rotarySliderFillColourId));
    //    g.fillPath(on);

    //    g.setColour(slider.findColour(Slider::trackColourId));
    //    g.fillPath(off);
    //}

    //void drawRotarySlider(Graphics& g, int x, int y, int width, int height, float sliderPos,
    //    float rotaryStartAngle, float rotaryEndAngle, Slider& slider) override
    //{
    //    auto radius = (float)jmin(width / 2, height / 2) - 2.0f;
    //    auto centreX = (float)x + (float)width * 0.5f;
    //    auto centreY = (float)y + (float)height * 0.5f;
    //    auto rx = centreX - radius;
    //    auto ry = centreY - radius;
    //    auto rw = radius * 2.0f;
    //    auto angle = rotaryStartAngle + sliderPos * (rotaryEndAngle - rotaryStartAngle);
    //    auto isMouseOver = slider.isMouseOverOrDragging() && slider.isEnabled();

    //    if (slider.isEnabled())
    //        g.setColour(slider.findColour(Slider::rotarySliderFillColourId).withAlpha(isMouseOver ? 1.0f : 0.7f));
    //    else
    //        g.setColour(Colour(0x80808080));

    //    {
    //        Path filledArc;
    //        filledArc.addPieSegment(rx, ry, rw, rw, rotaryStartAngle, angle, 0.0);
    //        g.fillPath(filledArc);
    //    }

    //    {
    //        auto lineThickness = jmin(15.0f, (float)jmin(width, height) * 0.45f) * 0.1f;
    //        Path outlineArc;
    //        outlineArc.addPieSegment(rx, ry, rw, rw, rotaryStartAngle, rotaryEndAngle, 0.0);
    //        g.strokePath(outlineArc, PathStrokeType(lineThickness));
    //    }
    //}
};

struct SliderLookAndFeel : juce::LookAndFeel_V4
{
    void drawRotarySlider(juce::Graphics&,
        int x, int y, int width, int height,
        float sliderPosProportional,
        float rotaryStartAngle,
        float rotaryEndAngle,
        juce::Slider&) override;
};

struct ButtonsLookAndFeel : juce::LookAndFeel_V4
{
    void drawToggleButton(juce::Graphics& g,
        juce::ToggleButton& toggleButton,
        bool shouldDrawButtonAsHighlighted,
        bool shouldDrawButtonAsDown) override;
};

struct LabelWithPosition
{
    float pos;
    juce::String label;
};

struct RotarySliderWithLabels : juce::Slider
{
    RotarySliderWithLabels(juce::RangedAudioParameter& rap, const juce::String& unitSuffix);
    ~RotarySliderWithLabels();

    juce::Array<LabelWithPosition> labels;

    void paint(juce::Graphics& g) override;
    juce::Rectangle<int> getSliderBounds() const;
    int getTextHeight() const { return 10; }
    juce::String getDisplayString() const;
private:

    SliderLookAndFeel lnf;
    juce::RangedAudioParameter* param;
    juce::String suffix;
};

struct PowerButton : juce::ToggleButton { };

struct AnalyzerButton : juce::ToggleButton
{
    void resized() override;

    juce::Path randomPath;
};