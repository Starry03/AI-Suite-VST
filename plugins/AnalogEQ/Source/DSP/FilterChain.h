#pragma once

#include "FilterBase.h"
#include "FilterTypes.h"
#include <juce_audio_basics/juce_audio_basics.h>
#include <vector>
#include <memory>

//==============================================================================
/**
 * Manager per la catena di filtri.
 * Gestisce un numero variabile di filtri e processa l'audio attraverso tutti.
 */
class FilterChain
{
public:
    FilterChain() = default;
    
    /**
     * Aggiunge un filtro alla catena.
     * @param filterType Il tipo di filtro da aggiungere
     * @return Puntatore al filtro aggiunto
     */
    FilterBase* addFilter(FilterType filterType);
    
    /**
     * Rimuove un filtro dalla catena.
     * @param index L'indice del filtro da rimuovere
     */
    void removeFilter(size_t index);
    
    /**
     * Processa un blocco di audio attraverso tutti i filtri.
     * @param buffer Il buffer audio da processare
     */
    void processBlock(juce::AudioBuffer<float>& buffer);
    
    /**
     * Prepara la catena per la riproduzione.
     */
    void prepare(double sampleRate, int samplesPerBlock);
    
    /**
     * Resetta tutti i filtri.
     */
    void reset();
    
    /**
     * Calcola la risposta in frequenza totale della catena.
     * @param frequency La frequenza in Hz
     * @return Il guadagno totale in dB
     */
    float getTotalFrequencyResponse(float frequency) const;
    
    /**
     * Ottiene un filtro specifico.
     */
    FilterBase* getFilter(size_t index);
    
    /**
     * Ottiene il numero di filtri nella catena.
     */
    size_t getNumFilters() const { return filters.size(); }
    
    /**
     * Aggiorna i coefficienti di tutti i filtri.
     */
    void updateAllCoefficients(double sampleRate);
    
    /**
     * Rimuove tutti i filtri dalla catena.
     */
    void removeAllFilters();
    
private:
    std::vector<std::unique_ptr<FilterBase>> filters;
    double currentSampleRate = 44100.0;
    int currentSamplesPerBlock = 512;
    
    std::unique_ptr<FilterBase> createFilter(FilterType type);
};
