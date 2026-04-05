#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "../DSP/FilterChain.h"
#include "../PluginProcessor.h"
#include "SpectrumAnalyzer.h"
#include <juce_audio_processors/juce_audio_processors.h>
#include <memory>

// Forward declaration
class ParameterTooltip;

//==============================================================================
/**
 * Componente per visualizzare la curva di risposta in frequenza.
 * Mostra anche i nodi dei filtri e permette interazione diretta.
 */
class FrequencyResponseCurve : public juce::Component,
                               public juce::Timer
{
public:
    FrequencyResponseCurve(FilterChain& fc, juce::AudioProcessorValueTreeState& apvts,
                          AudioPluginAudioProcessor& processor);
    ~FrequencyResponseCurve() override;
    
    void paint(juce::Graphics& g) override;
    void resized() override;
    void timerCallback() override;
    
    void mouseDown(const juce::MouseEvent& event) override;
    void mouseDrag(const juce::MouseEvent& event) override;
    void mouseUp(const juce::MouseEvent& event) override;
    void mouseMove(const juce::MouseEvent& event) override;
    void mouseExit(const juce::MouseEvent& event) override;
    void mouseDoubleClick(const juce::MouseEvent& event) override;
    void mouseWheelMove(const juce::MouseEvent& event, const juce::MouseWheelDetails& wheel) override;
    
    // Callback for filter selection change
    std::function<void(int)> onFilterSelected;

    // Deselect current filter (called when filter is removed from control panel)
    void deselectFilter();
    
private:
    FilterChain& filterChain;
    juce::AudioProcessorValueTreeState& apvtsRef;
    AudioPluginAudioProcessor& processorRef;
    
    std::unique_ptr<ParameterTooltip> tooltip;
    std::unique_ptr<SpectrumAnalyzer> spectrumAnalyzer;
    
    // Filter interaction state
    int selectedFilterIndex = -1;
    int hoveredFilterIndex = -1;
    bool isDragging = false;
    juce::Point<float> dragStartPosition;
    float dragStartFrequency = 0.0f;
    float dragStartGain = 0.0f;
    
    // Filter node appearance
    static constexpr float nodeRadius = 8.0f;
    
    // Frequency/gain ranges
    static constexpr float minFreq = 20.0f;
    static constexpr float maxFreq = 20000.0f;
    static constexpr float minGain = -24.0f;
    static constexpr float maxGain = 24.0f;
    static constexpr int maxFilters = 8;
    
    // Grid frequencies (Hz)
    std::array<float, 10> gridFrequencies = {20, 50, 100, 200, 500, 1000, 2000, 5000, 10000, 20000};
    
    // Performance optimization
    juce::Path cachedSpectrumPath;
    std::vector<float> cachedResponse;
    bool spectrumNeedsUpdate = true;
    bool responseNeedsUpdate = true;
    int frameSkipCounter = 0;
    
    // Helper methods
    std::vector<float> calculateFrequencyResponse();
    float xToFrequency(float x) const;
    float frequencyToX(float freq) const;
    float yToGain(float y) const;
    float gainToY(float gain) const;
    
    // Drawing methods
    void drawGrid(juce::Graphics& g);
    void drawResponseCurve(juce::Graphics& g, const std::vector<float>& response);
    void drawFrequencyLabels(juce::Graphics& g);
    void drawFilterNodes(juce::Graphics& g);
    void drawSpectrum(juce::Graphics& g); // New: draw spectrum analyzer
    
    // Filter interaction
    int findFilterNearMouse(juce::Point<float> mousePos);
    void addFilterAtMouse(juce::Point<float> mousePos);
    void showContextMenu(int filterIndex);
    juce::Point<float> getFilterNodePosition(int filterIndex);
    
    // Parameter helpers
    void updateFilterParameter(int filterIndex, const juce::String& paramName, float value);
    float getFilterParameter(int filterIndex, const juce::String& paramName);
    juce::String getFilterTypeName(int filterIndex);
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(FrequencyResponseCurve)
};
