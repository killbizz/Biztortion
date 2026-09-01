/*
  ==============================================================================

    WaveShapeVisualiser.cpp

    Copyright (c) 2022 KillBizz - Gabriel Bizzo

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

#include "WaveShapeVisualiser.h"

#include "../Shared/GUIStuff.h"

WaveShapeVisualiser::WaveShapeVisualiser(juce::Colour accentColour, Shaper shaperToUse,
                                         Style styleToUse)
    : accent(accentColour), shaper(std::move(shaperToUse)), style(styleToUse)
{
    // the graph is not interactive, but it has to be hit-testable for the tooltip window to
    // find it under the mouse (it does not need to catch clicks for any child)
    setInterceptsMouseClicks(true, false);
    // repainting on a timer is what makes the graph follow the knobs while they are dragged
    startTimerHz(20);
}

void WaveShapeVisualiser::setAccentColour(juce::Colour newColour)
{
    accent = newColour;
    repaint();
}

void WaveShapeVisualiser::setStyle(Style newStyle)
{
    style = newStyle;
    repaint();
}

void WaveShapeVisualiser::setCaption(juce::String newCaption)
{
    caption = std::move(newCaption);
    repaint();
}

void WaveShapeVisualiser::timerCallback()
{
    repaint();
}

juce::Path WaveShapeVisualiser::buildPath(const std::vector<float>& samples,
                                          juce::Rectangle<float> area, bool drawSteps) const
{
    juce::Path path;

    if (samples.empty() || area.isEmpty())
        return path;

    const auto toPoint = [&area, &samples](size_t index, float value) {
        const auto proportion = (float) index / (float) juce::jmax<size_t>(1, samples.size() - 1);
        return juce::Point<float>(area.getX() + proportion * area.getWidth(),
            juce::jmap(juce::jlimit(-1.0f, 1.0f, value), -1.0f, 1.0f,
                       area.getBottom(), area.getY()));
    };

    auto previous = toPoint(0, samples[0]);
    path.startNewSubPath(previous);

    for (size_t index = 1; index < samples.size(); ++index)
    {
        const auto point = toPoint(index, samples[index]);

        if (drawSteps)
        {
            // hold the previous level, then jump: the staircase look of a quantised signal
            path.lineTo(point.x, previous.y);
            path.lineTo(point);
        }
        else
        {
            path.lineTo(point);
        }

        previous = point;
    }

    return path;
}

void WaveShapeVisualiser::paintWaveform(juce::Graphics& g, juce::Rectangle<float> plot)
{
    // --- zero line ---
    g.setColour(BiztortionTheme::border.withAlpha(0.7f));
    g.drawHorizontalLine(juce::roundToInt(plot.getCentreY()), plot.getX(), plot.getRight());

    // --- reference cycle ---
    std::vector<float> input((size_t) resolution);
    for (size_t index = 0; index < input.size(); ++index)
    {
        const auto phase = (float) index / (float) (resolution - 1) * cycles
            * juce::MathConstants<float>::twoPi;
        input[index] = std::sin(phase);
    }

    auto output = input;
    if (shaper != nullptr)
        shaper(output);

    // input drawn faintly underneath, as the reference to compare against
    g.setColour(BiztortionTheme::textMuted.withAlpha(0.35f));
    g.strokePath(buildPath(input, plot, false),
        juce::PathStrokeType(1.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    const auto outputPath = buildPath(output, plot, style == Style::steppedWaveform);

    // filled body, so the shape reads at a glance even in a small graph
    auto filled = outputPath;
    filled.lineTo(plot.getRight(), plot.getCentreY());
    filled.lineTo(plot.getX(), plot.getCentreY());
    filled.closeSubPath();

    g.setColour(accent.withAlpha(0.22f));
    g.fillPath(filled);

    g.setColour(accent);
    g.strokePath(outputPath,
        juce::PathStrokeType(1.6f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
}

void WaveShapeVisualiser::paintSpectrumBars(juce::Graphics& g, juce::Rectangle<float> plot)
{
    // baseline: a gently decaying spectrum, so the effect of the module's parameters on it is
    // what the user actually sees changing
    std::vector<float> magnitudes((size_t) barCount);
    for (size_t index = 0; index < magnitudes.size(); ++index)
    {
        const auto proportion = (float) index / (float) (barCount - 1);
        magnitudes[index] = 0.35f + 0.6f * std::exp(-2.4f * proportion)
            + 0.08f * std::sin(proportion * 18.0f);
    }

    if (shaper != nullptr)
        shaper(magnitudes);

    g.setColour(BiztortionTheme::border.withAlpha(0.7f));
    g.drawHorizontalLine(juce::roundToInt(plot.getBottom()), plot.getX(), plot.getRight());

    const auto barWidth = plot.getWidth() / (float) barCount;

    for (size_t index = 0; index < magnitudes.size(); ++index)
    {
        const auto magnitude = juce::jlimit(0.0f, 1.0f, magnitudes[index]);
        const auto height = magnitude * plot.getHeight();
        const auto bar = juce::Rectangle<float>(plot.getX() + barWidth * (float) index,
            plot.getBottom() - height, barWidth, height).reduced(barWidth * 0.18f, 0.0f);

        if (bar.getHeight() < 1.0f)
            continue;

        g.setColour(accent.withAlpha(0.32f));
        g.fillRect(bar);
        g.setColour(accent);
        g.fillRect(bar.withHeight(juce::jmin(2.0f, bar.getHeight())));
    }
}

void WaveShapeVisualiser::paint(juce::Graphics& g)
{
    const auto bounds = getLocalBounds().toFloat();

    if (bounds.getWidth() < 8.0f || bounds.getHeight() < 8.0f)
        return;

    // --- panel ---
    g.setColour(BiztortionTheme::canvas);
    g.fillRoundedRectangle(bounds, 6.0f);
    g.setColour(BiztortionTheme::border.withAlpha(0.6f));
    g.drawRoundedRectangle(bounds, 6.0f, 1.0f);

    const auto plot = bounds.reduced(6.0f, 8.0f);

    if (style == Style::spectrumBars)
        paintSpectrumBars(g, plot);
    else
        paintWaveform(g, plot);

    if (caption.isNotEmpty())
    {
        g.setColour(BiztortionTheme::textMuted.withAlpha(0.8f));
        g.setFont(juce::Font("Segoe UI", 10.0f, juce::Font::bold));
        g.drawText(caption, bounds.reduced(8.0f, 4.0f).getSmallestIntegerContainer(),
                   juce::Justification::topRight, false);
    }
}
