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
#include "../Module/WaveshaperModule.h"

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

	auto width = static_cast<float> (getWidth());
	auto height = static_cast<float> (getHeight());

	g.fillAll(juce::Colours::black);

	auto bounds = getBounds();
	auto left = bounds.getX();
	auto right = bounds.getRight();
	auto top = bounds.getY();
	auto bottom = bounds.getBottom();

	auto leftPoint = juce::Point<float>(left, bounds.getCentreY());
	auto rightPoint = juce::Point<float>(right, bounds.getCentreY());
	auto topPoint = juce::Point<float>(bounds.getCentreX(), top);
	auto bottomPoint = juce::Point<float>(bounds.getCentreX(), bottom);
	auto hline = juce::Line<float>(leftPoint, rightPoint);
	auto vline = juce::Line<float>(topPoint, bottomPoint);

	// X
	juce::Array<float> x;
	int resolution = 800;

	float min = -2;
	float max = +2;
	float range = max - min;
	float delta = range / resolution;

	float v = min;
	for (int i = 0; i < resolution; i++)
	{
		x.add(v);
		v = v + delta;
	}

	// Y
	juce::Array<float> y;

	for (int i = 0; i < resolution; i++)
	{
		float vNorm;
		float v1 = tanhAmplitude * std::tanh(x[i] * tanhSlope) + sineAmplitude * std::sin(x[i] * sineFrequency);

		// hardclip to -1...1
		if (v1 <= -1)
			vNorm = -1;
		else if (v1 >= 1)
			vNorm = 1;
		else
			vNorm = v1;
		y.add(vNorm);
	}

	juce::Path t;

	t.startNewSubPath(x[0], y[0]);
	for (int i = 1; i < resolution; i++)
		t.lineTo(x[i], y[i]);

	t.applyTransform(juce::AffineTransform::scale(48, 158.f));
	t.applyTransform(juce::AffineTransform::translation(width / 2.f, height / 2.f));
	t.applyTransform(juce::AffineTransform::verticalFlip(height));

	hline.applyTransform(juce::AffineTransform::translation(-25.0468f, -55.1f));
	vline.applyTransform(juce::AffineTransform::translation(-25.1407f, -55.1f));

	// grid lines
	g.setColour(juce::Colours::darkgrey);
	g.drawLine(hline, 1.f);
	g.drawLine(vline, 1.f);

	// transfer function
	g.setColour(juce::Colours::white);
	g.strokePath(t, juce::PathStrokeType(1.0f));

	// labels
	const int fontHeight = 10;
	String str;
	g.setFont(Font(fontHeight, Font::bold));
	Rectangle<int> r;

	// --- INPUT ---
	g.setColour(juce::Colours::darkgrey);
	// -1
	str << "-1";
	auto textWidth = g.getCurrentFont().getStringWidth(str);
	r.setSize(textWidth, fontHeight);
	auto point = leftPoint;
	point.setXY(point.getX() - 18, point.getY() - fontHeight - 38);
	r.setCentre(point.getX(), point.getY());
	g.drawFittedText(str, r, juce::Justification::centred, 1);
	// 0
	str.clear();
	str << "0";
	textWidth = g.getCurrentFont().getStringWidth(str);
	r.setSize(textWidth, fontHeight);
	point = bounds.getCentre().toFloat();
	point.setXY(point.getX() - 18, point.getY() - fontHeight - 38);
	r.setCentre(point.getX(), point.getY());
	g.drawFittedText(str, r, juce::Justification::centred, 1);
	// +1
	str.clear();
	str << "+1";
	textWidth = g.getCurrentFont().getStringWidth(str);
	r.setSize(textWidth, fontHeight);
	point = rightPoint;
	point.setXY(point.getX() - 33, point.getY() - fontHeight - 38);
	r.setCentre(point.getX(), point.getY());
	g.drawFittedText(str, r, juce::Justification::centred, 1);
	// input label
	g.setFont(juce::Font("Courier New", fontHeight+2, Font::bold));
	str.clear();
	str << "input";
	textWidth = g.getCurrentFont().getStringWidth(str);
	r.setSize(textWidth, fontHeight);
	point = rightPoint;
	point.setXY(point.getX() - textWidth - 18, point.getY() - fontHeight - 52);
	r.setCentre(point.getX(), point.getY());
	g.drawFittedText(str, r, juce::Justification::centred, 1);

	// --- OUTPUT ---
	// -1
	str.clear();
	g.setFont(Font(fontHeight, Font::bold));
	str << "-1";
	textWidth = g.getCurrentFont().getStringWidth(str);
	r.setSize(textWidth, fontHeight);
	point = bottomPoint;
	point.setXY(point.getX() - 18, point.getY() - fontHeight - 52);
	r.setCentre(point.getX(), point.getY());
	g.drawFittedText(str, r, juce::Justification::centred, 1);
	// 0 (already drown)
	// +1
	str.clear();
	str << "+1";
	textWidth = g.getCurrentFont().getStringWidth(str);
	r.setSize(textWidth, fontHeight);
	point = topPoint;
	point.setXY(point.getX() - 16, point.getY() - fontHeight - 38);
	r.setCentre(point.getX(), point.getY());
	g.drawFittedText(str, r, juce::Justification::centred, 1);
	// output label
	g.setFont(juce::Font("Courier New", fontHeight+2, Font::bold));
	str.clear();
	str << "output";
	textWidth = g.getCurrentFont().getStringWidth(str);
	r.setSize(textWidth, fontHeight);
	point = topPoint;
	juce::AffineTransform transform;
	g.addTransform(transform.rotated(-1.f * juce::MathConstants<float>::pi / 2.0f));
	point.setXY(point.getX() - textWidth - 122, 
		point.getY() - fontHeight + 58);
	r.setCentre(point.getX(), point.getY());
	g.drawFittedText(str, r, juce::Justification::centred, 1);

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
