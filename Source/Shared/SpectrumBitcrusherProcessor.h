/*
  ==============================================================================

    SpectrumBitcrusherProcessor.h

    Copyright (c) 2022 KillBizz - Gabriel Bizzo

  ==============================================================================
*/

#pragma once

#include "OverlappingFFTProcessor.h"
#include "FFTAnalyzer.h"

class SpectrumBitcrusherProcessor : public OverlappingFFTProcessor
{
public:
    SpectrumBitcrusherProcessor();
    ~SpectrumBitcrusherProcessor();

private:
    void processFrameInBuffer(const int maxNumChannels) override;
};