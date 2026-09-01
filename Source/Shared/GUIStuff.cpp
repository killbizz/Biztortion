/*
  ==============================================================================

    GUIStuff.cpp

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

#include "GUIStuff.h"

#include "../Module/ChainModule/ChainModule.h"

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
    const auto accent = slider.isColourSpecified(juce::Slider::thumbColourId)
        ? slider.findColour(juce::Slider::thumbColourId)
        : BiztortionTheme::controlAccent;
    const auto trackBounds = bounds.reduced(3.0f);
    const auto currentAngle = jmap(sliderPosProportional, 0.0f, 1.0f,
                                   rotaryStartAngle, rotaryEndAngle);

    Path track;
    track.addCentredArc(trackBounds.getCentreX(), trackBounds.getCentreY(),
                       trackBounds.getWidth() * 0.5f, trackBounds.getHeight() * 0.5f,
                       0.0f, rotaryStartAngle, rotaryEndAngle, true);
    g.setColour(enabled ? BiztortionTheme::controlTrack : BiztortionTheme::controlTrack.darker());
    g.strokePath(track, PathStrokeType(4.0f, PathStrokeType::curved, PathStrokeType::rounded));

    if (enabled)
    {
        Path valueArc;
        valueArc.addCentredArc(trackBounds.getCentreX(), trackBounds.getCentreY(),
                              trackBounds.getWidth() * 0.5f, trackBounds.getHeight() * 0.5f,
                              0.0f, rotaryStartAngle, currentAngle, true);
        g.setColour(accent);
        g.strokePath(valueArc, PathStrokeType(4.0f, PathStrokeType::curved, PathStrokeType::rounded));
    }

    auto knobBounds = bounds.reduced(bounds.getWidth() * 0.16f);
    g.setColour(enabled ? BiztortionTheme::surfaceRaised : BiztortionTheme::surface);
    g.fillEllipse(knobBounds);
    g.setColour(enabled ? BiztortionTheme::border.brighter(0.2f) : BiztortionTheme::border.darker());
    g.drawEllipse(knobBounds, 1.0f);

    if (auto* rswl = dynamic_cast<RotarySliderWithLabels*>(&slider))
    {
        // drawing the slider value pointer rectangle
        auto center = bounds.getCentre();
        Path p;

        Rectangle<float> r;
        r.setLeft(center.getX() - 2);
        r.setRight(center.getX() + 2);
        r.setTop(knobBounds.getY() + 6.0f);
        r.setBottom(center.getY() - rswl->getTextHeight() * 0.45f);

        p.addRoundedRectangle(r, 2.f);

        jassert(rotaryStartAngle < rotaryEndAngle);

        // mapping the normalized parameter value to a radiant angle value
        auto sliderAngRad = currentAngle;

        p.applyTransform(AffineTransform().rotated(sliderAngRad, center.getX(), center.getY()));

        g.setColour(enabled ? BiztortionTheme::text : BiztortionTheme::textMuted.darker());
        g.fillPath(p);

        // drawing slider value label
        if (rswl->userIsDragging) {
            g.setFont(juce::Font("Segoe UI", rswl->getTextHeight() + 1.0f, juce::Font::bold));
            auto text = rswl->getDisplayString();
            auto strWidth = juce::GlyphArrangement::getStringWidthInt(g.getCurrentFont(),text);

            r.setSize(strWidth + 2, rswl->getTextHeight() + 2);
            r.setCentre(bounds.getCentre());

            g.setColour(enabled ? BiztortionTheme::canvas.withAlpha(0.94f) : BiztortionTheme::surface);
            g.fillRoundedRectangle(r.toFloat(), 3.0f);

            g.setColour(enabled ? BiztortionTheme::text : BiztortionTheme::textMuted);
            g.drawFittedText(text, r.toNearestInt(), juce::Justification::centred, 1);
        }
    }
}

void ButtonsLookAndFeel::drawToggleButton(juce::Graphics& g,
    juce::ToggleButton& toggleButton,
    bool shouldDrawButtonAsHighlighted,
    bool shouldDrawButtonAsDown)
{
    using namespace juce;

    if (auto* pb = dynamic_cast<PowerButton*>(&toggleButton))
    {
        Path powerButton;

        auto bounds = toggleButton.getLocalBounds();

        auto size = jmin(bounds.getWidth(), bounds.getHeight()) - 6;
        auto r = bounds.withSizeKeepingCentre(size, size).toFloat();

        float ang = 30.f;

        size -= 6;

        powerButton.addCentredArc(r.getCentreX(),
            r.getCentreY(),
            size * 0.5,
            size * 0.5,
            0.f,
            degreesToRadians(ang),
            degreesToRadians(360.f - ang),
            true);

        powerButton.startNewSubPath(r.getCentreX(), r.getY());
        powerButton.lineTo(r.getCentre());

        PathStrokeType pst(2.f, PathStrokeType::JointStyle::curved);
        // Colour(0u, 172u, 1u)
        auto color = toggleButton.getToggleState() || !toggleButton.isEnabled() ? Colours::dimgrey : this->color;

        g.setColour(color);
        g.strokePath(powerButton, pst);
        g.drawEllipse(r, 2);
    }
    else if (auto* analyzerButton = dynamic_cast<AnalyzerButton*>(&toggleButton))
    {
        const auto enabled = toggleButton.isEnabled();
        const auto on = toggleButton.getToggleState();
        const auto bounds = toggleButton.getLocalBounds().toFloat().reduced(0.5f);
        const auto cornerSize = jmin(7.0f, jmin(bounds.getWidth(), bounds.getHeight()) * 0.3f);

        // same plate as the other buttons in the app, dimmed when the control is unavailable
        g.setColour(enabled ? BiztortionTheme::surfaceRaised
                            : BiztortionTheme::surface.withAlpha(0.6f));
        g.fillRoundedRectangle(bounds, cornerSize);

        const auto activeColour = enabled ? this->color : this->color.withSaturation(0.15f);
        const auto traceColour = !enabled ? BiztortionTheme::textMuted.withAlpha(0.35f)
                                          : (on ? activeColour : BiztortionTheme::textMuted);

        g.setColour(!enabled ? BiztortionTheme::border.withAlpha(0.5f)
                             : (on ? activeColour : BiztortionTheme::border));
        g.drawRoundedRectangle(bounds, cornerSize, 1.0f);

        g.setColour(traceColour);
        g.strokePath(analyzerButton->randomPath, PathStrokeType(1.f));

        // a struck-through trace reads as "no analysis" at a glance
        if (!enabled)
        {
            const auto slash = bounds.reduced(bounds.getWidth() * 0.28f, bounds.getHeight() * 0.28f);
            g.setColour(BiztortionTheme::textMuted.withAlpha(0.55f));
            g.drawLine(slash.getX(), slash.getBottom(), slash.getRight(), slash.getY(), 1.4f);
        }
    }
    else
    {
        auto fontSize = (float)toggleButton.getHeight() * 0.75f;
        auto tickWidth = fontSize * 1.1f;

        drawTickBox(g, toggleButton, 4.0f, ((float)toggleButton.getHeight() - tickWidth) * 0.5f,
            tickWidth, tickWidth,
            toggleButton.getToggleState(),
            toggleButton.isEnabled(),
            shouldDrawButtonAsHighlighted,
            shouldDrawButtonAsDown);

        g.setColour(toggleButton.findColour(ToggleButton::textColourId));
        g.setFont(fontSize);

        if (!toggleButton.isEnabled())
            g.setOpacity(0.5f);

        g.drawFittedText(toggleButton.getButtonText(),
            toggleButton.getLocalBounds().withTrimmedLeft(roundToInt(tickWidth) + 10)
            .withTrimmedRight(2),
            Justification::centredLeft, 10);
    }
}

void ButtonsLookAndFeel::drawTickBox(Graphics& g, Component& component, float x, float y, float w, float h, const bool ticked, const bool isEnabled, const bool shouldDrawButtonAsHighlighted, const bool shouldDrawButtonAsDown)
{
    ignoreUnused(shouldDrawButtonAsHighlighted, shouldDrawButtonAsDown);

    Rectangle<float> tickBounds(x, y, w, h);

    g.setColour(isEnabled ? component.findColour(ToggleButton::tickDisabledColourId) : juce::Colours::dimgrey);
    g.drawRoundedRectangle(tickBounds, 4.0f, 1.0f);

    if (ticked)
    {
        g.setColour(isEnabled ? component.findColour(ToggleButton::tickColourId) : juce::Colours::dimgrey);
        auto tick = getTickShape(0.75f);
        g.fillPath(tick, tick.getTransformToScaleToFit(tickBounds.reduced(4, 5).toFloat(), false));
    }
}

RotarySliderWithLabels::RotarySliderWithLabels(juce::RangedAudioParameter& rap, const juce::String& unitSuffix) :
    juce::Slider(juce::Slider::SliderStyle::RotaryHorizontalVerticalDrag,
        juce::Slider::TextEntryBoxPosition::NoTextBox),
    param(&rap),
    suffix(unitSuffix)
{
    setLookAndFeel(&lnf);
}

RotarySliderWithLabels::~RotarySliderWithLabels()
{
    setLookAndFeel(nullptr);
}

void RotarySliderWithLabels::paint(juce::Graphics& g)
{
    using namespace juce;

    // angle in radiants representing start/end of the slider range
    auto startAng = degreesToRadians(180.f + 45.f);
    auto endAng = degreesToRadians(180.f - 45.f) + MathConstants<float>::twoPi;

    auto range = getRange();

    auto sliderBounds = getSliderBounds();
    // correction of sliderBounds
    sliderBounds.setY(2);

    // draw slider bounds
    /*g.setColour(Colours::red);
    g.drawRect(getLocalBounds());
    g.setColour(Colours::yellow);
    g.drawRect(sliderBounds);*/

    // drawing custom rotatory slider
    getLookAndFeel().drawRotarySlider(g,
        sliderBounds.getX(),
        sliderBounds.getY(),
        sliderBounds.getWidth(),
        sliderBounds.getHeight(),
        static_cast<float>(valueToProportionOfLength(getValue())),
        startAng,
        endAng,
        *this);

    // drawing min/max range value labels
    auto center = sliderBounds.toFloat().getCentre();
    auto radius = sliderBounds.getWidth() * 0.525f;

    g.setColour(isEnabled() ? BiztortionTheme::textMuted : BiztortionTheme::textMuted.darker());
    g.setFont(juce::Font("Segoe UI", getTextHeight(), juce::Font::plain));

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
        r.setSize(juce::GlyphArrangement::getStringWidthInt(g.getCurrentFont(),str), getTextHeight());
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

    return r;

}

