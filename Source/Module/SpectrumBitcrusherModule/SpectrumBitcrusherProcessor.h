/*
  ==============================================================================

    SpectrumBitcrusherProcessor.h

    Copyright (c) 2022 KillBizz - Gabriel Bizzo

  ==============================================================================
*/

#pragma once

#include "../../Shared/OverlappingFFTProcessor.h"
#include "../../Shared/FFTAnalyzer.h"

class SpectrumBitcrusherProcessor : public OverlappingFFTProcessor
{
public:
    SpectrumBitcrusherProcessor();
    ~SpectrumBitcrusherProcessor();

  int getFFTSize() const noexcept { return fftSize; }
  int getHopSize() const noexcept { return hopSize; }

private:
    void processFrameInBuffer(const int maxNumChannels) override;
};