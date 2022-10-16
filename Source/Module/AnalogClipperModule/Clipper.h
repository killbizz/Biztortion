//
//  Clipper.hpp
//  clipper - VST3
//
//  Created by Francesco Magoga
//

#ifndef Clipper_hpp
#define Clipper_hpp

#define MAX_L_VALUE 50.0

#include <vector>

class Diode
{
	public:
		float alpha;
		float beta;
};

class Clipper
{
	public:
		Clipper();
		~Clipper();
	
		void setSampleRate(unsigned int sampleRate);
		
		void process(float *buffer, size_t size);
		
	private:
		Diode diode;
		
		float Rin, C, T, lastFPOutput;
	
		float capacitorVoltage(float vIn);
		float diodesFunction(float vIn, float vDiodes, float oldVDiodes);
		float diodesFunctionDerivative(float vIn, float vDiodes, float oldVDiodes);
	
		unsigned int sampleRate;
	
		float inputGain;
};

#endif /* Clipper_hpp */
