/*
  ==============================================================================

    FFTAnalyzer.cpp

    Copyright (c) 2021 KillBizz - Gabriel Bizzo

  ==============================================================================
*/

/*
  ==============================================================================

    Copyright (c) 2021 Charles Schiermeyer
    Content: PathProducer
    Source: https://github.com/matkatmusic/SimpleEQ

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

#include "FFTAnalyzer.h"

PathProducer::PathProducer(SingleChannelSampleFifo<juce::AudioBuffer<float>>& scsf) :
    leftChannelFifo(&scsf)
{
    leftChannelFFTDataGenerator.changeOrder(FFTOrder::order4096);
    monoBuffer.setSize(1, leftChannelFFTDataGenerator.getFFTSize());
}

void PathProducer::process(juce::Rectangle<float> fftBounds, double sampleRate)
{
    juce::AudioBuffer<float> tempIncomingBuffer;
    while (leftChannelFifo->getNumCompleteBuffersAvailable() > 0) {
        if (leftChannelFifo->getAudioBuffer(tempIncomingBuffer)) {
            // notice that with these operations monoBuffer never changes size
            auto size = tempIncomingBuffer.getNumSamples();
            // left shifting of audio data in the buffer (losting the first block)
            juce::FloatVectorOperations::copy(
                monoBuffer.getWritePointer(0, 0),
                monoBuffer.getReadPointer(0, size - 1),
                monoBuffer.getNumSamples() - size);
            // appending fresh audio data block in the buffer
            juce::FloatVectorOperations::copy(
                monoBuffer.getWritePointer(0, monoBuffer.getNumSamples() - size),
                tempIncomingBuffer.getReadPointer(0, 0),
                size);
            // negativeInfinity = minimum magnitude value to display in the analyzer
            leftChannelFFTDataGenerator.produceFFTDataForRendering(monoBuffer, -48.f);
        }
    }
    /*
    * if there are FFT data buffers to pull
    *   uf we can pull a buffer
    *       generate a path
    */
    const auto fftSize = leftChannelFFTDataGenerator.getFFTSize();
    // binWidth = sampleRate / fftSize
    const auto binWidth = sampleRate / (double)fftSize;
    while (leftChannelFFTDataGenerator.getNumAvailableFFTDataBlocks() > 0) {
        std::vector<float> fftData;
        if (leftChannelFFTDataGenerator.getFFTData(fftData)) {
            pathProducer.generatePath(fftData, fftBounds, fftSize, binWidth, -48.f);
        }
    }
    /*
    * while there are paths that can be pulled
    *   pull as many as we can
    *       display the most recent path
    */
    while (pathProducer.getNumPathsAvailable() > 0) {
        pathProducer.getPath(leftChannelFFTPath);
    }
}

void PathProducer::setSingleChannelSampleFifo(SingleChannelSampleFifo<juce::AudioBuffer<float>>* scsf)
{
    leftChannelFifo = scsf;
}
