/*
  ==============================================================================

    FFTAnalyzer.h

    Copyright (c) 2021 KillBizz - Gabriel Bizzo

  ==============================================================================
*/

/*
  ==============================================================================

    Copyright (c) 2021 Charles Schiermeyer
    Content: Fifo, SingleChannelSampleFifo, FFTAnalyzerDataGenerator, AnalyzerPathGenerator; PathProducer
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

#pragma once

#include <JuceHeader.h>
#include <array>

enum Channel {
    Right, // 0
    Left // 1
};

// retrieves fixed amount audio samples blocks
template<typename T>
struct Fifo
{
    void prepare(int numChannels, int numSamples)
    {
        static_assert(std::is_same_v<T, juce::AudioBuffer<float>>,
            "prepare(numChannels, numSamples) should only be used when the Fifo is holding juce::AudioBuffer<float>");
        for (auto& buffer : buffers)
        {
            buffer.setSize(numChannels,
                numSamples,
                false,   //clear everything?
                true,    //including the extra space?
                true);   //avoid reallocating if you can?
            buffer.clear();
        }
    }

    void prepare(size_t numElements)
    {
        static_assert(std::is_same_v<T, std::vector<float>>,
            "prepare(numElements) should only be used when the Fifo is holding std::vector<float>");
        for (auto& buffer : buffers)
        {
            buffer.clear();
            buffer.resize(numElements, 0);
        }
    }

    bool push(const T& t)
    {
        auto write = fifo.write(1);
        if (write.blockSize1 > 0)
        {
            buffers[write.startIndex1] = t;
            return true;
        }

        return false;
    }

    bool pull(T& t)
    {
        auto read = fifo.read(1);
        if (read.blockSize1 > 0)
        {
            t = buffers[read.startIndex1];
            return true;
        }

        return false;
    }

    int getNumAvailableForReading() const
    {
        return fifo.getNumReady();
    }
private:
    static constexpr int Capacity = 30;
    std::array<T, Capacity> buffers;
    juce::AbstractFifo fifo{ Capacity };
};

// produces single-channel blocks of fixed amount audio samples
template<typename BlockType>
struct SingleChannelSampleFifo
{
    SingleChannelSampleFifo(Channel ch) : channelToUse(ch)
    {
        prepared.set(false);
    }

    void update(const BlockType& buffer)
    {
        jassert(prepared.get());
        jassert(buffer.getNumChannels() > channelToUse);
        auto* channelPtr = buffer.getReadPointer(channelToUse);

        for (int i = 0; i < buffer.getNumSamples(); ++i)
        {
            pushNextSampleIntoFifo(channelPtr[i]);
        }
    }

    void prepare(int bufferSize)
    {
        prepared.set(false);
        size.set(bufferSize);

        bufferToFill.setSize(
            1,             //channel
            bufferSize,    //num samples
            false,         //keepExistingContent
            true,          //clear extra space
            true);         //avoid reallocating
        audioBufferFifo.prepare(1, bufferSize);
        fifoIndex = 0;
        prepared.set(true);
    }
    //==============================================================================
    int getNumCompleteBuffersAvailable() const { return audioBufferFifo.getNumAvailableForReading(); }
    bool isPrepared() const { return prepared.get(); }
    int getSize() const { return size.get(); }
    //==============================================================================
    bool getAudioBuffer(BlockType& buf) { return audioBufferFifo.pull(buf); }
private:
    Channel channelToUse;
    int fifoIndex = 0;
    Fifo<BlockType> audioBufferFifo;
    BlockType bufferToFill;
    juce::Atomic<bool> prepared = false;
    juce::Atomic<int> size = 0;

    void pushNextSampleIntoFifo(float sample)
    {
        if (fifoIndex == bufferToFill.getNumSamples())
        {
            auto ok = audioBufferFifo.push(bufferToFill);

            juce::ignoreUnused(ok);

            fifoIndex = 0;
        }

        bufferToFill.setSample(0, fifoIndex, sample);
        ++fifoIndex;
    }
};

enum FFTOrder
{
    order2048 = 11,
    order4096 = 12,
    order8192 = 13
};

template<typename BlockType>
struct FFTAnalyzerDataGenerator
{
    /**
     produces the FFT data from an audio buffer to visualize its spectrum.
     */
    void produceFFTDataForRendering(const juce::AudioBuffer<float>& audioData, const float negativeInfinity)
    {
        const auto fftSize = getFFTSize();

        fftData.assign(fftData.size(), 0);
        auto* readIndex = audioData.getReadPointer(0);
        std::copy(readIndex, readIndex + fftSize, fftData.begin());

        // first apply a windowing function to our data
        window->multiplyWithWindowingTable(fftData.data(), fftSize);       // [1]

        // then render our FFT data..
        forwardFFT->performFrequencyOnlyForwardTransform(fftData.data());  // [2]

        int numBins = (int)fftSize / 2;

        // normalize the fft values.
        for (int i = 0; i < numBins; ++i)
        {
            auto v = fftData[i];
            // fftData[i] /= (float) numBins;
            if (!std::isinf(v) && !std::isnan(v))
            {
                v /= float(numBins);
            }
            else
            {
                v = 0.f;
            }
            fftData[i] = v;
        }

        //convert them to decibels
        for (int i = 0; i < numBins; ++i)
        {
            fftData[i] = juce::Decibels::gainToDecibels(fftData[i], negativeInfinity);
        }

        fftDataFifo.push(fftData);
    }

    void changeOrder(FFTOrder newOrder)
    {
        //when you change order, recreate the window, forwardFFT, fifo, fftData
        //also reset the fifoIndex
        //things that need recreating should be created on the heap via std::make_unique<>

        order = newOrder;
        auto fftSize = getFFTSize();

        forwardFFT = std::make_unique<juce::dsp::FFT>(order);
        window = std::make_unique<juce::dsp::WindowingFunction<float>>(fftSize, juce::dsp::WindowingFunction<float>::blackmanHarris);

        fftData.clear();
        fftData.resize(fftSize * 2, 0);

        fftDataFifo.prepare(fftData.size());
    }
    //==============================================================================
    int getFFTSize() const { return 1 << order; }
    int getNumAvailableFFTDataBlocks() const { return fftDataFifo.getNumAvailableForReading(); }
    //==============================================================================
    bool getFFTData(BlockType& fftData) { return fftDataFifo.pull(fftData); }
