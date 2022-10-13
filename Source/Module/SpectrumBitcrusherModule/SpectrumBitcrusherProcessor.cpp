/*
  ==============================================================================

    SpectrumBitcrusherProcessor.cpp

    Copyright (c) 2022 KillBizz - Gabriel Bizzo

  ==============================================================================
*/

#include "SpectrumBitcrusherProcessor.h"

SpectrumBitcrusherProcessor::SpectrumBitcrusherProcessor() : OverlappingFFTProcessor(FFTOrder::order2048, 2) {}

SpectrumBitcrusherProcessor::~SpectrumBitcrusherProcessor() {}

void SpectrumBitcrusherProcessor::processFrameInBuffer(const int maxNumChannels)
{
    // FFT
    for (int ch = 0; ch < maxNumChannels; ++ch)
        fft.performRealOnlyForwardTransform(fftInOutBuffer.getWritePointer(ch), true);

    // clear high frequency content
    for (int ch = 0; ch < maxNumChannels; ++ch)
        FloatVectorOperations::clear(fftInOutBuffer.getWritePointer(ch, fftSize / 2), fftSize / 2);

    // SPECTRUM DATA ELABORATION
    spectrumProcessingCallback(fftInOutBuffer);

    // IFFT
    for (int ch = 0; ch < maxNumChannels; ++ch)
        fft.performRealOnlyInverseTransform(fftInOutBuffer.getWritePointer(ch));
}
