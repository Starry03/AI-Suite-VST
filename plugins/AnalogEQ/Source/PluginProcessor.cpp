#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "Utils/ParameterHelper.h"
#include <cmath>

//==============================================================================
AudioPluginAudioProcessor::AudioPluginAudioProcessor()
    : AudioProcessor(BusesProperties()
                    .withInput("Input", juce::AudioChannelSet::stereo(), true)
                    .withInput("Sidechain", juce::AudioChannelSet::stereo(), false)
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

    for (auto& line : naturalPhaseDelayLines)
        line.assign(maxNaturalPhaseDelaySamples, 0.0f);

    linearPhaseKernel.assign(static_cast<size_t>(maxLinearPhaseKernelSize), 0.0f);
    linearPhaseKernel[static_cast<size_t>(currentLinearPhaseLatencySamples)] = 1.0f;
    for (auto& history : linearPhaseHistory)
        history.assign(static_cast<size_t>(maxLinearPhaseKernelSize), 0.0f);

    const auto nan = std::numeric_limits<float>::quiet_NaN();
    previousFreq.fill(nan);
    previousGain.fill(nan);
    previousQ.fill(nan);
    previousSlope.fill(nan);
    previousEnabled.fill(nan);
    previousType.fill(nan);
    dynamicBaseGain.fill(0.0f);
    dynamicCurrentOffset.fill(0.0f);
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

    dryBuffer.setSize(2, juce::jmax(samplesPerBlock, 1), false, false, true);
    dryBuffer.clear();

    for (auto& line : naturalPhaseDelayLines)
        std::fill(line.begin(), line.end(), 0.0f);
    naturalPhaseWritePos = 0;

    for (auto& history : linearPhaseHistory)
        std::fill(history.begin(), history.end(), 0.0f);
    linearPhaseWritePos = 0;
    linearPhaseKernelDirty = true;

    updatePhaseModeAndLatency();

    // Nota: Lo spectrum analyzer verrà preparato nel FrequencyResponseCurve quando riceve i primi campioni
}

void AudioPluginAudioProcessor::releaseResources()
{
    filterChain.reset();
    dryBuffer.setSize(0, 0);
}

