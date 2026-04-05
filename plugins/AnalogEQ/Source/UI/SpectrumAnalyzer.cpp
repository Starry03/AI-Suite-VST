#include "SpectrumAnalyzer.h"
#include <algorithm>
#include <cmath>

SpectrumAnalyzer::SpectrumAnalyzer()
{
    setFFTOrder(11); // Default to 2048 samples
}

void SpectrumAnalyzer::prepare(double sampleRate)
{
    currentSampleRate = sampleRate;
}

void SpectrumAnalyzer::setFFTOrder(int order)
{
    // Clamp to valid range: 10-13 (1024-8192 samples)
    order = juce::jlimit(10, 13, order);
    
    // CRITICAL: Don't skip initialization if pointers are null (first call)
    if (order == currentFFTOrder && fft != nullptr && window != nullptr)
        return; // No change needed
    
    currentFFTOrder = order;
    currentFFTSize = 1 << order;
    currentScopeSize = currentFFTSize / 2;
    
    // Recreate FFT and window with new size
    fft = std::make_unique<juce::dsp::FFT>(order);
    window = std::make_unique<juce::dsp::WindowingFunction<float>>(
        currentFFTSize, 
        juce::dsp::WindowingFunction<float>::hann
    );
    
    // Resize buffers
    fftData.resize(currentFFTSize * 2, 0.0f);
    scopeData.resize(currentScopeSize, 0.0f);
    smoothedData.resize(currentScopeSize, 0.0f);
    audioBuffer.resize(currentFFTSize, 0.0f);
    
    // Reset write position
    bufferWritePosition = 0;
    dataReady = false;
}

void SpectrumAnalyzer::pushSamples(const float* samples, int numSamples)
{
    // Safety check: ensure buffer is allocated
    if (audioBuffer.empty() || currentFFTSize == 0)
        return;
    
    for (int i = 0; i < numSamples; ++i)
    {
        audioBuffer[bufferWritePosition] = samples[i];
        bufferWritePosition++;
        
        // When buffer is full, perform FFT for precise spectrum updates
        if (bufferWritePosition >= currentFFTSize)
        {
            performFFT();
            bufferWritePosition = 0;
        }
    }
}

void SpectrumAnalyzer::performFFT()
{
    // Safety check: ensure FFT objects are initialized
    if (!fft || !window)
        return;
    
    // Safety check: ensure buffers have correct size
    if (fftData.size() < static_cast<size_t>(currentFFTSize * 2) ||
        scopeData.size() < static_cast<size_t>(currentScopeSize) ||
        audioBuffer.size() < static_cast<size_t>(currentFFTSize))
        return;
    
    // Copy audio buffer to FFT data (real part)
    for (int i = 0; i < currentFFTSize; ++i)
    {
        fftData[i] = audioBuffer[i];
        fftData[i + currentFFTSize] = 0.0f; // Imaginary part
    }
    
    // Apply window
    window->multiplyWithWindowingTable(fftData.data(), currentFFTSize);
    
    // Perform FFT
    fft->performFrequencyOnlyForwardTransform(fftData.data());
    
    // Calculate magnitude spectrum in dB
    for (int i = 0; i < currentScopeSize; ++i)
    {
        // Get magnitude
        float magnitude = fftData[i];
        
        // Convert to dB with floor at -60dB
        const float minLevel = 0.0000001f; // -140dB
        magnitude = std::max(magnitude, minLevel);
        
        float db = 20.0f * std::log10(magnitude);
        
        // Clamp to -60dB to 0dB range
        scopeData[i] = juce::jlimit(-60.0f, 0.0f, db);
    }
    
    // Apply smoothing
    smoothSpectrum();
    
    dataReady = true;
}

void SpectrumAnalyzer::smoothSpectrum()
{
    for (int i = 0; i < currentScopeSize; ++i)
    {
        // Exponential moving average
        smoothedData[i] = smoothingFactor * smoothedData[i] + 
                         (1.0f - smoothingFactor) * scopeData[i];
    }
}

int SpectrumAnalyzer::frequencyToBin(float frequency) const
{
    // Convert frequency to FFT bin index
    float binFreq = static_cast<float>(currentSampleRate) / currentFFTSize;
    int bin = static_cast<int>(frequency / binFreq);
    return juce::jlimit(0, currentScopeSize - 1, bin);
}

float SpectrumAnalyzer::getMagnitudeForFrequency(float frequency) const
{
    int bin = frequencyToBin(frequency);
    
    // Linear interpolation between bins for smoother display
    if (bin < currentScopeSize - 1)
    {
        float binFreq = static_cast<float>(currentSampleRate) / currentFFTSize;
        float exactBin = frequency / binFreq;
        float fraction = exactBin - bin;
        
        return smoothedData[bin] * (1.0f - fraction) + 
               smoothedData[bin + 1] * fraction;
    }
    
    return smoothedData[bin];
}
