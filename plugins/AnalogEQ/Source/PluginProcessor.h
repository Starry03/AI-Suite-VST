#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
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
    
    // Meter level access (thread-safe)
    float getMeterLevel() const { return meterLevel.load(); }
    
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
    
    // Meter and gain matching
    std::atomic<float> meterLevel { -60.0f };
    float inputRMS = 0.0f;
    float outputRMS = 0.0f;
    
    void updateFiltersFromParameters();
    FilterType getFilterTypeFromChoice(int choice);
    float calculateRMS(const juce::AudioBuffer<float>& buffer);
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AudioPluginAudioProcessor)
};