bool AudioPluginAudioProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const
{
    // Supporta solo stereo
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

    if (layouts.getMainInputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

    if (layouts.inputBuses.size() > 1)
    {
        const auto sidechain = layouts.getChannelSet(true, 1);
        if (!(sidechain.isDisabled() || sidechain == juce::AudioChannelSet::mono() || sidechain == juce::AudioChannelSet::stereo()))
            return false;
    }

    return true;
}

void AudioPluginAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer,
                                             juce::MidiBuffer& midiMessages)
{
    juce::ignoreUnused(midiMessages);
    juce::ScopedNoDenormals noDenormals;

    // Main input/signal buffers
    auto mainInput = getBusBuffer(buffer, true, 0);
    juce::AudioBuffer<float> emptySidechain;
    auto sidechainInput = (getBusCount(true) > 1) ? getBusBuffer(buffer, true, 1) : emptySidechain;
    const auto* sidechainEnabledParam = apvts.getRawParameterValue("sidechain_enabled");
    const bool sidechainEnabled = (sidechainEnabledParam != nullptr && sidechainEnabledParam->load() > 0.5f);
    sidechainEnabledForUI.store(sidechainEnabled);

    // Cattura audio per spectrum analyzer (solo canale sinistro)
    const int numSamples = buffer.getNumSamples();
    if (mainInput.getNumChannels() > 0)
        pushToFifo(audioFifo, audioFifoBuffer, mainInput.getReadPointer(0), numSamples);

    if (sidechainEnabled && sidechainInput.getNumChannels() > 0)
        pushToFifo(sidechainAudioFifo, sidechainAudioFifoBuffer, sidechainInput.getReadPointer(0), numSamples);

    // Aggiorna i parametri dei filtri
    updateDynamicGain(sidechainInput, mainInput);
    updateFiltersFromParameters();
    updatePhaseModeAndLatency();

    dryBuffer.makeCopyOf(buffer, true);

    // Calculate input RMS for gain matching
    inputRMS = calculateRMS(buffer);

    if (currentPhaseMode == PhaseMode::linear)
    {
        if (linearPhaseKernelDirty)
            rebuildLinearPhaseKernel();

        buffer.makeCopyOf(dryBuffer, true);
        processLinearPhaseModel(buffer);
    }
    else
    {
        // Minimum/Natural phase use IIR chain directly.
        filterChain.processBlock(buffer);
        processPhaseModel(buffer, dryBuffer);
    }

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

juce::String AudioPluginAudioProcessor::getCurrentPhaseModeName() const
{
    switch (currentPhaseModeForUI.load())
    {
        case 1: return "Natural";
        case 2: return "Linear Phase";
        case 0:
        default: return "Minimum";
    }
}

juce::String AudioPluginAudioProcessor::getCurrentLinearPhaseQualityName() const
{
    switch (currentLinearPhaseQualityForUI.load())
    {
        case 0: return "Low";
        case 2: return "High";
        case 1:
        default: return "Mid";
    }
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
    bool filterStateChanged = false;

    auto hasChanged = [](float previous, float current)
    {
        return std::isnan(previous) || std::abs(previous - current) > 0.0001f;
    };

    for (int i = 0; i < maxNumFilters; ++i)
    {
        juce::String prefix = "filter" + juce::String(i) + "_";
        
        auto* filter = filterInstances[i];
        if (!filter)
            continue;
        
        // Aggiorna enabled
        auto enabledParam = apvts.getRawParameterValue(prefix + "enabled");
        if (enabledParam)
        {
            const auto enabledValue = enabledParam->load();
            if (hasChanged(previousEnabled[i], enabledValue))
            {
                previousEnabled[i] = enabledValue;
                filterStateChanged = true;
            }
            filter->setEnabled(enabledParam->load() > 0.5f);
        }

        // Aggiorna tipo (ricrea il filtro solo se cambiato)
        auto typeParam = apvts.getRawParameterValue(prefix + "type");
        if (typeParam)
        {
            const auto typeValue = typeParam->load();
            if (hasChanged(previousType[i], typeValue))
            {
                previousType[i] = typeValue;
                filterStateChanged = true;
            }

            FilterType newType = getFilterTypeFromChoice(static_cast<int>(typeValue));

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
                        if (jGainParam)
                        {
                            const auto baseGain = jGainParam->load();
                            dynamicBaseGain[j] = baseGain;
                            filterInstances[j]->setGain(baseGain + dynamicCurrentOffset[j]);
                        }
                        if (jQParam) filterInstances[j]->setQ(jQParam->load());
                        if (jSlopeParam) filterInstances[j]->setSlope(static_cast<int>(jSlopeParam->load()));
                        if (jEnabledParam) filterInstances[j]->setEnabled(jEnabledParam->load() > 0.5f);
                        filterInstances[j]->updateCoefficients(getSampleRate());

                        if (jFreqParam) previousFreq[j] = jFreqParam->load();
                        if (jGainParam) previousGain[j] = jGainParam->load();
                        if (jQParam) previousQ[j] = jQParam->load();
                        if (jSlopeParam) previousSlope[j] = jSlopeParam->load();
                        if (jEnabledParam) previousEnabled[j] = jEnabledParam->load();
                        if (jTypeParam) previousType[j] = jTypeParam->load();
                    }
                }

                // Aggiorna il puntatore corrente e salta aggiornamento singolo
                // (tutti i parametri sono già stati applicati nel loop sopra)
                filter = filterInstances[i];
                filterStateChanged = true;
                continue;
            }
        }

        // Leggi tutti i parametri prima di aggiornare i coefficienti (una sola volta)
        auto freqParam = apvts.getRawParameterValue(prefix + "freq");
        auto gainParam = apvts.getRawParameterValue(prefix + "gain");
        auto qParam = apvts.getRawParameterValue(prefix + "q");
        auto slopeParam = apvts.getRawParameterValue(prefix + "slope");

        if (freqParam)
        {
            const auto value = freqParam->load();
            if (hasChanged(previousFreq[i], value))
            {
                previousFreq[i] = value;
                filterStateChanged = true;
            }
            filter->setFrequency(value);
        }
        if (gainParam)
        {
            const auto value = gainParam->load();
            if (hasChanged(previousGain[i], value))
            {
                previousGain[i] = value;
                filterStateChanged = true;
            }
            dynamicBaseGain[i] = value;
            filter->setGain(value + dynamicCurrentOffset[i]);
        }
        if (qParam)
        {
            const auto value = qParam->load();
            if (hasChanged(previousQ[i], value))
            {
                previousQ[i] = value;
                filterStateChanged = true;
            }
            filter->setQ(value);
        }
        if (slopeParam)
        {
            const auto value = slopeParam->load();
            if (hasChanged(previousSlope[i], value))
            {
                previousSlope[i] = value;
                filterStateChanged = true;
            }
            filter->setSlope(static_cast<int>(value));
        }

        filter->updateCoefficients(getSampleRate());
    }

    if (filterStateChanged)
        linearPhaseKernelDirty = true;
}