private:
    FFTOrder order;
    BlockType fftData;
    std::unique_ptr<juce::dsp::FFT> forwardFFT;
    std::unique_ptr<juce::dsp::WindowingFunction<float>> window;

    Fifo<BlockType> fftDataFifo;
};

template<typename PathType>
struct AnalyzerPathGenerator
{
    /*
     converts 'renderData[]' into a juce::Path
     */
    void generatePath(const std::vector<float>& renderData,
        juce::Rectangle<float> fftBounds,
        int fftSize,
        float binWidth,
        float negativeInfinity)
    {
        auto top = fftBounds.getY();
        auto bottom = fftBounds.getHeight();
        auto width = fftBounds.getWidth();

        int numBins = (int)fftSize / 2;

        PathType p;
        p.preallocateSpace(3 * (int)fftBounds.getWidth());

        auto map = [bottom, top, negativeInfinity](float v)
        {
            return juce::jmap(v,
                negativeInfinity, 0.f,
                float(bottom + 10), top);
        };

        auto y = map(renderData[0]);

        //        jassert( !std::isnan(y) && !std::isinf(y) );
        if (std::isnan(y) || std::isinf(y))
            y = bottom;

        p.startNewSubPath(0, y);

        const int pathResolution = 2; //you can draw line-to's every 'pathResolution' pixels.

        for (int binNum = 1; binNum < numBins; binNum += pathResolution)
        {
            y = map(renderData[binNum]);

            //            jassert( !std::isnan(y) && !std::isinf(y) );

            if (!std::isnan(y) && !std::isinf(y))
            {
                auto binFreq = binNum * binWidth;
                auto normalizedBinX = juce::mapFromLog10(binFreq, 20.f, 20000.f);
                int binX = std::floor(normalizedBinX * width);
                p.lineTo(binX, y);
            }
        }

        pathFifo.push(p);
    }

    int getNumPathsAvailable() const
    {
        return pathFifo.getNumAvailableForReading();
    }

    bool getPath(PathType& path)
    {
        return pathFifo.pull(path);
    }
private:
    Fifo<PathType> pathFifo;
};

// class for a single channel fft analyzer path producer
struct PathProducer
{
    PathProducer(SingleChannelSampleFifo<juce::AudioBuffer<float>>& scsf);
    void process(juce::Rectangle<float> fftBounds, double sampleRate);
    juce::Path getPath() { return leftChannelFFTPath; }
    void setSingleChannelSampleFifo(SingleChannelSampleFifo<juce::AudioBuffer<float>>* scsf);
private:
    SingleChannelSampleFifo<juce::AudioBuffer<float>>* leftChannelFifo;

    juce::AudioBuffer<float> monoBuffer;

    FFTAnalyzerDataGenerator<std::vector<float>> leftChannelFFTDataGenerator;

    AnalyzerPathGenerator<juce::Path> pathProducer;

    juce::Path leftChannelFFTPath;
};

template<typename BlockType>
struct FFTDataGenerator {

