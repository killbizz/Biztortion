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

namespace BiztortionTheme
{
inline const Colour canvas { 18, 20, 22 };
inline const Colour surface { 29, 32, 35 };
inline const Colour surfaceRaised { 37, 40, 44 };
inline const Colour border { 62, 67, 72 };
inline const Colour text { 238, 240, 242 };
inline const Colour textMuted { 157, 164, 171 };
inline const Colour controlTrack { 54, 59, 64 };
inline const Colour controlAccent { 80, 202, 181 };
// warm tone used for the right channel of the analyzers, so an L / R comparison stays readable
// whatever the (cool) accent colour of the module is
inline const Colour analyzerRightChannel { 255, 176, 64 };
inline constexpr int accentOutlineColourId = 0x2020400;
}

struct ModuleLookAndFeel : public LookAndFeel_V4
{
    ModuleLookAndFeel()
    {
        setColour(Label::textColourId, BiztortionTheme::text);
        setColour(HyperlinkButton::textColourId, BiztortionTheme::textMuted);
        setColour(TextButton::buttonColourId, BiztortionTheme::surfaceRaised);
        setColour(TextButton::textColourOffId, BiztortionTheme::text);
        setColour(TextButton::buttonOnColourId, BiztortionTheme::controlAccent);
        setColour(TextButton::textColourOnId, BiztortionTheme::canvas);
        setColour(PopupMenu::backgroundColourId, BiztortionTheme::surfaceRaised);
        setColour(PopupMenu::textColourId, BiztortionTheme::text);
        setColour(PopupMenu::highlightedBackgroundColourId, BiztortionTheme::controlAccent.withAlpha(0.22f));
        setColour(PopupMenu::highlightedTextColourId, BiztortionTheme::text);
        setColour(ComboBox::backgroundColourId, BiztortionTheme::surfaceRaised);
        setColour(ComboBox::textColourId, BiztortionTheme::text);
        setColour(ComboBox::outlineColourId, BiztortionTheme::border);
        setColour(TooltipWindow::backgroundColourId, BiztortionTheme::surfaceRaised);
        setColour(TooltipWindow::textColourId, BiztortionTheme::text);
        setColour(TooltipWindow::outlineColourId, BiztortionTheme::border);
    }

    static Font getLabelsFont() {
        return Font("Segoe UI", 13, Font::bold);
    }

    static Font getTitlesFont() {
        return Font("Segoe UI", 25, Font::bold);
    }

    Font getTextButtonFont(TextButton&, int buttonHeight) override
    {
        return juce::Font("Segoe UI", buttonHeight * 0.38f, Font::bold);
    }

    Font getAlertWindowTitleFont() override {
        return juce::Font("Segoe UI", 24, Font::bold);
    }

    Font getComboBoxFont(ComboBox&) override {
        return juce::Font("Segoe UI", 16, Font::plain);
    }

    Font getPopupMenuFont() override {
        return juce::Font("Segoe UI", 15, Font::plain);
    }

    // padding between the tooltip's frame and its text
    static constexpr int tooltipPaddingX = 11;
    static constexpr int tooltipPaddingY = 7;
    static constexpr int tooltipMaximumWidth = 420;
    // the frame is drawn inside the window bounds; both the measuring and the drawing pass
    // have to allow for it or the text is laid out narrower than it was measured
    static constexpr int tooltipFrameInset = 1;

    static Font getTooltipFont() { return Font("Segoe UI", 13.0f, Font::plain); }

    // Wraps the text properly instead of guessing a line count: the previous version measured
    // the text as one long line and then drew it with a 3 line limit, which squeezed longer
    // tooltips horizontally.
    static TextLayout layoutTooltip(const String& text, float maximumTextWidth)
    {
        AttributedString attributed;
        attributed.setWordWrap(AttributedString::byWord);
        attributed.setJustification(Justification::topLeft);
        attributed.append(text, getTooltipFont(), BiztortionTheme::text);

        TextLayout layout;
        layout.createLayout(attributed, maximumTextWidth);

        return layout;
    }

    Rectangle<int> getTooltipBounds(const String& tipText, Point<int> screenPos,
                                    Rectangle<int> parentArea) override
    {
        const auto maximumTextWidth = (float) (tooltipMaximumWidth - tooltipPaddingX * 2);

        // Measure twice. The first pass finds the natural width of the text, the second one
        // re-measures the height at exactly that width: measuring the height at the maximum
        // width and then shrinking the box clipped every tooltip whose text wrapped once it
        // got narrower.
        const auto textWidth = jlimit(60.0f, maximumTextWidth,
            std::ceil(layoutTooltip(tipText, maximumTextWidth).getWidth()) + 1.0f);
        const auto layout = layoutTooltip(tipText, textWidth);

        const auto width = roundToInt(textWidth) + (tooltipPaddingX + tooltipFrameInset) * 2;
        const auto height = roundToInt(std::ceil(layout.getHeight()))
                          + (tooltipPaddingY + tooltipFrameInset) * 2;
        const auto x = screenPos.x > parentArea.getCentreX()
            ? screenPos.x - width - 12
            : screenPos.x + 20;
        const auto y = screenPos.y > parentArea.getCentreY()
            ? screenPos.y - height - 8
            : screenPos.y + 8;

        return Rectangle<int>(x, y, width, height).constrainedWithin(parentArea);
    }

