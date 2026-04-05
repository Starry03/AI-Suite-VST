#include "FilterNode.h"
#include <ai_ui/ModernLookAndFeel.h>

FilterNode::FilterNode(FilterBase* filter, int filterIndex)
    : filterPtr(filter), index(filterIndex)
{
    setSize(nodeRadius * 4, nodeRadius * 4);
}

FilterNode::~FilterNode()
{
}

void FilterNode::paint(juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat();
    auto center = bounds.getCentre();
    
    // Glow effect quando in hover o dragging
    if (isHovering || isDragging)
    {
        for (int i = 3; i > 0; --i)
        {
            float alpha = 0.1f * (4 - i);
            g.setColour(ModernLookAndFeel::ColorScheme::accent.withAlpha(alpha));
            g.fillEllipse(center.x - nodeRadius - i * 2, 
                         center.y - nodeRadius - i * 2,
                         (nodeRadius + i * 2) * 2,
                         (nodeRadius + i * 2) * 2);
        }
    }
    
    // Outer ring
    g.setColour(ModernLookAndFeel::ColorScheme::accent.withAlpha(0.5f));
    g.fillEllipse(center.x - nodeRadius * 1.2f, 
                 center.y - nodeRadius * 1.2f,
                 nodeRadius * 2.4f,
                 nodeRadius * 2.4f);
    
    // Inner circle
    g.setColour(ModernLookAndFeel::ColorScheme::accentBright);
    g.fillEllipse(center.x - nodeRadius, 
                 center.y - nodeRadius,
                 nodeRadius * 2,
                 nodeRadius * 2);
    
    // Centro
    g.setColour(ModernLookAndFeel::ColorScheme::background);
    g.fillEllipse(center.x - nodeRadius * 0.4f, 
                 center.y - nodeRadius * 0.4f,
                 nodeRadius * 0.8f,
                 nodeRadius * 0.8f);
}

void FilterNode::resized()
{
}

void FilterNode::mouseDown(const juce::MouseEvent& event)
{
    juce::ignoreUnused(event);
    isDragging = true;
    repaint();
}

void FilterNode::mouseDrag(const juce::MouseEvent& event)
{
    // Aggiorna la posizione durante il drag
    auto newPos = nodePosition + event.getOffsetFromDragStart().toFloat();
    setNodePosition(newPos);
    
    // TODO: aggiornare i parametri del filtro basandosi sulla nuova posizione
}

void FilterNode::mouseUp(const juce::MouseEvent& event)
{
    juce::ignoreUnused(event);
    isDragging = false;
    repaint();
}

void FilterNode::mouseEnter(const juce::MouseEvent& event)
{
    juce::ignoreUnused(event);
    isHovering = true;
    repaint();
}

void FilterNode::mouseExit(const juce::MouseEvent& event)
{
    juce::ignoreUnused(event);
    isHovering = false;
    repaint();
}

void FilterNode::setNodePosition(juce::Point<float> pos)
{
    nodePosition = pos;
    setCentrePosition(pos.toInt());
}