    /**
     produces the FFT data from an audio buffer to elaborate its spectrum.
     */
    void produceFFTDataForSpectrumElaboration(juce::AudioBuffer<float>& audioData)
    {
        auto fftSize = getFFTSize();
        auto numSamples = audioData.getNumSamples();
        auto hopSize = fftSize / OVERLAP_FACTOR;

        jassert(numSamples == fftSize);

        std::vector<std::complex<float>> timeData;
        timeData.resize(fftSize);
        std::vector<std::complex<float>> spectrumData;
        spectrumData.resize(fftSize);

        fftData.assign(fftData.size(), 0);
        auto* writeIndex = audioData.getWritePointer(0);
        auto* readIndex = latestAudioData.getReadPointer(0);
        
        // applying a windowing function to our data
        window->multiplyWithWindowingTable(writeIndex, fftSize);

        for (int i = 0; i < numSamples; i++){
            float x = writeIndex[i];
            // x = (i < hopSize ? x + readIndex[fftSize - hopSize + i] : x);
            timeData[i] = std::complex<float>(x, 0.f);
        }

        // rendering the FFT data
        forwardFFT->perform(timeData.data(), spectrumData.data(), false);

        int numBins = (int)fftSize / 2;

        // normalizing the fft values
        for (int i = 0; i < fftSize; ++i)
        {
            auto magnitude = std::abs(spectrumData[i]);
            if (!std::isinf(magnitude) && !std::isnan(magnitude)) {
                magnitude /= float(numBins);
            } else {
                magnitude = 0.f;
            }
            fftData[i] = magnitude;
        }

        /*latestAudioData.clear();
        latestAudioData.addFrom(0, 0, writeIndex, fftSize);*/

        fftDataFifo.push(fftData);
    }

    void prepare(FFTOrder newOrder)
    {
        //when you change order, recreate the window, forwardFFT, fifo, fftData
        //also reset the fifoIndex
        //things that need recreating should be created on the heap via std::make_unique<>

        order = newOrder;
        auto fftSize = getFFTSize();

        forwardFFT = std::make_unique<juce::dsp::FFT>(order);
        window = std::make_unique<juce::dsp::WindowingFunction<float>>(fftSize, juce::dsp::WindowingFunction<float>::hamming);

        fftData.clear();
        fftData.resize(fftSize, 0);

        latestAudioData.clear();
        latestAudioData.setSize(1, fftSize, false, true, true);

        fftDataFifo.prepare(fftData.size());
    }
    //==============================================================================
    int getFFTSize() const { return 1 << order; }
    int getNumAvailableFFTDataBlocks() const { return fftDataFifo.getNumAvailableForReading(); }
    //==============================================================================
    bool getFFTData(BlockType& fftData) { return fftDataFifo.pull(fftData); }
private:

    const int OVERLAP_FACTOR = 4;

    FFTOrder order;
    BlockType fftData;
    
    juce::AudioBuffer<float> latestAudioData;

    std::unique_ptr<juce::dsp::FFT> forwardFFT;
    std::unique_ptr<juce::dsp::WindowingFunction<float>> window;

    Fifo<BlockType> fftDataFifo;
};

template<typename BlockType>
struct AudioDataGenerator
{
    /**
     produces the audio data from an fft data buffer (frequency domain -> time domain)
     */
    void produceAudioDataFromFFTData(const BlockType& fftData)
    {
        const auto fftSize = getFFTSize();
        auto numSamples = fftData.size();

        std::vector<std::complex<float>> timeData;
        timeData.resize(fftSize);
        std::vector<std::complex<float>> spectrumData;
        spectrumData.resize(fftSize);

        audioData.clear();
        auto* readIndex = fftData.data();

        for (int i = 0; i < numSamples; i++) {
            spectrumData[i] = std::complex<float>(readIndex[i], 0.f);
        }

        // rendering the FFT data
        reverseFFT->perform(spectrumData.data(), timeData.data(), true);

        float* data = audioData.getWritePointer(0);

        for (int i = 0; i < fftSize; ++i)
        {
            data[i] = timeData[i].real();
        }

        // splicing audioData in multiple audio buffers
        monoChannelSampleFifo.update(audioData);
    }

    void prepare(int samplesPerBlock, FFTOrder newOrder)
    {
        //when you change order, recreate the window, forwardFFT, fifo, fftData
        //also reset the fifoIndex
        //things that need recreating should be created on the heap via std::make_unique<>

        order = newOrder;
        auto fftSize = getFFTSize();

        reverseFFT = std::make_unique<juce::dsp::FFT>(order);

        audioData.clear();
        audioData.setSize(1, fftSize, false, true, true);

        monoChannelSampleFifo.prepare(samplesPerBlock);
    }
    //==============================================================================
    int getFFTSize() const { return 1 << order; }
    int getNumAvailablAudioDataBlocks() const { return monoChannelSampleFifo.getNumCompleteBuffersAvailable(); }
    //==============================================================================
    bool getAudioData(juce::AudioBuffer<float>& audioData) { return monoChannelSampleFifo.getAudioBuffer(audioData); }
private:
    FFTOrder order;
    juce::AudioBuffer<float> audioData;
    std::unique_ptr<juce::dsp::FFT> reverseFFT;

    SingleChannelSampleFifo<juce::AudioBuffer<float>> monoChannelSampleFifo{ Channel::Right };
};