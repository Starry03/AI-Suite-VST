#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>

//==============================================================================
/**
 * Classe base astratta per tutti i filtri.
 * Fornisce interfaccia comune per processamento audio, aggiornamento coefficienti,
 * e calcolo della risposta in frequenza.
 */
class FilterBase
{
public:
    virtual ~FilterBase() = default;
    
    /**
     * Processa un blocco di audio attraverso il filtro.
     * @param buffer Il buffer audio da processare (in-place)
     */
    virtual void process(juce::AudioBuffer<float>& buffer) = 0;
    
    /**
     * Aggiorna i coefficienti del filtro basandosi sui parametri correnti.
     * @param sampleRate Il sample rate corrente
     */
    virtual void updateCoefficients(double sampleRate) = 0;
    
    /**
     * Prepara il filtro per la riproduzione.
     * @param sampleRate Il sample rate
     * @param samplesPerBlock Il numero massimo di samples per blocco
     */
    virtual void prepare(double sampleRate, int samplesPerBlock)
    {
        currentSampleRate = sampleRate;
        maxSamplesPerBlock = samplesPerBlock;
    }
    
    /**
     * Resetta lo stato interno del filtro.
     */
    virtual void reset() = 0;
    
    /**
     * Calcola la risposta in frequenza del filtro alla frequenza specificata.
     * @param frequency La frequenza in Hz
     * @return Il guadagno in dB alla frequenza specificata
     */
    virtual float getFrequencyResponse(float frequency) const = 0;
    
    // Setters per i parametri comuni
    virtual void setFrequency(float freq) { frequency = freq; }
    virtual void setGain(float gainDb) { gain = gainDb; }
    virtual void setQ(float qVal) { q = qVal; }
    virtual void setSlope(int slopeIndex) { slope = slopeIndex; }

    // Getters
    float getFrequency() const { return frequency; }
    float getGain() const { return gain; }
    float getQ() const { return q; }
    int getSlope() const { return slope; }
    bool isEnabled() const { return enabled; }
    void setEnabled(bool shouldBeEnabled) { enabled = shouldBeEnabled; }
    
protected:
    float frequency = 1000.0f;  // Hz
    float gain = 0.0f;          // dB
    float q = 0.707f;           // Q factor
    int slope = 1;              // 0=6dB, 1=12dB, 2=24dB, 3=48dB, 4=96dB
    bool enabled = true;
    
    double currentSampleRate = 44100.0;
    int maxSamplesPerBlock = 512;
};
