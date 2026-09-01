/*
  ==============================================================================

	TransferFunctionGraphComponent.cpp

	Copyright (c) 2021 KillBizz - Gabriel Bizzo

  ==============================================================================
*/

/*
  ==============================================================================

	Copyright (c) 2018 Daniele Filaretti
	Content: the Transfer Function Graph Component
	Source: https://github.com/dfilaretti/waveshaper-demo

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

#include "TransferFunctionGraphComponent.h"

#include "../Shared/GUIStuff.h"
#include "../Module/WaveshaperModule/WaveshaperModule.h"

TransferFunctionGraphComponent::TransferFunctionGraphComponent(PluginState& p, unsigned int parameterNumber)
	: pluginState(p), parameterNumber(parameterNumber)
{
	updateParams();

	const auto& params = pluginState.audioProcessor.getParameters();
	for (auto param : params) {
		if (param->getLabel() == juce::String("Waveshaper " + std::to_string(parameterNumber))) {
			param->addListener(this);
		}
	}
	startTimerHz(60);
}

TransferFunctionGraphComponent::~TransferFunctionGraphComponent()
{
	const auto& params = pluginState.audioProcessor.getParameters();
	for (auto param : params) {
		if (param->getLabel() == juce::String("Waveshaper " + std::to_string(parameterNumber))) {
			param->removeListener(this);
		}
	}
}

void TransferFunctionGraphComponent::parameterValueChanged(int parameterIndex, float newValue)
{
	parameterChanged.set(true);
}

void TransferFunctionGraphComponent::timerCallback()
{
	if (parameterChanged.compareAndSetBool(false, true)) {
		updateParams();
		repaint();
	}
}

void TransferFunctionGraphComponent::paint(juce::Graphics& g)
{
	using namespace juce;

	g.fillAll(BiztortionTheme::canvas);

	const auto bounds = getLocalBounds().toFloat();

	constexpr int fontHeight = 10;
	const Font valueFont("Segoe UI", (float) fontHeight, Font::bold);
	const Font axisFont("Segoe UI", (float) fontHeight + 1.0f, Font::bold);

	// Reserve a margin on each side for the axis labels, then draw the curve only inside the
	// remaining plot rectangle. Everything is derived from the component bounds (the previous
	// version mixed getBounds() with hard-coded offsets, which clipped the top/bottom labels).
	const auto valueLabelWidth = (float) GlyphArrangement::getStringWidthInt(valueFont, "-1") + 6.0f;
	auto plot = bounds;
	plot.removeFromLeft(valueLabelWidth + 4.0f);
	plot.removeFromRight(6.0f);
	plot.removeFromTop((float) fontHeight * 0.5f + 4.0f);
	plot.removeFromBottom((float) fontHeight + 6.0f);

	if (plot.getWidth() <= 2.0f || plot.getHeight() <= 2.0f)
		return;

	const auto inputToX = [&plot](float input) {
		return jmap(input, -1.0f, 1.0f, plot.getX(), plot.getRight());
	};
	const auto outputToY = [&plot](float output) {
		return jmap(output, -1.0f, 1.0f, plot.getBottom(), plot.getY());
	};

	// --- grid ---
	g.setColour(BiztortionTheme::border.withAlpha(0.45f));
	for (const auto value : { -0.5f, 0.5f })
	{
		g.drawHorizontalLine(roundToInt(outputToY(value)), plot.getX(), plot.getRight());
		g.drawVerticalLine(roundToInt(inputToX(value)), plot.getY(), plot.getBottom());
	}

	// --- axes ---
	g.setColour(BiztortionTheme::border.brighter(0.15f));
	g.drawHorizontalLine(roundToInt(outputToY(0.0f)), plot.getX(), plot.getRight());
	g.drawVerticalLine(roundToInt(inputToX(0.0f)), plot.getY(), plot.getBottom());

	// --- transfer function ---
	constexpr int resolution = 512;
	Path curve;

	for (int i = 0; i < resolution; ++i)
	{
		const auto proportion = (float) i / (float) (resolution - 1);
		const auto input = jmap(proportion, -1.0f, 1.0f);
		const auto output = jlimit(-1.0f, 1.0f,
			tanhAmplitude * std::tanh(input * tanhSlope)
			+ sineAmplitude * std::sin(input * sineFrequency));

		const auto point = Point<float>(inputToX(input), outputToY(output));

		if (i == 0)
			curve.startNewSubPath(point);
		else
			curve.lineTo(point);
	}

	g.setColour(BiztortionTheme::text);
	g.strokePath(curve, PathStrokeType(1.4f, PathStrokeType::curved, PathStrokeType::rounded));

	// --- axis value labels, laid out inside the reserved margins ---
	const auto drawLabel = [&g](const String& text, Rectangle<float> area) {
		g.drawFittedText(text, area.getSmallestIntegerContainer(), Justification::centred, 1);
	};

	g.setFont(valueFont);
	g.setColour(BiztortionTheme::textMuted);

	// input (horizontal) values, in the bottom margin
	const auto inputLabelArea = Rectangle<float>(0.0f, plot.getBottom() + 2.0f,
		valueLabelWidth + 8.0f, (float) fontHeight + 2.0f);
	drawLabel("-1", inputLabelArea.withCentre({ inputToX(-1.0f), inputLabelArea.getCentreY() }));
	drawLabel("0", inputLabelArea.withCentre({ inputToX(0.0f), inputLabelArea.getCentreY() }));
	drawLabel("+1", inputLabelArea.withCentre({ inputToX(1.0f), inputLabelArea.getCentreY() }));

	// output (vertical) values, in the left margin
	const auto outputLabelArea = Rectangle<float>(0.0f, 0.0f,
		valueLabelWidth, (float) fontHeight + 2.0f);
	const auto outputLabelX = valueLabelWidth * 0.5f;
	drawLabel("+1", outputLabelArea.withCentre({ outputLabelX, outputToY(1.0f) }));
	drawLabel("0", outputLabelArea.withCentre({ outputLabelX, outputToY(0.0f) }));
	drawLabel("-1", outputLabelArea.withCentre({ outputLabelX, outputToY(-1.0f) }));

	// --- axis captions, kept inside the plot corners the curve normally leaves free ---
	g.setFont(axisFont);
	g.setColour(BiztortionTheme::textMuted.withAlpha(0.75f));

	const auto captionHeight = (float) fontHeight + 2.0f;
	g.drawText("input", plot.removeFromBottom(captionHeight).reduced(4.0f, 0.0f)
		.getSmallestIntegerContainer(), Justification::centredRight, false);
	g.drawText("output", plot.removeFromTop(captionHeight).reduced(4.0f, 0.0f)
		.getSmallestIntegerContainer(), Justification::centredLeft, false);
}

void TransferFunctionGraphComponent::updateParams()
{
	WaveshaperSettings settings = WaveshaperModuleDSP::getSettings(pluginState.apvts, parameterNumber);
	setTanhAmp(settings.tanhAmp);
	setTanhSlope(settings.tanhSlope);
	setSineAmp(settings.sinAmp);
	setSineFreq(settings.sinFreq);
}

void TransferFunctionGraphComponent::setTanhAmp(float v)
{
	tanhAmplitude = v / 100.f;
}

void TransferFunctionGraphComponent::setTanhSlope(float v)
{
	tanhSlope = v;
}

void TransferFunctionGraphComponent::setSineAmp(float v)
{
	sineAmplitude = v / 100.f;
}

void TransferFunctionGraphComponent::setSineFreq(float v)
{
	sineFrequency = v;
}
