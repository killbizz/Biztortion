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

using namespace juce;

struct ModuleLookAndFeel : public LookAndFeel_V4
{

    static Font getLabelsFont() {
        return Font("Courier New", 14, Font::bold);
    }

    static Font getTitlesFont() {
        return Font("Courier New", 26, Font::bold);
    }

    Font getTextButtonFont(TextButton&, int buttonHeight) override
    {
        return juce::Font("Courier New", buttonHeight*0.39f, Font::bold);
    }

    Font getAlertWindowTitleFont() override {
        return juce::Font("Courier New", 26, 0);
    }

    Font getComboBoxFont(ComboBox&) override {
        return juce::Font("Courier New", 18, 0);
    }

    Font getPopupMenuFont() override {
        return juce::Font("Courier New", 16, 0);
    }

    void drawPopupMenuItem(Graphics& g, const Rectangle< int >& area, bool isSeparator, bool isActive, bool isHighlighted, bool isTicked, bool hasSubMenu,
        const String& text, const String& shortcutKeyText, const Drawable* icon, const Colour* textColourToUse) override {
        if (isSeparator)
        {
            auto r = area.reduced(5, 0);
            r.removeFromTop(roundToInt(((float)r.getHeight() * 0.5f) - 0.5f));

            g.setColour(findColour(PopupMenu::textColourId).withAlpha(0.3f));
            g.fillRect(r.removeFromTop(1));
        }
        else
        {
            auto textColour = (textColourToUse == nullptr ? findColour(PopupMenu::textColourId)
                : *textColourToUse);

            auto r = area.reduced(1);
            auto r_copy = r;

            if (isHighlighted && isActive)
            {
                g.setColour(findColour(PopupMenu::highlightedBackgroundColourId));
                g.fillRect(r);

                g.setColour(findColour(PopupMenu::highlightedTextColourId));
            }
            else
            {
                g.setColour(textColour.withMultipliedAlpha(isActive ? 1.0f : 0.5f));
            }

            r.reduce(jmin(5, area.getWidth() / 20), 0);

            auto font = getPopupMenuFont();

            auto maxFontHeight = (float)r.getHeight() / 1.3f;

            if (font.getHeight() > maxFontHeight)
                font.setHeight(maxFontHeight);

            g.setFont(font);

            auto iconArea = r.removeFromLeft(roundToInt(maxFontHeight)).toFloat();

            if (icon != nullptr)
            {
                icon->drawWithin(g, iconArea, RectanglePlacement::centred | RectanglePlacement::onlyReduceInSize, 1.0f);
                r.removeFromLeft(roundToInt(maxFontHeight * 0.5f));
            }
            else if (isTicked)
            {
                auto tick = getTickShape(1.0f);
                g.fillPath(tick, tick.getTransformToScaleToFit(iconArea.reduced(iconArea.getWidth() / 5, 0).toFloat(), true));
            }

            if (hasSubMenu)
            {
                auto arrowH = 0.6f * getPopupMenuFont().getAscent();

                auto x = static_cast<float> (r.removeFromRight((int)arrowH).getX());
                auto halfH = static_cast<float> (r.getCentreY());

                Path path;
                path.startNewSubPath(x, halfH - arrowH * 0.5f);
                path.lineTo(x + arrowH * 0.6f, halfH);
                path.lineTo(x, halfH + arrowH * 0.5f);

                g.strokePath(path, PathStrokeType(2.0f));
            }

            r.removeFromRight(3);
            g.drawFittedText(text, r_copy, Justification::centred, 1);

            if (shortcutKeyText.isNotEmpty())
            {
                auto f2 = font;
                f2.setHeight(f2.getHeight() * 0.75f);
                f2.setHorizontalScale(0.95f);
                g.setFont(f2);

                g.drawText(shortcutKeyText, r, Justification::centredRight, true);
            }
        }
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

            auto outlineColour = juce::Colours::black;

            g.setColour(baseColour);
            g.fillPath(outline);

            g.setColour(outlineColour);
            g.strokePath(outline, PathStrokeType(lineThickness));
        }
    }
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

    void drawTickBox(Graphics& g, Component& component,
        float x, float y, float w, float h,
        const bool ticked,
        const bool isEnabled,
        const bool shouldDrawButtonAsHighlighted,
        const bool shouldDrawButtonAsDown) override;

    juce::Colour color = juce::Colours::lightgreen;
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
    bool userIsDragging = false;

    void paint(juce::Graphics& g) override;
    juce::Rectangle<int> getSliderBounds() const;
    int getTextHeight() const { return 10; }
    juce::String getDisplayString() const;

    void startedDragging() override;
    void stoppedDragging() override;
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

//==============================================================================

// class for module instantiation logic

class BizComboBox : public juce::ComboBox {
public:
    BizComboBox(juce::TextButton* nm) {
        newModule = nm;
    }

    void focusLost(juce::Component::FocusChangeType ct) override {
        hidePopup();
        setVisible(false);
        juce::ComboBox::focusLost(ct);
        newModule->setToggleState(false, juce::NotificationType::dontSendNotification);
    }
private:
    juce::TextButton* newModule;
};

// classes for drag-and-drop logic

class BizDrawable : public juce::DrawableImage {
public:
    BizDrawable() = default;
    BizDrawable(const juce::DrawableImage& d) : juce::DrawableImage(d) {}
    void mouseDrag(const MouseEvent& event) override;
};

class BizTextButton : public juce::TextButton {
public:
    BizTextButton() = default;
    void mouseDrag(const MouseEvent& event) override;
};

class BizLabel : public juce::Label {
public:
    void mouseDrag(const MouseEvent& event) override;
};