#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <array>
#include <limits>
#include <vector>
#include "DSP/FilterChain.h"

//==============================================================================
/**
 * Audio Processor principale per l'equalizzatore.
 */
class AudioPluginAudioProcessor : public juce::AudioProcessor
{
public:
    //==============================================================================
    AudioPluginAudioProcessor();
    ~AudioPluginAudioProcessor() override;

    //==============================================================================
    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

    bool isBusesLayoutSupported(const BusesLayout& layouts) const override;

    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    using AudioProcessor::processBlock;

    //==============================================================================
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    //==============================================================================
    const juce::String getName() const override;

    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;

    //==============================================================================
    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram(int index) override;
    const juce::String getProgramName(int index) override;
    void changeProgramName(int index, const juce::String& newName) override;

    //==============================================================================
    void getStateInformation(juce::MemoryBlock& destData) override;
    void setStateInformation(const void* data, int sizeInBytes) override;

    //==============================================================================
    // Accesso al filter chain
    FilterChain& getFilterChain() { return filterChain; }
    
    // Accesso all'APVTS
    juce::AudioProcessorValueTreeState& getAPVTS() { return apvts; }
    
    // Spectrum analyzer audio data access
    juce::AbstractFifo& getAudioFifo() { return audioFifo; }
    float* getAudioFifoBuffer() { return audioFifoBuffer.data(); }
    static constexpr int getAudioFifoSize() { return audioFifoSize; }
    juce::AbstractFifo& getSidechainAudioFifo() { return sidechainAudioFifo; }
    float* getSidechainAudioFifoBuffer() { return sidechainAudioFifoBuffer.data(); }

    // Meter level access (thread-safe)
    float getMeterLevel() const { return meterLevel.load(); }
    int getCurrentLatencySamplesForUI() const { return currentLatencySamplesForUI.load(); }
    juce::String getCurrentPhaseModeName() const;
    juce::String getCurrentLinearPhaseQualityName() const;
    bool isSidechainEnabledForUI() const { return sidechainEnabledForUI.load(); }

private:
    //==============================================================================
    FilterChain filterChain;
    juce::AudioProcessorValueTreeState apvts;
    
    static constexpr int maxNumFilters = 8;
    std::array<FilterBase*, maxNumFilters> filterInstances;
    std::array<FilterType, maxNumFilters> currentFilterTypes; // Track current types to avoid unnecessary recreation
    
    // Spectrum analyzer audio capture
    static constexpr int audioFifoSize = 8192; // Must be power of 2
    juce::AbstractFifo audioFifo { audioFifoSize };
    std::array<float, audioFifoSize> audioFifoBuffer;
    juce::AbstractFifo sidechainAudioFifo { audioFifoSize };
    std::array<float, audioFifoSize> sidechainAudioFifoBuffer;

    // Meter and gain matching
    std::atomic<float> meterLevel { -60.0f };
    std::atomic<int> currentLatencySamplesForUI { 0 };
    std::atomic<int> currentPhaseModeForUI { 0 };
    std::atomic<int> currentLinearPhaseQualityForUI { 1 };
    std::atomic<bool> sidechainEnabledForUI { false };
    float inputRMS = 0.0f;
    float outputRMS = 0.0f;

    enum class PhaseMode
    {
        minimum = 0,
        natural,
        linear
    };

    enum class LinearPhaseQuality
    {
        low = 0,
        mid,
        high
    };

    static constexpr int maxNaturalPhaseDelaySamples = 512;
    static constexpr int naturalPhaseLatencySamples = 64;
    std::array<std::vector<float>, 2> naturalPhaseDelayLines;
    int naturalPhaseWritePos = 0;

    static constexpr int maxLinearPhaseKernelSize = 2049;
    static constexpr int linearPhaseFFTOrder = 12; // 4096-point design FFT
    std::vector<float> linearPhaseKernel;
    std::array<std::vector<float>, 2> linearPhaseHistory;
    int linearPhaseWritePos = 0;
    bool linearPhaseKernelDirty = true;
    int currentLinearPhaseKernelSize = 1025;
    int currentLinearPhaseLatencySamples = (1025 - 1) / 2;
    LinearPhaseQuality currentLinearPhaseQuality = LinearPhaseQuality::mid;

    std::array<float, maxNumFilters> previousFreq;
    std::array<float, maxNumFilters> previousGain;
    std::array<float, maxNumFilters> previousQ;
    std::array<float, maxNumFilters> previousSlope;
    std::array<float, maxNumFilters> previousEnabled;
    std::array<float, maxNumFilters> previousType;
    std::array<float, maxNumFilters> dynamicBaseGain;
    std::array<float, maxNumFilters> dynamicCurrentOffset;

    int currentPhaseLatencySamples = 0;
    PhaseMode currentPhaseMode = PhaseMode::minimum;
    juce::AudioBuffer<float> dryBuffer;

    void updateFiltersFromParameters();
    void updateDynamicGain(const juce::AudioBuffer<float>& sidechainBuffer, const juce::AudioBuffer<float>& inputBuffer);
    void pushToFifo(juce::AbstractFifo& fifo, std::array<float, audioFifoSize>& fifoBuffer, const float* samples, int numSamples);
    float calculateBandLevelDb(const juce::AudioBuffer<float>& buffer, float centerFrequency, float detectorQ) const;
    void updatePhaseModeAndLatency();
    void rebuildLinearPhaseKernel();
    void processLinearPhaseModel(juce::AudioBuffer<float>& wetBuffer);
    void processPhaseModel(juce::AudioBuffer<float>& wetBuffer, const juce::AudioBuffer<float>& dryInput);
    FilterType getFilterTypeFromChoice(int choice);
    float calculateRMS(const juce::AudioBuffer<float>& buffer);
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AudioPluginAudioProcessor)
};