void AudioPluginAudioProcessor::updateDynamicGain(const juce::AudioBuffer<float>& sidechainBuffer,
                                                  const juce::AudioBuffer<float>& inputBuffer)
{
    const auto* sidechainEnabledParam = apvts.getRawParameterValue("sidechain_enabled");
    const bool sidechainEnabled = (sidechainEnabledParam != nullptr && sidechainEnabledParam->load() > 0.5f);

    const juce::AudioBuffer<float>* detectorBuffer = &inputBuffer;
    if (sidechainEnabled && sidechainBuffer.getNumChannels() > 0 && sidechainBuffer.getNumSamples() > 0)
        detectorBuffer = &sidechainBuffer;

    bool dynamicOffsetChanged = false;
    const float sr = static_cast<float>(juce::jmax(1.0, getSampleRate()));
    const int numSamples = juce::jmax(1, detectorBuffer->getNumSamples());
    constexpr float fallbackReleaseCoeff = 0.08f;

    for (int i = 0; i < maxNumFilters; ++i)
    {
        const juce::String prefix = "filter" + juce::String(i) + "_";
        const auto* enabledParam = apvts.getRawParameterValue(prefix + "enabled");
        const auto* thresholdParam = apvts.getRawParameterValue(prefix + "dyn_threshold");
        const auto* dynGainParam = apvts.getRawParameterValue(prefix + "dyn_gain");
        const auto* attackParam = apvts.getRawParameterValue(prefix + "dyn_attack_ms");
        const auto* releaseParam = apvts.getRawParameterValue(prefix + "dyn_release_ms");
        const auto* modeParam = apvts.getRawParameterValue(prefix + "dyn_mode");
        const auto* detectorQParam = apvts.getRawParameterValue(prefix + "dyn_detector_q");

        if (!enabledParam || !thresholdParam || !dynGainParam || !attackParam || !releaseParam || !modeParam || !detectorQParam
            || enabledParam->load() < 0.5f)
        {
            auto& currentOffset = dynamicCurrentOffset[static_cast<size_t>(i)];
            const auto before = currentOffset;
            currentOffset *= (1.0f - fallbackReleaseCoeff);
            if (std::abs(currentOffset - before) > 0.05f)
                dynamicOffsetChanged = true;
            continue;
        }

        const float amountDb = juce::jmax(0.0f, dynGainParam->load());
        if (amountDb <= 0.0001f)
        {
            auto& currentOffset = dynamicCurrentOffset[static_cast<size_t>(i)];
            const auto before = currentOffset;
            currentOffset *= (1.0f - fallbackReleaseCoeff);
            if (std::abs(currentOffset - before) > 0.05f)
                dynamicOffsetChanged = true;
            continue;
        }

        const float thresholdDb = thresholdParam->load();
        const auto* freqParam = apvts.getRawParameterValue(prefix + "freq");
        const float freq = juce::jmax(20.0f, freqParam != nullptr ? freqParam->load() : 1000.0f);
        const float detectorQ = juce::jmax(0.3f, detectorQParam->load());
        const float detectorDb = calculateBandLevelDb(*detectorBuffer, freq, detectorQ);
        const float overDb = juce::jmax(0.0f, detectorDb - thresholdDb);
        const float overNorm = juce::jlimit(0.0f, 1.0f, overDb / 24.0f);
        const float baseGain = dynamicBaseGain[static_cast<size_t>(i)];

        const int mode = static_cast<int>(modeParam->load());
        float direction = (baseGain >= 0.0f) ? -1.0f : 1.0f; // Auto
        if (mode == 1)
            direction = -1.0f; // Cut
        else if (mode == 2)
            direction = 1.0f;  // Boost

        const float targetOffset = direction * amountDb * overNorm;
        float& currentOffset = dynamicCurrentOffset[static_cast<size_t>(i)];

        const float attackMs = juce::jmax(1.0f, attackParam->load());
        const float releaseMs = juce::jmax(5.0f, releaseParam->load());
        const float attackCoeff = 1.0f - std::exp(-static_cast<float>(numSamples) / (attackMs * 0.001f * sr));
        const float releaseCoeff = 1.0f - std::exp(-static_cast<float>(numSamples) / (releaseMs * 0.001f * sr));
        const float coeff = (std::abs(targetOffset) > std::abs(currentOffset)) ? attackCoeff : releaseCoeff;
        const auto before = currentOffset;
        currentOffset += coeff * (targetOffset - currentOffset);
        if (std::abs(currentOffset - before) > 0.05f)
            dynamicOffsetChanged = true;
    }

    if (currentPhaseMode == PhaseMode::linear && dynamicOffsetChanged)
        linearPhaseKernelDirty = true;
}

