#include "ParameterHelper.h"

namespace ParameterHelper
{
    void addFilterParameters(juce::AudioProcessorValueTreeState::ParameterLayout& layout,
                            int filterIndex,
                            FilterType filterType)
    {
        juce::String prefix = "filter" + juce::String(filterIndex) + "_";
        
        // Parametro: Enabled
        layout.add(std::make_unique<juce::AudioParameterBool>(
            prefix + "enabled",
            "Filter " + juce::String(filterIndex + 1) + " Enabled",
            filterIndex == 0)); // Solo il primo filtro è abilitato di default
        
        // Parametro: Type
        layout.add(std::make_unique<juce::AudioParameterChoice>(
            prefix + "type",
            "Filter " + juce::String(filterIndex + 1) + " Type",
            juce::StringArray{"Low Pass", "High Pass", "Bell", "Low Shelf", "High Shelf", "Notch"},
            2)); // Default: Bell
        
        // Parametro: Frequency
        layout.add(std::make_unique<juce::AudioParameterFloat>(
            prefix + "freq",
            "Filter " + juce::String(filterIndex + 1) + " Frequency",
            Ranges::getFrequencyRange(),
            1000.0f,
            juce::String(),
            juce::AudioProcessorParameter::genericParameter,
            [](float value, int) { return juce::String(value, 1) + " Hz"; }));
        
        // Parametro: Gain (per Bell e Shelf)
        layout.add(std::make_unique<juce::AudioParameterFloat>(
            prefix + "gain",
            "Filter " + juce::String(filterIndex + 1) + " Gain",
            Ranges::getGainRange(),
            0.0f,
            juce::String(),
            juce::AudioProcessorParameter::genericParameter,
            [](float value, int) { return juce::String(value, 1) + " dB"; }));
        
        // Parametro: Q
        layout.add(std::make_unique<juce::AudioParameterFloat>(
            prefix + "q",
            "Filter " + juce::String(filterIndex + 1) + " Q",
            Ranges::getQRange(),
            0.707f,
            juce::String(),
            juce::AudioProcessorParameter::genericParameter,
            [](float value, int) { return juce::String(value, 2); }));
        
        // Parametro: Pan
        layout.add(std::make_unique<juce::AudioParameterFloat>(
            prefix + "pan",
            "Filter " + juce::String(filterIndex + 1) + " Pan",
            Ranges::getPanRange(),
            0.0f,  // Default: center
            juce::String(),
            juce::AudioProcessorParameter::genericParameter,
            [](float value, int) { 
                if (value == 0.0f) return juce::String("C");
                else if (value < 0.0f) return juce::String(static_cast<int>(-value)) + "L";  // Remove minus
                else return juce::String(static_cast<int>(value)) + "R";
            }));
        
        // Parametro: Slope
        layout.add(std::make_unique<juce::AudioParameterChoice>(
            prefix + "slope",
            "Filter " + juce::String(filterIndex + 1) + " Slope",
            juce::StringArray{"6 dB/oct", "12 dB/oct", "24 dB/oct", "48 dB/oct", "96 dB/oct"},
            1));  // Default: 12 dB/oct

        // Parametro: Dynamic Threshold
        layout.add(std::make_unique<juce::AudioParameterFloat>(
            prefix + "dyn_threshold",
            "Filter " + juce::String(filterIndex + 1) + " Dynamic Threshold",
            juce::NormalisableRange<float>(-60.0f, 0.0f, 0.1f),
            -24.0f,
            juce::String(),
            juce::AudioProcessorParameter::genericParameter,
            [](float value, int) { return juce::String(value, 1) + " dB"; }));

        // Parametro: Dynamic Gain Amount (0 = off)
        layout.add(std::make_unique<juce::AudioParameterFloat>(
            prefix + "dyn_gain",
            "Filter " + juce::String(filterIndex + 1) + " Dynamic Gain",
            juce::NormalisableRange<float>(0.0f, 24.0f, 0.1f),
            0.0f,
            juce::String(),
            juce::AudioProcessorParameter::genericParameter,
            [](float value, int) { return juce::String(value, 1) + " dB"; }));

        layout.add(std::make_unique<juce::AudioParameterChoice>(
            prefix + "dyn_mode",
            "Filter " + juce::String(filterIndex + 1) + " Dynamic Mode",
            juce::StringArray{"Auto", "Cut", "Boost"},
            0));

        layout.add(std::make_unique<juce::AudioParameterFloat>(
            prefix + "dyn_attack_ms",
            "Filter " + juce::String(filterIndex + 1) + " Dynamic Attack",
            juce::NormalisableRange<float>(1.0f, 250.0f, 0.1f),
            20.0f,
            juce::String(),
            juce::AudioProcessorParameter::genericParameter,
            [](float value, int) { return juce::String(value, 1) + " ms"; }));

        layout.add(std::make_unique<juce::AudioParameterFloat>(
            prefix + "dyn_release_ms",
            "Filter " + juce::String(filterIndex + 1) + " Dynamic Release",
            juce::NormalisableRange<float>(10.0f, 1000.0f, 0.1f),
            180.0f,
            juce::String(),
            juce::AudioProcessorParameter::genericParameter,
            [](float value, int) { return juce::String(value, 1) + " ms"; }));

        layout.add(std::make_unique<juce::AudioParameterFloat>(
            prefix + "dyn_detector_q",
            "Filter " + juce::String(filterIndex + 1) + " Detector Q",
            juce::NormalisableRange<float>(0.3f, 8.0f, 0.01f),
            1.0f,
            juce::String(),
            juce::AudioProcessorParameter::genericParameter,
            [](float value, int) { return juce::String(value, 2); }));
    }
    
    juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout(int numFilters)
    {
        juce::AudioProcessorValueTreeState::ParameterLayout layout;
        
        // Parametri globali
        layout.add(std::make_unique<juce::AudioParameterFloat>(
            "output_gain",
            "Output Gain",
            juce::NormalisableRange<float>(-24.0f, 24.0f, 0.1f),
            0.0f,
            juce::String(),
            juce::AudioProcessorParameter::genericParameter,
            [](float value, int) { return juce::String(value, 1) + " dB"; }));
        
        // Global Auto-Gain
        layout.add(std::make_unique<juce::AudioParameterBool>(
            "auto_gain",
            "Auto Gain",
            false));

        // Global Sidechain Enable
        layout.add(std::make_unique<juce::AudioParameterBool>(
            "sidechain_enabled",
            "Sidechain Enabled",
            false));

        // Global phase mode
        layout.add(std::make_unique<juce::AudioParameterChoice>(
            "phase_mode",
            "Phase Mode",
            juce::StringArray{"Minimum", "Natural", "Linear Phase"},
            0));

        layout.add(std::make_unique<juce::AudioParameterChoice>(
            "linear_phase_quality",
            "Linear Phase Quality",
            juce::StringArray{"Low", "Mid", "High"},
            1));

        // Crea parametri per ogni filtro
        for (int i = 0; i < numFilters; ++i)
        {
            addFilterParameters(layout, i, FilterType::Bell);
        }
        
        return layout;
    }
    
    juce::String getParameterID(int filterIndex, const juce::String& paramName)
    {
        return "filter" + juce::String(filterIndex) + "_" + paramName;
    }
    
    juce::String getParameterName(int filterIndex, const juce::String& paramName)
    {
        return "Filter " + juce::String(filterIndex + 1) + " " + paramName;
    }
}
