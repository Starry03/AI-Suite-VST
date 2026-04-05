#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "../DSP/FilterBase.h"

//==============================================================================
/**
 * Componente che rappresenta un singolo nodo filtro sulla curva.
 * Visualizza e permette di modificare i parametri del filtro.
 */
class FilterNode : public juce::Component
{
public:
    FilterNode(FilterBase* filter, int filterIndex);
    ~FilterNode() override;
    
    void paint(juce::Graphics& g) override;
    void resized() override;
    
    void mouseDown(const juce::MouseEvent& event) override;
    void mouseDrag(const juce::MouseEvent& event) override;
    void mouseUp(const juce::MouseEvent& event) override;
    void mouseEnter(const juce::MouseEvent& event) override;
    void mouseExit(const juce::MouseEvent& event) override;
    
    // Posizione del nodo in coordinate schermo
    void setNodePosition(juce::Point<float> pos);
    juce::Point<float> getNodePosition() const { return nodePosition; }
    
private:
    FilterBase* filterPtr;
    int index;
    juce::Point<float> nodePosition;
    bool isHovering = false;
    bool isDragging = false;
    
    static constexpr float nodeRadius = 8.0f;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(FilterNode)
};
