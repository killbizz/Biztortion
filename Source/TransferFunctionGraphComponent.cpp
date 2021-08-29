/*
  ==============================================================================

    TransferFunctionGraphComponent.cpp
    Created: 28 Aug 2021 6:53:09pm
    Author:  gabri

  ==============================================================================
*/

#include "TransferFunctionGraphComponent.h"
#include "PluginProcessor.h"

TransferFunctionGraphComponent::TransferFunctionGraphComponent(BiztortionAudioProcessor& p)
	: audioProcessor(p)
{

	updateParams();

	const auto& params = audioProcessor.getParameters();
	for (auto param : params) {
		if (param->getLabel() == juce::String("Waveshaper")) {
			param->addListener(this);
			// std::cout << param->getName(100);
		}
	}
	startTimerHz(60);
}

TransferFunctionGraphComponent::~TransferFunctionGraphComponent()
{
	const auto& params = audioProcessor.getParameters();
	for (auto param : params) {
		if (param->getLabel() == juce::String("Waveshaper")) {
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
	}
	repaint();
}

void TransferFunctionGraphComponent::paint(juce::Graphics& g)
{
	auto width = static_cast<float> (getWidth());
	auto height = static_cast<float> (getHeight());

	g.fillAll(juce::Colours::darkslategrey.withMultipliedBrightness(.4f));

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

		// clip to -1...1
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

	t.applyTransform(juce::AffineTransform::scale(80.0f, 80.0f));
	t.applyTransform(juce::AffineTransform::translation(width / 2.f, height / 2.f));
	t.applyTransform(juce::AffineTransform::verticalFlip(height));

	g.setColour(juce::Colours::white);
	g.strokePath(t, juce::PathStrokeType(1.0f));
}

void TransferFunctionGraphComponent::updateParams()
{
	WaveshaperSettings settings = WaveshaperModule::getSettings(audioProcessor.apvts);
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