float AudioPluginAudioProcessor::calculateBandLevelDb(const juce::AudioBuffer<float>& buffer,
                                                      float centerFrequency,
                                                      float detectorQ) const
{
    const int numSamples = buffer.getNumSamples();
    if (numSamples <= 8 || buffer.getNumChannels() == 0)
        return -60.0f;

    const float sr = static_cast<float>(juce::jmax(1.0, getSampleRate()));
    const float clampedFreq = juce::jlimit(20.0f, 0.495f * sr, centerFrequency);
    const float kFloat = (static_cast<float>(numSamples) * clampedFreq) / sr;
    const int centerBin = juce::jlimit(1, numSamples / 2 - 2, static_cast<int>(std::round(kFloat)));

    const int halfWidth = juce::jlimit(1, 8, static_cast<int>(std::ceil(4.0f / juce::jmax(0.3f, detectorQ))));
    float weightedPower = 0.0f;
    float weightSum = 0.0f;

    const int channelCount = juce::jmin(2, buffer.getNumChannels());
    std::vector<float> mono(static_cast<size_t>(numSamples), 0.0f);
    for (int ch = 0; ch < channelCount; ++ch)
    {
        const float* read = buffer.getReadPointer(ch);
        for (int i = 0; i < numSamples; ++i)
            mono[static_cast<size_t>(i)] += read[i];
    }

    const float invChannels = 1.0f / static_cast<float>(channelCount);
    for (auto& sample : mono)
        sample *= invChannels;

    for (int offset = -halfWidth; offset <= halfWidth; ++offset)
    {
        const int k = juce::jlimit(1, numSamples / 2 - 2, centerBin + offset);
        const float omega = juce::MathConstants<float>::twoPi * static_cast<float>(k) / static_cast<float>(numSamples);
        const float coeff = 2.0f * std::cos(omega);

        float s0 = 0.0f, s1 = 0.0f, s2 = 0.0f;
        for (int n = 0; n < numSamples; ++n)
        {
            s0 = mono[static_cast<size_t>(n)] + coeff * s1 - s2;
            s2 = s1;
            s1 = s0;
        }

        const float power = s1 * s1 + s2 * s2 - coeff * s1 * s2;
        const float weight = 1.0f / (1.0f + static_cast<float>(std::abs(offset)));
        weightedPower += weight * juce::jmax(0.0f, power);
        weightSum += weight;
    }

    const float avgPower = (weightSum > 0.0f) ? (weightedPower / weightSum) : 0.0f;
    const float normalized = avgPower / static_cast<float>(numSamples * numSamples);
    const float magnitude = std::sqrt(juce::jmax(1.0e-12f, normalized));
    const float db = 20.0f * std::log10(magnitude + 1.0e-12f);
    return juce::jlimit(-60.0f, 12.0f, db);
}

