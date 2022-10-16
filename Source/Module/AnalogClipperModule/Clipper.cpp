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

#include <fstream>

using juce_math = juce::dsp::FastMathApproximations;

Clipper::Clipper()
{
	diode.alpha = 1/(2*23e-3);         //23mV
	diode.beta = 2.52e-9;              //2.52nA
	
	Rin = 1e3;                          //1kOhm
	C = 100e-9;                         //100nF
	
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
	/*
	for(int i=0;i<200;i++)
		buffer[i] *= i/200.0f;
	*/
	
	for(int sample=0;sample<size;sample++)
		buffer[sample] = capacitorVoltage(buffer[sample]);
}

float Clipper::capacitorVoltage(float vIn)
{
	float vDiodes = lastFPOutput;
	float oldVDiodes = vDiodes+1;
	
	unsigned int iteration = 0;
	while(std::abs(vDiodes-oldVDiodes) > THRESHOLD && iteration < MAX_ITERATIONS)
	{
		oldVDiodes = vDiodes;
		
		//vDiodes = oldVDiodes-diodesFunction(vIn, oldVDiodes, lastFPOutput)/diodesFunctionDerivative(vIn, oldVDiodes, lastFPOutput);
		vDiodes = vDiodes-(vDiodes-diodesFunction(vIn, oldVDiodes, lastFPOutput))/(1-diodesFunctionDerivative(vIn, oldVDiodes, lastFPOutput));

		iteration++;
	}
	
	lastFPOutput = vDiodes;
	
	//if(vDiodes > 0 || std::isnan(vDiodes)) { std::ofstream f("/Users/francesco/Desktop/output.txt", std::ios::app); f << vDiodes << std::endl; }
	
	return vDiodes;
}

float Clipper::diodesFunction(float vIn, float vDiodes, float oldVDiodes)
{
	float f = (vIn-vDiodes)/Rin-2*diode.beta*juce_math::sinh(diode.alpha*vDiodes);
	float ret = T * f / C + oldVDiodes;
	
	return ret;
}

float Clipper::diodesFunctionDerivative(float vIn, float vDiodes, float oldVDiodes)
{
	float f = 2*diode.alpha*diode.beta*juce_math::cosh(diode.alpha*vDiodes)+1/Rin;
	float ret = - T * f / C;

	if(ret == 0) { std::ofstream f("/Users/francesco/Desktop/output.txt", std::ios::app); f << vDiodes << std::endl; }
	
	return ret;
}
