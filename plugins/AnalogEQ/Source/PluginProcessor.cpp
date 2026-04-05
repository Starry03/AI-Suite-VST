#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "Utils/ParameterHelper.h"

//==============================================================================
AudioPluginAudioProcessor::AudioPluginAudioProcessor()
    : AudioProcessor(BusesProperties()
                    .withInput("Input", juce::AudioChannelSet::stereo(), true)
                    .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      apvts(*this, nullptr, "Parameters", ParameterHelper::createParameterLayout(maxNumFilters))
{
    // Inizializza i filtri
    filterInstances.fill(nullptr);
    currentFilterTypes.fill(FilterType::Bell);
    
    // Crea i filtri iniziali (tutti disabilitati tranne il primo)
    for (int i = 0; i < maxNumFilters; ++i)
    {
        auto* filter = filterChain.addFilter(FilterType::Bell);
        filterInstances[i] = filter;
        currentFilterTypes[i] = FilterType::Bell;
        
        if (filter)
        {
            filter->setFrequency(1000.0f);
            filter->setGain(0.0f);
            filter->setQ(0.707f);
            filter->setEnabled(i == 0); // Solo il primo attivo
        }
    }
}

AudioPluginAudioProcessor::~AudioPluginAudioProcessor()
{
}

//==============================================================================
const juce::String AudioPluginAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool AudioPluginAudioProcessor::acceptsMidi() const
{
    return false;
}

bool AudioPluginAudioProcessor::producesMidi() const
{
    return false;
}

bool AudioPluginAudioProcessor::isMidiEffect() const
{
    return false;
}

double AudioPluginAudioProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int AudioPluginAudioProcessor::getNumPrograms()
{
    return 1;
}

int AudioPluginAudioProcessor::getCurrentProgram()
{
    return 0;
}

void AudioPluginAudioProcessor::setCurrentProgram(int index)
{
    juce::ignoreUnused(index);
}

const juce::String AudioPluginAudioProcessor::getProgramName(int index)
{
    juce::ignoreUnused(index);
    return {};
}

void AudioPluginAudioProcessor::changeProgramName(int index, const juce::String& newName)
{
    juce::ignoreUnused(index, newName);
}

//==============================================================================
void AudioPluginAudioProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    // Prepara la catena di filtri
    filterChain.prepare(sampleRate, samplesPerBlock);
    
    // Aggiorna i coefficienti dei filtri con il nuovo sample rate
    filterChain.updateAllCoefficients(sampleRate);
    
    // Nota: Lo spectrum analyzer verrà preparato nel FrequencyResponseCurve quando riceve i primi campioni
}

void AudioPluginAudioProcessor::releaseResources()
{
    filterChain.reset();
}

bool AudioPluginAudioProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const
{
    // Supporta solo stereo
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

    if (layouts.getMainInputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

    return true;
}

void AudioPluginAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer,
                                             juce::MidiBuffer& midiMessages)
{
    juce::ignoreUnused(midiMessages);
    juce::ScopedNoDenormals noDenormals;

    // Cattura audio per spectrum analyzer (solo canale sinistro)
    const int numSamples = buffer.getNumSamples();
    const float* channelData = buffer.getReadPointer(0);
    
    // Copy samples to FIFO in chunks that fit
    int start = 0;
    while (start < numSamples)
    {
        int numToWrite = std::min(numSamples - start, audioFifo.getFreeSpace());
        if (numToWrite > 0)
        {
            int start1, size1, start2, size2;
            audioFifo.prepareToWrite(numToWrite, start1, size1, start2, size2);
            
            if (size1 > 0)
                std::copy_n(channelData + start, size1, audioFifoBuffer.data() + start1);
            if (size2 > 0)
                std::copy_n(channelData + start + size1, size2, audioFifoBuffer.data() + start2);
            
            audioFifo.finishedWrite(size1 + size2);
            start += size1 + size2;
        }
        else
            break; // FIFO is full
    }

    // Aggiorna i parametri dei filtri
    updateFiltersFromParameters();

    // Calculate input RMS for gain matching
    inputRMS = calculateRMS(buffer);

    // Processa attraverso la catena di filtri
    filterChain.processBlock(buffer);

    // Calculate output RMS after filtering
    outputRMS = calculateRMS(buffer);

    // Apply auto-gain (gain matching)
    auto autoGainParam = apvts.getRawParameterValue("auto_gain");
    if (autoGainParam && autoGainParam->load() > 0.5f)
    {
        // Avoid division by zero and extreme gains
        if (outputRMS > 0.0001f && inputRMS > 0.0001f)
        {
            float makeupGain = inputRMS / outputRMS;
            // Limit makeup gain to reasonable range (-12dB to +12dB)
            makeupGain = juce::jlimit(0.25f, 4.0f, makeupGain);
            buffer.applyGain(makeupGain);
            
            // Recalculate output RMS after makeup gain
            outputRMS = calculateRMS(buffer);
        }
    }

    // Applica output gain
    auto outputGain = apvts.getRawParameterValue("output_gain");
    if (outputGain != nullptr)
    {
        float gain = juce::Decibels::decibelsToGain(outputGain->load());
        buffer.applyGain(gain);
    }
    
    // Update meter level (calculate peak for meter display)
    float peak = buffer.getMagnitude(0, numSamples);
    float peakDB = peak > 0.00001f ? juce::Decibels::gainToDecibels(peak) : -60.0f;
    meterLevel.store(peakDB);
}

