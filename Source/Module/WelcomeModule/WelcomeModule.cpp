/*
  ==============================================================================

    WelcomeModule.cpp

    Copyright (c) 2021 KillBizz - Gabriel Bizzo

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

#include "WelcomeModule.h"

#include <array>

#include "../../Shared/GUIStuff.h"
#include "../../Shared/ModuleType.h"

namespace
{
    struct GettingStartedStep
    {
        const char* title;
        const char* description;
        ModuleType colourFrom;
    };

    // Four steps, in the order someone actually does them. The badge of each one borrows a
    // module colour, so the palette here matches the chain and the legend in the header.
    const std::array<GettingStartedStep, 4> gettingStartedSteps { {
        { "Build a chain",
          "Click + on a free slot below and pick a processor. Audio flows left to right.",
          ModuleType::Equalizer },
        { "Open a module",
          "Click a slot to bring its controls into this view.",
          ModuleType::Waveshaper },
        { "Rearrange it",
          "Drag slots to change the order, double-click a name to rename it.",
          ModuleType::ClassicBitcrusher },
        { "See what you hear",
          "Drop in a Scope, Spectrum Analyzer or Equalizer anywhere in the chain.",
          ModuleType::SpectrumAnalyzer }
    } };

    // below this the two column split gets too cramped, so the logo makes way for the text
    constexpr float minimumWidthForLogoColumn = 470.0f;

    // size of this panel at the smallest allowed window: the type is tuned for it and scales up
    constexpr float baseContentWidth = 520.0f;
    constexpr float baseContentHeight = 290.0f;
    constexpr float maximumContentScale = 1.9f;
}

WelcomeModuleGUI::WelcomeModuleGUI()
{
    bigNoseImg = juce::ImageFileFormat::loadFrom(BinaryData::biztortionNoseAlpha_png, BinaryData::biztortionNoseAlpha_pngSize);

    author.setText("(c) 2026 Gabriel Bizzo", juce::dontSendNotification);
    author.setFont(juce::Font("Segoe UI", 14, juce::Font::plain));
    author.setColour(juce::Label::textColourId, BiztortionTheme::textMuted);

    version.setText("version: 2.0", juce::dontSendNotification);
    version.setFont(juce::Font("Segoe UI", 14, juce::Font::plain));
    version.setColour(juce::Label::textColourId, BiztortionTheme::textMuted);

    std::random_device rd; // obtain a random number from hardware
    std::mt19937 gen(rd()); // seed the generator
    std::uniform_int_distribution<> distr(0, 100); // define the range

    hueCounter = distr(gen);

    for (auto* comp : getAllComps())
    {
        addAndMakeVisible(comp);
    }

    if (isTimerRunning())
        stopTimer();

    startTimerHz(40);

}

void WelcomeModuleGUI::paint(juce::Graphics& g)
{
    const auto pulse = 0.5f + 0.5f * std::sin((float) hueCounter++ * 0.055f);
    const auto brightness = juce::jmap(pulse, 0.58f, 1.0f);
    moduleColor = juce::Colour::fromFloatRGBA(brightness, brightness, brightness, 0.92f);
    drawContainer(g);

    if (! logoArea.isEmpty())
    {
        {
            juce::Graphics::ScopedSaveState state(g);
            g.addTransform(getTransform());
            juce::DropShadow(juce::Colours::white.withAlpha(0.16f + pulse * 0.28f),
                             juce::roundToInt(8.0f + pulse * 8.0f), {})
                .drawForImage(g, bigNoseImg);
        }

        juce::Graphics::ScopedSaveState state(g);
        g.reduceClipRegion(bigNoseImg, getTransform());
        g.setColour(moduleColor);
        g.fillAll();
    }

    drawGettingStarted(g);

    if (hueCounter == 100000)
        hueCounter = 0;
}

void WelcomeModuleGUI::drawGettingStarted(juce::Graphics& g)
{
    using namespace juce;

    if (guideArea.getWidth() < 160.0f || guideArea.getHeight() < 120.0f)
        return;

    // sizes are quoted at the smallest window and scaled from there
    const auto badgeDiameter = 22.0f * contentScale;
    const auto badgeGap = 10.0f * contentScale;
    const auto headingHeight = 28.0f * contentScale;
    const auto headingGap = 8.0f * contentScale;
    const auto titleHeight = 18.0f * contentScale;
    const auto stepGap = 14.0f * contentScale;
    const auto hintGap = 18.0f * contentScale;

    const Font headingFont("Segoe UI", 21.0f * contentScale, Font::bold);
    const Font titleFont("Segoe UI", 14.0f * contentScale, Font::bold);
    const Font bodyFont("Segoe UI", 12.5f * contentScale, Font::plain);
    const Font hintFont("Segoe UI", 12.0f * contentScale, Font::plain);
    const String hint("Hover any control for a tip, or use a module's ? button for the full story.");

    const auto textWidth = guideArea.getWidth() - badgeDiameter - badgeGap;

    const auto layoutFor = [](const String& text, const Font& font, juce::Colour colour, float width) {
        AttributedString attributed;
        attributed.setWordWrap(AttributedString::byWord);
        attributed.append(text, font, colour);

        TextLayout layout;
        layout.createLayout(attributed, width);

        return layout;
    };

    // Measure the whole block first, then centre it as one piece: laying it out from the top
    // and pinning the hint to the bottom pulled the two apart as the window grew.
    std::array<TextLayout, gettingStartedSteps.size()> descriptions;
    auto blockHeight = headingHeight + headingGap;

    for (size_t step = 0; step < gettingStartedSteps.size(); ++step)
    {
        descriptions[step] = layoutFor(gettingStartedSteps[step].description, bodyFont,
                                       BiztortionTheme::textMuted, textWidth);
        blockHeight += titleHeight + descriptions[step].getHeight() + stepGap;
    }

    const auto hintLayout = layoutFor(hint, hintFont, BiztortionTheme::textMuted.withAlpha(0.85f),
                                      guideArea.getWidth());
    blockHeight += hintGap - stepGap + hintLayout.getHeight();

    // Walk a cursor down the column instead of consuming a rectangle clamped to blockHeight:
    // that clamp left the hint short by a rounding error at some sizes, so it silently
    // disappeared. The only limit now is the real bottom of the column.
    const auto hintHeight = hintLayout.getHeight();
    const auto bottom = guideArea.getBottom();
    auto y = guideArea.getY() + jmax(0.0f, (guideArea.getHeight() - blockHeight) * 0.5f);

    const auto rowAt = [this](float top, float height) {
        return Rectangle<float>(guideArea.getX(), top, guideArea.getWidth(), height);
    };

    g.setColour(BiztortionTheme::text);
    g.setFont(headingFont);
    g.drawText("Getting started", rowAt(y, headingHeight).getSmallestIntegerContainer(),
               Justification::topLeft, false);
    y += headingHeight + headingGap;

    for (size_t step = 0; step < gettingStartedSteps.size(); ++step)
    {
        const auto& entry = gettingStartedSteps[step];
        const auto rowHeight = titleHeight + descriptions[step].getHeight();

        // keep the hint's room reserved, so a step is dropped before the hint ever is
        if (y + rowHeight + (hintGap - stepGap) + hintHeight > bottom)
            break;

        const auto accent = moduleType_colors.at(entry.colourFrom);

        // numbered badge
        const auto badge = Rectangle<float>(badgeDiameter, badgeDiameter)
                               .withPosition(guideArea.getX(), y + 1.0f);
        g.setColour(accent.withAlpha(0.16f));
        g.fillEllipse(badge);
        g.setColour(accent);
        g.drawEllipse(badge.reduced(0.5f), contentScale);
        g.setFont(Font("Segoe UI", 12.0f * contentScale, Font::bold));
        g.drawText(String((int) step + 1), badge.getSmallestIntegerContainer(),
                   Justification::centred, false);

        auto text = rowAt(y, rowHeight).withTrimmedLeft(badgeDiameter + badgeGap);

        g.setColour(BiztortionTheme::text);
        g.setFont(titleFont);
        g.drawText(entry.title, text.removeFromTop(titleHeight).getSmallestIntegerContainer(),
                   Justification::topLeft, false);

        // wrapped so the description reflows with the window instead of being clipped
        descriptions[step].draw(g, text);

        y += rowHeight + stepGap;
    }

    // the hint travels with the steps instead of sitting at the bottom of the panel
    y += hintGap - stepGap;

    // the epsilon absorbs the rounding of the measured heights added up above
    if (y + hintHeight <= bottom + 1.0f)
        hintLayout.draw(g, rowAt(y, hintHeight));
}

void WelcomeModuleGUI::resized()
{
    auto content = getContentRenderArea();

    // everything in this panel is drawn, not laid out in child components, so a single scale
    // factor keeps the logo, the type and the spacing growing together with the window
    contentScale = juce::jlimit(1.0f, maximumContentScale,
        juce::jmin((float) content.getWidth() / baseContentWidth,
                   (float) content.getHeight() / baseContentHeight));

    auto topArea = content.removeFromTop(juce::roundToInt(24.0f * contentScale));

    const auto captionFont = juce::Font("Segoe UI", 14.0f * contentScale, juce::Font::plain);
    author.setFont(captionFont);
    version.setFont(captionFont);

    author.setBounds(topArea);
    author.setJustificationType(juce::Justification::centredLeft);

    version.setBounds(topArea);
    version.setJustificationType(juce::Justification::centredRight);

    // Same split the distortion modules use: brand mark on the left, content on the right.
    // On a narrow window the logo would squeeze the text, so it gives up its column instead.
    auto body = content.reduced(6, 8).toFloat();

    if (body.getWidth() >= minimumWidthForLogoColumn)
    {
        logoArea = body.removeFromLeft(body.getWidth() * 0.38f);
        body.removeFromLeft(14.0f);
    }
    else
    {
        logoArea = {};
    }

    guideArea = body;
}

std::vector<juce::Component*> WelcomeModuleGUI::getAllComps()
{
    return {
        &author,
        &version
    };
}

std::vector<juce::Component*> WelcomeModuleGUI::getParamComps()
{
    return std::vector<juce::Component*>();
}

juce::AffineTransform WelcomeModuleGUI::getTransform()
{
    if (!bigNoseImg.isValid() || logoArea.isEmpty())
        return {};

    const auto target = logoArea.reduced(6.0f, 4.0f);

    const auto scale = juce::jmin(target.getWidth() / bigNoseImg.getWidth(),
                                  target.getHeight() / bigNoseImg.getHeight());
    const auto scaledWidth = bigNoseImg.getWidth() * scale;
    const auto scaledHeight = bigNoseImg.getHeight() * scale;
    const auto translationX = target.getCentreX() - scaledWidth * 0.5f;
    const auto translationY = target.getCentreY() - scaledHeight * 0.5f;

    return juce::AffineTransform::scale(scale).translated(translationX, translationY);
}

void WelcomeModuleGUI::drawContainer(juce::Graphics& g)
{
    auto container = getContainerArea().toFloat();
    g.setColour(BiztortionTheme::surface);
    g.fillRoundedRectangle(container, 7.0f);
    g.setColour(BiztortionTheme::border);
    g.drawRoundedRectangle(container, 7.0f, 1.0f);
}
