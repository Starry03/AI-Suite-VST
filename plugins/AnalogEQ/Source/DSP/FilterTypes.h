#pragma once

#include "FilterBase.h"
#include <juce_dsp/juce_dsp.h>
#include <juce_audio_basics/juce_audio_basics.h>

//==============================================================================
/**
 * Enum per i tipi di filtro disponibili.
 */
enum class FilterType
{
    LowPass,
    HighPass,
    BandPass,
    Bell,
    LowShelf,
    HighShelf,
    Notch
};

//==============================================================================
/**
 * Template base per filtri IIR con comportamento analogico.
 * Usa JUCE DSP per i coefficienti e aggiunge parameter smoothing.
 * Supporta sezioni biquad in cascata per slope variabili.
 */
template<typename CoefficientType>
class IIRFilterAnalog : public FilterBase
{
public:
    IIRFilterAnalog()
    {
        // Inizializza con 1 sezione per canale
        for (auto& channelFilters : filterSections)
        {
            channelFilters.push_back(std::make_unique<juce::dsp::IIR::Filter<float>>());
        }
    }

    void process(juce::AudioBuffer<float>& buffer) override
    {
        if (!enabled || buffer.getNumChannels() == 0)
            return;

        // Smooth parameters per comportamento analogico
        smoothFrequency();

        auto block = juce::dsp::AudioBlock<float>(buffer);

        // Processa ogni canale attraverso tutte le sezioni in cascata
        for (int ch = 0; ch < buffer.getNumChannels() && ch < 2; ++ch)
        {
            auto channelBlock = block.getSingleChannelBlock(ch);
            auto channelContext = juce::dsp::ProcessContextReplacing<float>(channelBlock);

            for (auto& section : filterSections[ch])
                section->process(channelContext);
        }

        // Analog saturation (soft clipping)
        applyAnalogSaturation(buffer);
    }

    void prepare(double sampleRate, int samplesPerBlock) override
    {
        FilterBase::prepare(sampleRate, samplesPerBlock);

        juce::dsp::ProcessSpec spec;
        spec.sampleRate = sampleRate;
        spec.maximumBlockSize = samplesPerBlock;
        spec.numChannels = 1;

        for (auto& channelFilters : filterSections)
            for (auto& filt : channelFilters)
                filt->prepare(spec);

        targetFrequency = frequency;
    }

    void reset() override
    {
        for (auto& channelFilters : filterSections)
            for (auto& filt : channelFilters)
                filt->reset();
    }

    float getFrequencyResponse(float freq) const override
    {
        if (filterSections[0].empty() || filterSections[0][0]->coefficients == nullptr)
            return 0.0f;

        float totalDb = 0.0f;
        for (const auto& section : filterSections[0])
        {
            auto magnitude = section->coefficients->getMagnitudeForFrequency(
                freq, currentSampleRate);
            totalDb += juce::Decibels::gainToDecibels(magnitude);
        }
        return totalDb;
    }

protected:
    // Per canale, un vettore di sezioni biquad in cascata
    std::array<std::vector<std::unique_ptr<juce::dsp::IIR::Filter<float>>>, 2> filterSections;
    float targetFrequency = 1000.0f;

    /**
     * Restituisce il numero di sezioni biquad per LP/HP (dove slope 0 usa 1st order).
     */
    int getNumSectionsLPHP() const
    {
        switch (slope)
        {
            case 0: return 1;   // 6 dB/oct (prima ordine)
            case 1: return 1;   // 12 dB/oct (biquad standard)
            case 2: return 2;   // 24 dB/oct
            case 3: return 4;   // 48 dB/oct
            case 4: return 8;   // 96 dB/oct
            default: return 1;
        }
    }

    /**
     * Restituisce il numero di sezioni biquad per Bell/Shelf/Notch.
     * Ogni livello di slope produce un numero diverso di sezioni.
     */
    int getNumSectionsPeak() const
    {
        switch (slope)
        {
            case 0: return 1;   // 6 dB - base
            case 1: return 2;   // 12 dB - 2 sezioni cascade
            case 2: return 3;   // 24 dB - 3 sezioni
            case 3: return 5;   // 48 dB - 5 sezioni
            case 4: return 8;   // 96 dB - 8 sezioni
            default: return 1;
        }
    }

