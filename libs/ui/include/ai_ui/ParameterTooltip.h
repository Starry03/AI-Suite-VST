#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

//==============================================================================
/**
 * Tooltip che mostra i parametri del filtro durante l'interazione.
 */
class ParameterTooltip : public juce::Component
{
public:
    ParameterTooltip();
    
    void paint(juce::Graphics& g) override;
    
    // Aggiorna i valori mostrati
    void setParameters(float frequency, float gain, float q, const juce::String& filterType);
    
    // Mostra il tooltip in una posizione specifica
    void showAt(juce::Point<int> position);
    
    // Nascondi il tooltip
    void hide();
    
private:
    float currentFreq = 1000.0f;
    float currentGain = 0.0f;
    float currentQ = 0.707f;
    juce::String filterTypeName = "Bell";
    
    float alpha = 0.0f; // Per fade in/out
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ParameterTooltip)
};
