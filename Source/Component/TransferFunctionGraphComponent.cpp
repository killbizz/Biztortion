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
#include "../PluginProcessor.h"

TransferFunctionGraphComponent::TransferFunctionGraphComponent(BiztortionAudioProcessor& p, unsigned int chainPosition)
	: audioProcessor(p), chainPosition(chainPosition)
{
	updateParams();

	const auto& params = audioProcessor.getParameters();
	for (auto param : params) {
		if (param->getLabel() == juce::String("Waveshaper " + std::to_string(chainPosition))) {
			param->addListener(this);
		}
	}
	startTimerHz(60);
}

TransferFunctionGraphComponent::~TransferFunctionGraphComponent()
{
	const auto& params = audioProcessor.getParameters();
	for (auto param : params) {
		if (param->getLabel() == juce::String("Waveshaper " + std::to_string(chainPosition))) {
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
	auto width = static_cast<float> (getWidth());
	auto height = static_cast<float> (getHeight());

	//g.fillAll(juce::Colours::darkslategrey.withMultipliedBrightness(.4f));
	g.fillAll(juce::Colours::black);

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

	t.applyTransform(juce::AffineTransform::scale(48, 138.0f));
	t.applyTransform(juce::AffineTransform::translation(width / 2.f, height / 2.f));
	t.applyTransform(juce::AffineTransform::verticalFlip(height));

	g.setColour(juce::Colours::white);
	g.strokePath(t, juce::PathStrokeType(1.0f));
}

void TransferFunctionGraphComponent::updateParams()
{
	WaveshaperSettings settings = WaveshaperModuleDSP::getSettings(audioProcessor.apvts, chainPosition);
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