// returns the parameter value or the choice name associated to the parameter as string
juce::String RotarySliderWithLabels::getDisplayString() const
{
    if (displayStringForValue)
        return displayStringForValue(getValue());

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
    else if (dynamic_cast<juce::AudioParameterInt*>(param) != nullptr)
    {
        str = juce::String(juce::roundToInt(getValue()));
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

void RotarySliderWithLabels::startedDragging()
{
    userIsDragging = true;
}

void RotarySliderWithLabels::stoppedDragging()
{
    userIsDragging = false;
}

//==============================================================================

StereoViewButton::StereoViewButton()
    : juce::Button("Stereo view")
{
    setClickingTogglesState(true);
}

void StereoViewButton::setModuleColours(juce::Colour module, juce::Colour left, juce::Colour right)
{
    moduleColour = module;
    leftColour = left;
    rightColour = right;
    repaint();
}

void StereoViewButton::paintButton(juce::Graphics& g, bool shouldDrawButtonAsHighlighted,
                                   bool shouldDrawButtonAsDown)
{
    const auto enabled = isEnabled();
    const auto stereo = getToggleState();
    const auto bounds = getLocalBounds().toFloat().reduced(0.5f);
    const auto cornerSize = jmin(7.0f, jmin(bounds.getWidth(), bounds.getHeight()) * 0.3f);

    auto plate = stereo ? moduleColour : BiztortionTheme::surfaceRaised;
    if (shouldDrawButtonAsDown)
        plate = plate.contrasting(0.2f);
    else if (shouldDrawButtonAsHighlighted)
        plate = plate.contrasting(0.1f);

    g.setColour(enabled ? plate : plate.withMultipliedAlpha(0.5f));
    g.fillRoundedRectangle(bounds, cornerSize);

    // the outline always carries the module colour, so the switch reads as part of the module
    g.setColour(enabled ? moduleColour : moduleColour.withMultipliedSaturation(0.2f).withAlpha(0.5f));
    g.drawRoundedRectangle(bounds, cornerSize, 1.0f);

    const auto font = juce::Font("Segoe UI", jmax(11.0f, bounds.getHeight() * 0.42f), Font::bold);
    g.setFont(font);

    if (stereo)
    {
        g.setColour(enabled ? BiztortionTheme::canvas : BiztortionTheme::canvas.withAlpha(0.6f));
        g.drawText("Stereo", getLocalBounds(), Justification::centred, false);
        return;
    }

    // "L / R" with each letter in the colour that channel is drawn with
    const auto separator = juce::String(" / ");
    const auto leftWidth = (float) GlyphArrangement::getStringWidthInt(font, "L");
    const auto rightWidth = (float) GlyphArrangement::getStringWidthInt(font, "R");
    const auto separatorWidth = (float) GlyphArrangement::getStringWidthInt(font, separator);
    const auto totalWidth = leftWidth + separatorWidth + rightWidth;

    auto x = bounds.getCentreX() - totalWidth * 0.5f;
    const auto drawGlyph = [&](const juce::String& text, float width, juce::Colour colour) {
        g.setColour(enabled ? colour : colour.withMultipliedSaturation(0.2f).withAlpha(0.6f));
        g.drawText(text, juce::Rectangle<float>(x, bounds.getY(), width, bounds.getHeight()),
                   Justification::centred, false);
        x += width;
    };

    drawGlyph("L", leftWidth, leftColour);
    drawGlyph(separator, separatorWidth, BiztortionTheme::textMuted);
    drawGlyph("R", rightWidth, rightColour);
}

//==============================================================================

void AnalyzerButton::resized()
{
    auto bounds = getLocalBounds();
    auto insetRect = bounds.reduced(4);

    randomPath.clear();

    juce::Random r;

    randomPath.startNewSubPath(insetRect.getX(),
        insetRect.getY() + insetRect.getHeight() * r.nextFloat());

    for (auto x = insetRect.getX() + 1; x < insetRect.getRight(); x += 2)
    {
        randomPath.lineTo(x,
            insetRect.getY() + insetRect.getHeight() * r.nextFloat());
    }
}

//==============================================================================

void BizDrawable::mouseDrag(const MouseEvent& event)
{
    auto newModule = dynamic_cast<ChainModuleGUI*>(getParentComponent());
    auto editor = dynamic_cast<juce::DragAndDropContainer*>(getParentComponent());
    if (newModule->getModuleType() != ModuleType::Uninstantiated)
        editor->startDragging("", newModule);
}

void BizTextButton::mouseDrag(const MouseEvent& event)
{
    auto newModule = dynamic_cast<ChainModuleGUI*>(getParentComponent());
    auto editor = dynamic_cast<juce::DragAndDropContainer*>(getParentComponent()->getParentComponent());
    if (newModule->getModuleType() != ModuleType::Uninstantiated)
        editor->startDragging("", newModule);
}

void BizTextButton::mouseDoubleClick(const MouseEvent&)
{
    if (onRenameRequested != nullptr)
        onRenameRequested();
}

bool BizTextButton::keyPressed(const juce::KeyPress& key)
{
    if (key == juce::KeyPress::F2Key && onRenameRequested != nullptr)
    {
        onRenameRequested();
        return true;
    }

    return juce::TextButton::keyPressed(key);
}

//==============================================================================

ModuleMenuItemComponent::ModuleMenuItemComponent(juce::Colour moduleColour, juce::String moduleName)
    : colour(moduleColour), name(std::move(moduleName))
{
}

void ModuleMenuItemComponent::getIdealSize(int& idealWidth, int& idealHeight)
{
    idealWidth = 250;
    idealHeight = 32;
}

void ModuleMenuItemComponent::paint(juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat().reduced(3.0f, 1.0f);

    if (isItemHighlighted())
    {
        g.setColour(colour.withAlpha(0.18f));
        g.fillRoundedRectangle(bounds, 5.0f);
        g.setColour(colour.withAlpha(0.55f));
        g.drawRoundedRectangle(bounds, 5.0f, 1.0f);
    }

    bounds.removeFromLeft(8.0f);

    // colour dot matching the module's colour in the chain
    const auto dotArea = bounds.removeFromLeft(10.0f).withSizeKeepingCentre(10.0f, 10.0f);
    g.setColour(colour);
    g.fillEllipse(dotArea);

    bounds.removeFromLeft(10.0f);

    g.setColour(isItemHighlighted() ? BiztortionTheme::text : BiztortionTheme::text.withAlpha(0.88f));
    g.setFont(juce::Font("Segoe UI", 15.0f, juce::Font::plain));
    g.drawText(name, bounds.getSmallestIntegerContainer(), juce::Justification::centredLeft, true);
}

void BizLabel::mouseDrag(const MouseEvent& event)
{
    auto newModule = dynamic_cast<ChainModuleGUI*>(getParentComponent());
    auto editor = dynamic_cast<juce::DragAndDropContainer*>(getParentComponent()->getParentComponent());
    if (newModule->getModuleType() != ModuleType::Uninstantiated)
        editor->startDragging("", newModule);
}