    void drawTooltip(Graphics& g, const String& text, int width, int height) override
    {
        auto bounds = Rectangle<float>(0.0f, 0.0f, (float) width, (float) height)
                          .reduced((float) tooltipFrameInset);
        DropShadow(Colours::black.withAlpha(0.45f), 6, { 0, 2 })
            .drawForRectangle(g, bounds.getSmallestIntegerContainer());
        g.setColour(BiztortionTheme::surfaceRaised.withAlpha(0.98f));
        g.fillRoundedRectangle(bounds, 6.0f);
        g.setColour(BiztortionTheme::border.brighter(0.12f));
        g.drawRoundedRectangle(bounds, 6.0f, 1.0f);

        const auto textArea = bounds.reduced((float) tooltipPaddingX, (float) tooltipPaddingY);
        layoutTooltip(text, textArea.getWidth()).draw(g, textArea);
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
                g.fillRoundedRectangle(r.toFloat().reduced(2.0f), 5.0f);

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
            g.drawFittedText(text, r_copy.reduced(14, 0), Justification::centredLeft, 1);

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
            auto cornerSize = jmin(7.0f, jmin(width, height) * 0.3f);
            auto lineThickness = 1.0f;
            auto halfThickness = lineThickness * 0.5f;

            Path outline;
            outline.addRoundedRectangle(0.5f + halfThickness, 0.5f + halfThickness,
                                        width - lineThickness, height - lineThickness, cornerSize);

            auto outlineColour = !button.getToggleState()
                                     && button.isColourSpecified(BiztortionTheme::accentOutlineColourId)
                ? button.findColour(BiztortionTheme::accentOutlineColourId)
                : BiztortionTheme::border;

            g.setColour(baseColour);
            g.fillPath(outline);

            g.setColour(outlineColour);
            g.strokePath(outline, PathStrokeType(lineThickness));
        }
    }
};

// A square button holding a single glyph (the modules' "?" button) needs a bigger point size
// than the height-derived default to look the same weight as the wider text buttons next to it.
struct GlyphButtonLookAndFeel : ModuleLookAndFeel
{
    Font getTextButtonFont(TextButton&, int buttonHeight) override
    {
        return juce::Font("Segoe UI", juce::jmax(15.0f, buttonHeight * 0.58f), Font::bold);
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
    std::function<juce::String(double)> displayStringForValue;

    void paint(juce::Graphics& g) override;
    juce::Rectangle<int> getSliderBounds() const;
    int getTextHeight() const { return 11; }
    juce::String getDisplayString() const;

    void startedDragging() override;
    void stoppedDragging() override;
private:

    SliderLookAndFeel lnf;
    juce::RangedAudioParameter* param;
    juce::String suffix;
};

struct PowerButton : juce::ToggleButton { };

//==============================================================================

/**
    Stereo / L-R view switch shared by every module that can show one averaged curve or the two
    channels separately.

    Toggle state on = "Stereo", off = "L / R". In L / R mode the two letters are painted with the
    exact colours the module uses to draw those channels, so the button doubles as the legend.
*/
class StereoViewButton : public juce::Button
{
public:
    StereoViewButton();

    void setModuleColours(juce::Colour module, juce::Colour left, juce::Colour right);

    void paintButton(juce::Graphics& g, bool shouldDrawButtonAsHighlighted,
                     bool shouldDrawButtonAsDown) override;

private:
    juce::Colour moduleColour { BiztortionTheme::controlAccent };
    juce::Colour leftColour { BiztortionTheme::controlAccent };
    juce::Colour rightColour { BiztortionTheme::analyzerRightChannel };
};

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

    // renaming a chain slot: double click, or F2 while the button has keyboard focus
    void mouseDoubleClick(const MouseEvent& event) override;
    bool keyPressed(const juce::KeyPress& key) override;

    std::function<void()> onRenameRequested;
};

//==============================================================================

/**
    Entry of the "create a new module" menu: a colour dot in the module's own colour followed by
    the module name, so the menu doubles as a reminder of the chain colour coding.
*/
class ModuleMenuItemComponent : public juce::PopupMenu::CustomComponent
{
public:
    ModuleMenuItemComponent(juce::Colour moduleColour, juce::String moduleName);

    void getIdealSize(int& idealWidth, int& idealHeight) override;
    void paint(juce::Graphics& g) override;

private:
    juce::Colour colour;
    juce::String name;
};

class BizLabel : public juce::Label {
public:
    void mouseDrag(const MouseEvent& event) override;
};