    /**
     * Assicura che ogni canale abbia il numero corretto di sezioni,
     * creando o rimuovendo sezioni secondo necessità.
     */
    void ensureSections(int numSections)
    {
        for (auto& channelFilters : filterSections)
        {
            while (static_cast<int>(channelFilters.size()) < numSections)
            {
                auto newFilter = std::make_unique<juce::dsp::IIR::Filter<float>>();
                juce::dsp::ProcessSpec spec;
                spec.sampleRate = currentSampleRate;
                spec.maximumBlockSize = static_cast<juce::uint32>(maxSamplesPerBlock);
                spec.numChannels = 1;
                newFilter->prepare(spec);
                channelFilters.push_back(std::move(newFilter));
            }
            while (static_cast<int>(channelFilters.size()) > numSections)
                channelFilters.pop_back();
        }
    }

    void smoothFrequency()
    {
        targetFrequency = frequency;
    }

    void applyAnalogSaturation(juce::AudioBuffer<float>& buffer)
    {
        // Soft clipping per simulare saturazione analogica
        const float threshold = 0.8f;
        const float makeup = 1.0f / threshold;

        for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
        {
            auto* data = buffer.getWritePointer(ch);
            for (int i = 0; i < buffer.getNumSamples(); ++i)
            {
                float x = data[i] * makeup;
                // Tanh saturation
                data[i] = std::tanh(x) * threshold;
            }
        }
    }
};

//==============================================================================
/**
 * Filtro Low Pass (passa-basso).
 */
class LowPassFilter : public IIRFilterAnalog<juce::dsp::IIR::Coefficients<float>>
{
public:
    void updateCoefficients(double sampleRate) override
    {
        int numSections = getNumSectionsLPHP();
        ensureSections(numSections);

        juce::ReferenceCountedObjectPtr<juce::dsp::IIR::Coefficients<float>> coeffs;

        if (slope == 0)
        {
            // Prima ordine (6 dB/oct) - nessun parametro Q
            coeffs = juce::dsp::IIR::Coefficients<float>::makeFirstOrderLowPass(
                sampleRate, targetFrequency);
        }
        else
        {
            // Seconda ordine (biquad) - con Q
            coeffs = juce::dsp::IIR::Coefficients<float>::makeLowPass(
                sampleRate, targetFrequency, q);
        }

        for (auto& channelFilters : filterSections)
            for (auto& filt : channelFilters)
                *filt->coefficients = *coeffs;
    }
};

//==============================================================================
/**
 * Filtro High Pass (passa-alto).
 */
class HighPassFilter : public IIRFilterAnalog<juce::dsp::IIR::Coefficients<float>>
{
public:
    void updateCoefficients(double sampleRate) override
    {
        int numSections = getNumSectionsLPHP();
        ensureSections(numSections);

        juce::ReferenceCountedObjectPtr<juce::dsp::IIR::Coefficients<float>> coeffs;

        if (slope == 0)
        {
            // Prima ordine (6 dB/oct) - nessun parametro Q
            coeffs = juce::dsp::IIR::Coefficients<float>::makeFirstOrderHighPass(
                sampleRate, targetFrequency);
        }
        else
        {
            // Seconda ordine (biquad) - con Q
            coeffs = juce::dsp::IIR::Coefficients<float>::makeHighPass(
                sampleRate, targetFrequency, q);
        }

        for (auto& channelFilters : filterSections)
            for (auto& filt : channelFilters)
                *filt->coefficients = *coeffs;
    }
};

//==============================================================================
/**
 * Filtro Bell (peaking/campana).
 * Con slope > 12 dB, il gain viene diviso tra le sezioni in cascata
 * per mantenere il guadagno totale corretto e ottenere fianchi più ripidi.
 */