void AudioPluginAudioProcessor::pushToFifo(juce::AbstractFifo& fifo,
                                           std::array<float, audioFifoSize>& fifoBuffer,
                                           const float* samples,
                                           int numSamples)
{
    if (samples == nullptr || numSamples <= 0)
        return;

    int start = 0;
    while (start < numSamples)
    {
        const int numToWrite = std::min(numSamples - start, fifo.getFreeSpace());
        if (numToWrite <= 0)
            break;

        int start1 = 0, size1 = 0, start2 = 0, size2 = 0;
        fifo.prepareToWrite(numToWrite, start1, size1, start2, size2);

        if (size1 > 0)
            std::copy_n(samples + start, size1, fifoBuffer.data() + start1);
        if (size2 > 0)
            std::copy_n(samples + start + size1, size2, fifoBuffer.data() + start2);

        fifo.finishedWrite(size1 + size2);
        start += size1 + size2;
    }
}

void AudioPluginAudioProcessor::updatePhaseModeAndLatency()
{
    auto* phaseParam = apvts.getRawParameterValue("phase_mode");
    const int modeValue = phaseParam != nullptr ? static_cast<int>(phaseParam->load()) : 0;

    switch (modeValue)
    {
        case 1: currentPhaseMode = PhaseMode::natural; break;
        case 2: currentPhaseMode = PhaseMode::linear; break;
        default: currentPhaseMode = PhaseMode::minimum; break;
    }
    currentPhaseModeForUI.store(modeValue);

    auto* qualityParam = apvts.getRawParameterValue("linear_phase_quality");
    const int qualityValue = qualityParam != nullptr ? static_cast<int>(qualityParam->load()) : 1;

    switch (qualityValue)
    {
        case 0:
            currentLinearPhaseQuality = LinearPhaseQuality::low;
            currentLinearPhaseKernelSize = 513;
            currentLinearPhaseLatencySamples = 256;
            break;
        case 2:
            currentLinearPhaseQuality = LinearPhaseQuality::high;
            currentLinearPhaseKernelSize = 2049;
            currentLinearPhaseLatencySamples = 1024;
            break;
        case 1:
        default:
            currentLinearPhaseQuality = LinearPhaseQuality::mid;
            currentLinearPhaseKernelSize = 1025;
            currentLinearPhaseLatencySamples = 512;
            break;
    }
    currentLinearPhaseQualityForUI.store(qualityValue);

    if (linearPhaseKernel.size() != static_cast<size_t>(currentLinearPhaseKernelSize))
    {
        linearPhaseKernel.assign(static_cast<size_t>(currentLinearPhaseKernelSize), 0.0f);
        for (auto& history : linearPhaseHistory)
            history.assign(static_cast<size_t>(currentLinearPhaseKernelSize), 0.0f);
        linearPhaseWritePos = 0;
        linearPhaseKernelDirty = true;
    }

    int requestedLatency = 0;
    if (currentPhaseMode == PhaseMode::natural)
        requestedLatency = naturalPhaseLatencySamples;
    else if (currentPhaseMode == PhaseMode::linear)
        requestedLatency = currentLinearPhaseLatencySamples;

    if (requestedLatency != currentPhaseLatencySamples)
    {
        currentPhaseLatencySamples = requestedLatency;
        setLatencySamples(currentPhaseLatencySamples);
    }

    currentLatencySamplesForUI.store(currentPhaseLatencySamples);
}

