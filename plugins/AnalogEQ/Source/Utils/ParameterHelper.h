#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include "../DSP/FilterTypes.h"
#include <vector>

//==============================================================================
/**
 * Helper per la creazione e gestione dei parametri dinamici.
 */
namespace ParameterHelper
{
    /**
     * Crea i parametri per un singolo filtro.
     * @param layout Il layout dei parametri a cui aggiungere
     * @param filterIndex L'indice del filtro (0-based)
     * @param filterType Il tipo di filtro
     */
    void addFilterParameters(juce::AudioProcessorValueTreeState::ParameterLayout& layout,
                            int filterIndex,
                            FilterType filterType);
    
    /**
     * Crea un layout di parametri per N filtri di default.
     * @param numFilters Il numero di filtri da creare
     * @return Il layout dei parametri
     */
    juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout(int numFilters = 8);
    
    /**
     * Genera l'ID del parametro per un filtro specifico.
     */
    juce::String getParameterID(int filterIndex, const juce::String& paramName);
    
    /**
     * Genera il nome visualizzato del parametro.
     */
    juce::String getParameterName(int filterIndex, const juce::String& paramName);
    
    /**
     * Range standard per i diversi parametri.
     */
    namespace Ranges
    {
        inline juce::NormalisableRange<float> getFrequencyRange()
        {
            return juce::NormalisableRange<float>(
                20.0f, 20000.0f,
                [](float start, float end, float normalized) {
                    // Scala logaritmica per la frequenza
                    return start * std::pow(end / start, normalized);
                },
                [](float start, float end, float value) {
                    return std::log(value / start) / std::log(end / start);
                });
        }
        
        inline juce::NormalisableRange<float> getGainRange()
        {
            return juce::NormalisableRange<float>(-24.0f, 24.0f, 0.1f);
        }
        
        inline juce::NormalisableRange<float> getQRange()
        {
            return juce::NormalisableRange<float>(0.1f, 10.0f, 0.01f);
        }
        
        inline juce::NormalisableRange<float> getPanRange()
        {
            return juce::NormalisableRange<float>(-100.0f, 100.0f, 1.0f);
        }
        
        inline juce::NormalisableRange<float> getSlopeRange()
        {
            // 0=6dB, 1=12dB, 2=24dB, 3=48dB
            return juce::NormalisableRange<float>(0.0f, 3.0f, 1.0f);
        }
    }
}