class BellFilter : public IIRFilterAnalog<juce::dsp::IIR::Coefficients<float>>
{
public:
    void updateCoefficients(double sampleRate) override
    {
        int numSections = getNumSectionsPeak();
        ensureSections(numSections);

        // Se abbiamo una sola sezione, comportamento standard (Bell classico 2nd order)
        if (numSections == 1)
        {
            auto coeffs = juce::dsp::IIR::Coefficients<float>::makePeakFilter(
                sampleRate, targetFrequency, q,
                juce::Decibels::decibelsToGain(gain));

            for (auto& channelFilters : filterSections)
                if (!channelFilters.empty())
                    *channelFilters[0]->coefficients = *coeffs;
            
            return;
        }

        // Per slope più alti, implementiamo un "Flat Top" bell staggerando le frequenze.
        // Dividiamo il gain totale tra le sezioni.
        float sectionGain = gain / static_cast<float>(numSections);
        
        // Calcoliamo lo spread delle frequenze basato sul Q.
        // Più alto è il Q, più stretto è il range.
        // Euristicamente usiamo una frazione della larghezza di banda (-3dB).
        float bandwidthOct = juce::jmin(2.0f, 1.0f / (q + 0.1f));
        float range = bandwidthOct * 0.6f; // Spread factor tunato a orecchio
        
        // Aumentiamo il Q delle singole sezioni per mantenere la ripidità ai fianchi
        // ed evitare che la somma sia troppo larga.
        float sectionQ = q * std::sqrt(static_cast<float>(numSections));

        for (int i = 0; i < numSections; ++i)
        {
            float offsetOct = 0.0f;
            if (numSections > 1)
                offsetOct = -range * 0.5f + range * (float)i / (float)(numSections - 1);
            
            float freqMultiplier = std::pow(2.0f, offsetOct);
            float sectionFreq = targetFrequency * freqMultiplier;

            auto coeffs = juce::dsp::IIR::Coefficients<float>::makePeakFilter(
                sampleRate, sectionFreq, sectionQ,
                juce::Decibels::decibelsToGain(sectionGain));

            // Applica i coefficienti alla sezione iesima di ogni canale
            for (auto& channelFilters : filterSections)
            {
                if (i < static_cast<int>(channelFilters.size()))
                    *channelFilters[i]->coefficients = *coeffs;
            }
        }
    }
};

//==============================================================================
/**
 * Filtro Low Shelf.
 * Con slope > 12 dB, il gain viene diviso tra le sezioni in cascata.
 */
class LowShelfFilter : public IIRFilterAnalog<juce::dsp::IIR::Coefficients<float>>
{
public:
    void updateCoefficients(double sampleRate) override
    {
        int numSections = getNumSectionsPeak();
        ensureSections(numSections);

        float sectionGain = gain / static_cast<float>(numSections);

        auto coeffs = juce::dsp::IIR::Coefficients<float>::makeLowShelf(
            sampleRate, targetFrequency, q,
            juce::Decibels::decibelsToGain(sectionGain));

        for (auto& channelFilters : filterSections)
            for (auto& filt : channelFilters)
                *filt->coefficients = *coeffs;
    }
};

//==============================================================================
/**
 * Filtro High Shelf.
 * Con slope > 12 dB, il gain viene diviso tra le sezioni in cascata.
 */
class HighShelfFilter : public IIRFilterAnalog<juce::dsp::IIR::Coefficients<float>>
{
public:
    void updateCoefficients(double sampleRate) override
    {
        int numSections = getNumSectionsPeak();
        ensureSections(numSections);

        float sectionGain = gain / static_cast<float>(numSections);

        auto coeffs = juce::dsp::IIR::Coefficients<float>::makeHighShelf(
            sampleRate, targetFrequency, q,
            juce::Decibels::decibelsToGain(sectionGain));

        for (auto& channelFilters : filterSections)
            for (auto& filt : channelFilters)
                *filt->coefficients = *coeffs;
    }
};

//==============================================================================
/**
 * Filtro Notch.
 * Con slope > 12 dB, sezioni in cascata per un notch più profondo/largo.
 */
class NotchFilter : public IIRFilterAnalog<juce::dsp::IIR::Coefficients<float>>
{
public:
    void updateCoefficients(double sampleRate) override
    {
        int numSections = getNumSectionsPeak();
        ensureSections(numSections);

        auto coeffs = juce::dsp::IIR::Coefficients<float>::makeNotch(
            sampleRate, targetFrequency, q);

        for (auto& channelFilters : filterSections)
            for (auto& filt : channelFilters)
                *filt->coefficients = *coeffs;
    }
};
