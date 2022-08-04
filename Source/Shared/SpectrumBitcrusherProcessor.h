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
    SpectrumBitcrusherProcessor() : OverlappingFFTProcessor(FFTOrder::order2048, 2)
    {}
    ~SpectrumBitcrusherProcessor() {}

private:
    void processFrameInBuffer(const int maxNumChannels) override;
};