void AudioPluginAudioProcessor::rebuildLinearPhaseKernel()
{
    const auto sampleRate = juce::jmax(1.0, getSampleRate());
    const int fftSize = 1 << linearPhaseFFTOrder;
    const int maxBin = fftSize / 2;

    juce::dsp::FFT fft(linearPhaseFFTOrder);
    std::vector<float> ifftBuffer(static_cast<size_t>(2 * fftSize), 0.0f);

    for (int bin = 0; bin <= maxBin; ++bin)
    {
        const float frequency = juce::jmax(1.0f, static_cast<float>(sampleRate * static_cast<double>(bin) / static_cast<double>(fftSize)));
        const float responseDb = filterChain.getTotalFrequencyResponse(frequency);
        const float magnitude = juce::jlimit(0.0001f, 16.0f, juce::Decibels::decibelsToGain(responseDb));

        ifftBuffer[static_cast<size_t>(2 * bin)] = magnitude;
        ifftBuffer[static_cast<size_t>(2 * bin + 1)] = 0.0f;

        if (bin > 0 && bin < maxBin)
        {
            const int mirrored = fftSize - bin;
            ifftBuffer[static_cast<size_t>(2 * mirrored)] = magnitude;
            ifftBuffer[static_cast<size_t>(2 * mirrored + 1)] = 0.0f;
        }
    }

    fft.performRealOnlyInverseTransform(ifftBuffer.data());

    const int kernelSize = currentLinearPhaseKernelSize;
    const int delay = currentLinearPhaseLatencySamples;
    const float denom = static_cast<float>(juce::jmax(1, kernelSize - 1));

    for (int n = 0; n < kernelSize; ++n)
    {
        const int shiftedIndex = (n - delay + fftSize) % fftSize;
        const float x = static_cast<float>(n) / denom;
        const float window = 0.42f - 0.5f * std::cos(juce::MathConstants<float>::twoPi * x)
                           + 0.08f * std::cos(2.0f * juce::MathConstants<float>::twoPi * x);
        linearPhaseKernel[static_cast<size_t>(n)] = ifftBuffer[static_cast<size_t>(shiftedIndex)] * window;
    }

    float dcGain = 0.0f;
    for (const auto tap : linearPhaseKernel)
        dcGain += tap;

    if (std::abs(dcGain) > 1.0e-6f)
    {
        const float norm = 1.0f / dcGain;
        for (auto& tap : linearPhaseKernel)
            tap *= norm;
    }

    for (auto& history : linearPhaseHistory)
        std::fill(history.begin(), history.end(), 0.0f);
    linearPhaseWritePos = 0;
    linearPhaseKernelDirty = false;
}

void AudioPluginAudioProcessor::processLinearPhaseModel(juce::AudioBuffer<float>& wetBuffer)
{
    const int numSamples = wetBuffer.getNumSamples();
    if (numSamples <= 0)
        return;

    const int numChannels = juce::jmin(2, wetBuffer.getNumChannels());
    const int kernelSize = static_cast<int>(linearPhaseKernel.size());
    if (kernelSize <= 0)
        return;

    int writePos = linearPhaseWritePos;

    for (int sample = 0; sample < numSamples; ++sample)
    {
        for (int ch = 0; ch < numChannels; ++ch)
        {
            auto* data = wetBuffer.getWritePointer(ch);
            auto& history = linearPhaseHistory[static_cast<size_t>(ch)];

            history[static_cast<size_t>(writePos)] = data[sample];

            float y = 0.0f;
            int tapIndex = writePos;
            for (int tap = 0; tap < kernelSize; ++tap)
            {
                y += linearPhaseKernel[static_cast<size_t>(tap)] * history[static_cast<size_t>(tapIndex)];
                tapIndex = (tapIndex == 0) ? (kernelSize - 1) : (tapIndex - 1);
            }

            data[sample] = y;
        }

        writePos = (writePos + 1) % kernelSize;
    }

    linearPhaseWritePos = writePos;
}

void AudioPluginAudioProcessor::processPhaseModel(juce::AudioBuffer<float>& wetBuffer,
                                                  const juce::AudioBuffer<float>& dryInput)
{
    if (wetBuffer.getNumSamples() <= 0)
        return;

    if (currentPhaseMode != PhaseMode::natural || currentPhaseLatencySamples <= 0)
        return;

    const int numChannels = juce::jmin(2, wetBuffer.getNumChannels());
    const int numSamples = wetBuffer.getNumSamples();
    const int delay = currentPhaseLatencySamples;

    for (int ch = 0; ch < numChannels; ++ch)
    {
        auto* wet = wetBuffer.getWritePointer(ch);
        const auto* dry = dryInput.getReadPointer(ch);
        auto& delayLine = naturalPhaseDelayLines[static_cast<size_t>(ch)];

        int writePos = naturalPhaseWritePos;
        for (int i = 0; i < numSamples; ++i)
        {
            const int readPos = (writePos - delay + maxNaturalPhaseDelaySamples) % maxNaturalPhaseDelaySamples;
            const float delayedWet = delayLine[static_cast<size_t>(readPos)];

            delayLine[static_cast<size_t>(writePos)] = wet[i];

            wet[i] = delayedWet * 0.8f + dry[i] * 0.2f;

            writePos = (writePos + 1) % maxNaturalPhaseDelaySamples;
        }
    }

    naturalPhaseWritePos = (naturalPhaseWritePos + numSamples) % maxNaturalPhaseDelaySamples;
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
