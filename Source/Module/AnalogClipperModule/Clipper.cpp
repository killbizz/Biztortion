//
//  Clipper.cpp
//  clipper - VST3
//
//  Created by Francesco Magoga
//

#include "Clipper.h"

#include <cmath>

#include <unistd.h>

#include <juce_dsp/maths/juce_FastMathApproximations.h>

#define THRESHOLD 1e-4				//in Volts
#define MAX_ITERATIONS 250

using juce_math = juce::dsp::FastMathApproximations;

Clipper::Clipper()
{
	diode.alpha = 1/(2*23e-3);         //23mV
	diode.beta = 2.52e-9;              //2.52nA
	
	Rin = 1e3;                          //1kOhm
	C = 100e-9;                         //100nF
		
	h = 1e-5;
	
	lastFPOutput = 0;
	
	inputGain = 0.0f;
}

Clipper::~Clipper()
{
	
}

void Clipper::setSampleRate(unsigned int sampleRate)
{
	this->sampleRate = sampleRate;
	
	T = 1.0f/sampleRate;
}

void Clipper::process(float *buffer, size_t size)
{
	for(int sample=0;sample<size;sample++)
		buffer[sample] = capacitorVoltage(buffer[sample]);
}

float Clipper::capacitorVoltage(float vIn)
{
	if(inputGain < 1.0f)
	{
		vIn *= inputGain;
		
		inputGain += 0.0000001f;
		
		if(inputGain > 0.000001f)
			inputGain = 1.0f;
	}
	
	float vDiodes = fixedPoint(vIn);
	
	return vDiodes;
}

float Clipper::fixedPoint(float vIn)
{
	float vDiodes = lastFPOutput;
	float oldVDiodes = vDiodes+1;
	
	unsigned int iteration = 0;
	while(std::abs(vDiodes-oldVDiodes) > THRESHOLD && iteration < MAX_ITERATIONS)
	{
		oldVDiodes = vDiodes;
		
		vDiodes = vDiodes-diodesFunction(vIn, vDiodes, lastFPOutput)/diodesFunctionDerivative(vIn, vDiodes, lastFPOutput);
		
		iteration++;
	}
	
	lastFPOutput = vDiodes;

	return vDiodes;
}

float Clipper::diodesFunction(float vIn, float vDiodes, float oldVDiodes)
{
	float f = (vIn-vDiodes)/Rin-2*diode.beta*juce_math::sinh(diode.alpha*vDiodes);

	return h * f / C + oldVDiodes;
}

float Clipper::diodesFunctionDerivative(float vIn, float vDiodes, float oldVDiodes)
{
	float f = (vIn-vDiodes)/Rin-2*diode.alpha*diode.beta*juce_math::cosh(diode.alpha*vDiodes);

	return h * f / C + oldVDiodes;
}
