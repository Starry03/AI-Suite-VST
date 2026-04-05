#pragma once

#include <juce_dsp/juce_dsp.h>
#include <juce_audio_basics/juce_audio_basics.h>
#include <vector>
#include <array>

/**
 * Real-time FFT spectrum analyzer for audio visualization.
 * Performs FFT analysis on incoming audio and provides magnitude data
 * for frequency spectrum display.
 */
class SpectrumAnalyzer
{
public:
    SpectrumAnalyzer();
    
    /**
     * Prepare the analyzer for processing.
     * @param sampleRate The sample rate in Hz
     */
    void prepare(double sampleRate);
    
    /**
     * Push audio samples into the analyzer.
     * Call this from the UI thread when samples are available.
     * @param samples Pointer to audio samples
     * @param numSamples Number of samples to push
     */
    void pushSamples(const float* samples, int numSamples);
    
    /**
     * Get the magnitude at a specific frequency in dB.
     * @param frequency The frequency in Hz
     * @return Magnitude in dB (range: -60 to 0)
     */
    float getMagnitudeForFrequency(float frequency) const;
    
    /**
     * Check if new FFT data is available.
     */
    bool hasNewData() const { return dataReady; }
    
    /**
     * Mark data as consumed.
     */
    void clearNewDataFlag() { dataReady = false; }
    
    /**
     * Set FFT order (power of 2).
     * Valid range: 10-13 (1024-8192 samples)
     * @param order FFT order (2^order = FFT size)
     */
    void setFFTOrder(int order);
    
    /**
     * Get current FFT order.
     */
    int getFFTOrder() const { return currentFFTOrder; }
    
private:
    // FFT configuration (runtime configurable)
    int currentFFTOrder = 11;  // Default 2^11 = 2048 samples
    int currentFFTSize = 2048;
    int currentScopeSize = 1024;
    
    std::unique_ptr<juce::dsp::FFT> fft;
    std::unique_ptr<juce::dsp::WindowingFunction<float>> window;
    
    std::vector<float> fftData;  // Real + imaginary (size = fftSize * 2)
    std::vector<float> scopeData;  // Magnitude spectrum (size = scopeSize)
    std::vector<float> smoothedData; // Smoothed spectrum (size = scopeSize)
    
    std::vector<float> audioBuffer;
    int bufferWritePosition = 0;
    
    double currentSampleRate = 44100.0;
    float smoothingFactor = 0.8f; // Balanced smoothing
    
    bool dataReady = false;
    
    void performFFT();
    void smoothSpectrum();
    int frequencyToBin(float frequency) const;
};
