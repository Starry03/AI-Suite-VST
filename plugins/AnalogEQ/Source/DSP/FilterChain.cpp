#include "FilterChain.h"

FilterBase* FilterChain::addFilter(FilterType filterType)
{
    auto filter = createFilter(filterType);
    auto* filterPtr = filter.get();
    
    filter->prepare(currentSampleRate, currentSamplesPerBlock);
    filters.push_back(std::move(filter));
    
    return filterPtr;
}

void FilterChain::removeFilter(size_t index)
{
    if (index < filters.size())
        filters.erase(filters.begin() + index);
}

void FilterChain::processBlock(juce::AudioBuffer<float>& buffer)
{
    // Processa il buffer attraverso ogni filtro in sequenza
    for (auto& filter : filters)
    {
        if (filter && filter->isEnabled())
            filter->process(buffer);
    }
}

void FilterChain::prepare(double sampleRate, int samplesPerBlock)
{
    currentSampleRate = sampleRate;
    currentSamplesPerBlock = samplesPerBlock;
    
    for (auto& filter : filters)
    {
        if (filter)
            filter->prepare(sampleRate, samplesPerBlock);
    }
}

void FilterChain::reset()
{
    for (auto& filter : filters)
    {
        if (filter)
            filter->reset();
    }
}

float FilterChain::getTotalFrequencyResponse(float frequency) const
{
    float totalResponse = 0.0f;
    
    // Somma le risposte in dB di tutti i filtri
    for (const auto& filter : filters)
    {
        if (filter && filter->isEnabled())
            totalResponse += filter->getFrequencyResponse(frequency);
    }
    
    return totalResponse;
}

FilterBase* FilterChain::getFilter(size_t index)
{
    if (index < filters.size())
        return filters[index].get();
    return nullptr;
}

void FilterChain::updateAllCoefficients(double sampleRate)
{
    for (auto& filter : filters)
    {
        if (filter)
            filter->updateCoefficients(sampleRate);
    }
}

std::unique_ptr<FilterBase> FilterChain::createFilter(FilterType type)
{
    switch (type)
    {
        case FilterType::LowPass:
            return std::make_unique<LowPassFilter>();
        case FilterType::HighPass:
            return std::make_unique<HighPassFilter>();
        case FilterType::Bell:
            return std::make_unique<BellFilter>();
        case FilterType::LowShelf:
            return std::make_unique<LowShelfFilter>();
        case FilterType::HighShelf:
            return std::make_unique<HighShelfFilter>();
        case FilterType::Notch:
            return std::make_unique<NotchFilter>();
        case FilterType::BandPass:
        default:
            // BandPass da implementare se necessario
            return std::make_unique<BellFilter>(); // Default
    }
}

void FilterChain::removeAllFilters()
{
    filters.clear();
}