//==============================================================================
bool AudioPluginAudioProcessor::hasEditor() const
{
    return true;
}

juce::AudioProcessorEditor* AudioPluginAudioProcessor::createEditor()
{
    return new AudioPluginAudioProcessorEditor(*this);
}

//==============================================================================
void AudioPluginAudioProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    std::unique_ptr<juce::XmlElement> xml(state.createXml());
    copyXmlToBinary(*xml, destData);
}

void AudioPluginAudioProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xmlState(getXmlFromBinary(data, sizeInBytes));

    if (xmlState.get() != nullptr)
        if (xmlState->hasTagName(apvts.state.getType()))
            apvts.replaceState(juce::ValueTree::fromXml(*xmlState));
}

//==============================================================================
void AudioPluginAudioProcessor::updateFiltersFromParameters()
{
    for (int i = 0; i < maxNumFilters; ++i)
    {
        juce::String prefix = "filter" + juce::String(i) + "_";
        
        auto* filter = filterInstances[i];
        if (!filter)
            continue;
        
        // Aggiorna enabled
        auto enabledParam = apvts.getRawParameterValue(prefix + "enabled");
        if (enabledParam)
            filter->setEnabled(enabledParam->load() > 0.5f);

        // Aggiorna tipo (ricrea il filtro solo se cambiato)
        auto typeParam = apvts.getRawParameterValue(prefix + "type");
        if (typeParam)
        {
            FilterType newType = getFilterTypeFromChoice(static_cast<int>(typeParam->load()));

            // Ricrea solo se il tipo è cambiato
            if (newType != currentFilterTypes[i])
            {
                // Rimuovi tutti i filtri e ricreali
                filterChain.removeAllFilters();

                // Ricrea tutti i filtri con i tipi corretti
                for (int j = 0; j < maxNumFilters; ++j)
                {
                    juce::String jPrefix = "filter" + juce::String(j) + "_";
                    auto jTypeParam = apvts.getRawParameterValue(jPrefix + "type");
                    FilterType jType = currentFilterTypes[j];

                    if (jTypeParam)
                        jType = getFilterTypeFromChoice(static_cast<int>(jTypeParam->load()));

                    currentFilterTypes[j] = jType;
                    filterInstances[j] = filterChain.addFilter(jType);

                    // Ripristina parametri da APVTS per tutti i filtri
                    auto jFreqParam = apvts.getRawParameterValue(jPrefix + "freq");
                    auto jGainParam = apvts.getRawParameterValue(jPrefix + "gain");
                    auto jQParam = apvts.getRawParameterValue(jPrefix + "q");
                    auto jEnabledParam = apvts.getRawParameterValue(jPrefix + "enabled");
                    auto jSlopeParam = apvts.getRawParameterValue(jPrefix + "slope");

                    if (filterInstances[j])
                    {
                        if (jFreqParam) filterInstances[j]->setFrequency(jFreqParam->load());
                        if (jGainParam) filterInstances[j]->setGain(jGainParam->load());
                        if (jQParam) filterInstances[j]->setQ(jQParam->load());
                        if (jSlopeParam) filterInstances[j]->setSlope(static_cast<int>(jSlopeParam->load()));
                        if (jEnabledParam) filterInstances[j]->setEnabled(jEnabledParam->load() > 0.5f);
                        filterInstances[j]->updateCoefficients(getSampleRate());
                    }
                }

                // Aggiorna il puntatore corrente e salta aggiornamento singolo
                // (tutti i parametri sono già stati applicati nel loop sopra)
                filter = filterInstances[i];
                continue;
            }
        }

        // Leggi tutti i parametri prima di aggiornare i coefficienti (una sola volta)
        auto freqParam = apvts.getRawParameterValue(prefix + "freq");
        auto gainParam = apvts.getRawParameterValue(prefix + "gain");
        auto qParam = apvts.getRawParameterValue(prefix + "q");
        auto slopeParam = apvts.getRawParameterValue(prefix + "slope");

        if (freqParam) filter->setFrequency(freqParam->load());
        if (gainParam) filter->setGain(gainParam->load());
        if (qParam) filter->setQ(qParam->load());
        if (slopeParam) filter->setSlope(static_cast<int>(slopeParam->load()));

        filter->updateCoefficients(getSampleRate());
    }
}

FilterType AudioPluginAudioProcessor::getFilterTypeFromChoice(int choice)
{
    switch (choice)
    {
        case 0: return FilterType::LowPass;
        case 1: return FilterType::HighPass;
        case 2: return FilterType::Bell;
        case 3: return FilterType::LowShelf;
        case 4: return FilterType::HighShelf;
        case 5: return FilterType::Notch;
        default: return FilterType::Bell;
    }
}

float AudioPluginAudioProcessor::calculateRMS(const juce::AudioBuffer<float>& buffer)
{
    float sumSquares = 0.0f;
    int totalSamples = 0;
    
    for (int channel = 0; channel < buffer.getNumChannels(); ++channel)
    {
        const float* channelData = buffer.getReadPointer(channel);
        int numSamples = buffer.getNumSamples();
        
        for (int i = 0; i < numSamples; ++i)
        {
            float sample = channelData[i];
            sumSquares += sample * sample;
        }
        
        totalSamples += numSamples;
    }
    
    if (totalSamples == 0)
        return 0.0f;
    
    return std::sqrt(sumSquares / static_cast<float>(totalSamples));
}

//==============================================================================
// Questo crea nuove istanze del plugin
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new AudioPluginAudioProcessor();
}
