/*
  ==============================================================================

    FFTAnalyzer.cpp
    Created: 21 Aug 2021 12:21:45pm
    Author:  gabri

  ==============================================================================
*/

#include "FFTAnalyzer.h"

PathProducer::PathProducer(SingleChannelSampleFifo<juce::AudioBuffer<float>>& scsf) :
    leftChannelFifo(&scsf)
{
    leftChannelFFTDataGenerator.changeOrder(FFTOrder::order2048);
    monoBuffer.setSize(1, leftChannelFFTDataGenerator.getFFTSize());
}

void PathProducer::process(juce::Rectangle<float> fftBounds, double sampleRate)
{
    juce::AudioBuffer<float> tempIncomingBuffer;
    while (leftChannelFifo->getNumCompleteBuffersAvailable() > 0) {
        if (leftChannelFifo->getAudioBuffer(tempIncomingBuffer)) {
            // notice that with these operations monoBuffer never changes size
            auto size = leftChannelFifo->getSize();
            // left shifting of audio data in the buffer (losting the first block)
            juce::FloatVectorOperations::copy(
                monoBuffer.getWritePointer(0, 0),
                monoBuffer.getReadPointer(0, size),
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
