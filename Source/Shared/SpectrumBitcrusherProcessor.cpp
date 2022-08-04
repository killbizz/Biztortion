/*
  ==============================================================================

    SpectrumBitcrusherProcessor.cpp

    Copyright (c) 2022 KillBizz - Gabriel Bizzo

  ==============================================================================
*/

#include "SpectrumBitcrusherProcessor.h"

void SpectrumBitcrusherProcessor::processFrameInBuffer(const int maxNumChannels)
{
    // FFT
    for (int ch = 0; ch < maxNumChannels; ++ch)
        fft.performRealOnlyForwardTransform(fftInOutBuffer.getWritePointer(ch), true);

    // clear high frequency content
    for (int ch = 0; ch < maxNumChannels; ++ch)
        FloatVectorOperations::clear(fftInOutBuffer.getWritePointer(ch, fftSize / 2), fftSize / 2);

    // SPECTRUM DATA ELABORATION
    // ...

    // IFFT
    for (int ch = 0; ch < maxNumChannels; ++ch)
        fft.performRealOnlyInverseTransform(fftInOutBuffer.getWritePointer(ch));
}
