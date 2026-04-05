#include "FrequencyResponseCurve.h"
#include <ai_ui/ModernLookAndFeel.h>
#include <ai_ui/ParameterTooltip.h>

FrequencyResponseCurve::FrequencyResponseCurve(FilterChain& fc, juce::AudioProcessorValueTreeState& apvts,
                                               AudioPluginAudioProcessor& processor)
    : filterChain(fc), apvtsRef(apvts), processorRef(processor)
{
    startTimerHz(30); // 30 FPS for maximum fluidity and precision
    
    // Crea tooltip
    tooltip = std::make_unique<ParameterTooltip>();
    addAndMakeVisible(tooltip.get());
    tooltip->setVisible(false);
    
    // Crea spectrum analyzer
    spectrumAnalyzer = std::make_unique<SpectrumAnalyzer>();
}

FrequencyResponseCurve::~FrequencyResponseCurve()
{
    stopTimer();
}

void FrequencyResponseCurve::paint(juce::Graphics& g)
{
    auto bounds = getLocalBounds();
    
    // Background
    g.fillAll(ModernLookAndFeel::ColorScheme::backgroundDark);
    
    // Draw grid
    drawGrid(g);
    
    // Draw spectrum analyzer BEHIND the curve
    drawSpectrum(g);
    
    // Calculate and draw response curve
    auto response = calculateFrequencyResponse();
    drawResponseCurve(g, response);
    
    // Draw filter nodes
    drawFilterNodes(g);
    
    // Draw frequency labels
    drawFrequencyLabels(g);
}

void FrequencyResponseCurve::resized()
{
}

void FrequencyResponseCurve::timerCallback()
{
    // Pull audio samples from FIFO and feed to spectrum analyzer
    auto& fifo = processorRef.getAudioFifo();
    auto* fifoBuffer = processorRef.getAudioFifoBuffer();
    
    int numReady = fifo.getNumReady();
    if (numReady > 0)
    {
        int start1, size1, start2, size2;
        fifo.prepareToRead(numReady, start1, size1, start2, size2);
        
        if (size1 > 0)
            spectrumAnalyzer->pushSamples(fifoBuffer + start1, size1);
        if (size2 > 0)
            spectrumAnalyzer->pushSamples(fifoBuffer + start2, size2);
        
        fifo.finishedRead(size1 + size2);
    }
    
    // Repaint only when there's new spectrum data or when dragging
    bool shouldRepaint = false;
    
    if (spectrumAnalyzer->hasNewData())
    {
        spectrumNeedsUpdate = true;
        shouldRepaint = true;
    }
    
    // Always repaint when dragging for smooth interaction
    if (isDragging)
    {
        responseNeedsUpdate = true;
        shouldRepaint = true;
    }
    
    if (shouldRepaint)
        repaint();
}

std::vector<float> FrequencyResponseCurve::calculateFrequencyResponse()
{
    std::vector<float> response;
    auto width = getWidth();
    response.reserve(width);
    
    // Calcola la risposta per ogni pixel orizzontale
    for (int x = 0; x < width; ++x)
    {
        float freq = xToFrequency(static_cast<float>(x));
        float gain = filterChain.getTotalFrequencyResponse(freq);
        response.push_back(gain);
    }
    
    return response;
}

float FrequencyResponseCurve::xToFrequency(float x) const
{
    auto width = getWidth();
    float normalized = x / width;
    // Scala logaritmica
    return minFreq * std::pow(maxFreq / minFreq, normalized);
}

float FrequencyResponseCurve::frequencyToX(float freq) const
{
    auto width = getWidth();
    float normalized = std::log(freq / minFreq) / std::log(maxFreq / minFreq);
    return normalized * width;
}

float FrequencyResponseCurve::yToGain(float y) const
{
    auto height = getHeight();
    float normalized = y / height;
    return maxGain - (normalized * (maxGain - minGain));
}

float FrequencyResponseCurve::gainToY(float gain) const
{
    auto height = getHeight();
    float normalized = (maxGain - gain) / (maxGain - minGain);
    return normalized * height;
}

void FrequencyResponseCurve::drawGrid(juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat();
    
    // Linee verticali (frequenze)
    g.setColour(ModernLookAndFeel::ColorScheme::gridLine);
    for (float freq : gridFrequencies)
    {
        float x = frequencyToX(freq);
        g.drawVerticalLine(static_cast<int>(x), bounds.getY(), bounds.getBottom());
    }
    
    // Linee orizzontali (gain)
    std::array<float, 5> gainLines = {-12.0f, -6.0f, 0.0f, 6.0f, 12.0f};
    for (float gain : gainLines)
    {
        float y = gainToY(gain);
        
        // Linea a 0 dB più marcata
        if (std::abs(gain) < 0.1f)
        {
            g.setColour(ModernLookAndFeel::ColorScheme::gridLine.brighter(0.3f));
            g.drawHorizontalLine(static_cast<int>(y), bounds.getX(), bounds.getRight());
            g.setColour(ModernLookAndFeel::ColorScheme::gridLine);
        }
        else
        {
            g.drawHorizontalLine(static_cast<int>(y), bounds.getX(), bounds.getRight());
        }
    }
}

void FrequencyResponseCurve::drawResponseCurve(juce::Graphics& g, const std::vector<float>& response)
{
    if (response.empty())
        return;
    
    juce::Path curvePath;
    auto bounds = getLocalBounds().toFloat();
    
    // Crea il path della curva
    bool firstPoint = true;
    for (size_t i = 0; i < response.size(); ++i)
    {
        float x = static_cast<float>(i);
        float y = gainToY(response[i]);
        
        if (firstPoint)
        {
            curvePath.startNewSubPath(x, y);
            firstPoint = false;
        }
        else
        {
            curvePath.lineTo(x, y);
        }
    }
    
    // Fill sotto la curva con gradient
    juce::Path fillPath = curvePath;
    fillPath.lineTo(bounds.getRight(), gainToY(0.0f));
    fillPath.lineTo(bounds.getX(), gainToY(0.0f));
    fillPath.closeSubPath();
    
    juce::ColourGradient gradient(
        ModernLookAndFeel::ColorScheme::accent.withAlpha(0.3f), 
        bounds.getCentreX(), gainToY(12.0f),
        ModernLookAndFeel::ColorScheme::accent.withAlpha(0.05f), 
        bounds.getCentreX(), gainToY(-12.0f),
        false);
    
    g.setGradientFill(gradient);
    g.fillPath(fillPath);
    
    // Disegna la curva con glow
    g.setColour(ModernLookAndFeel::ColorScheme::accent.withAlpha(0.2f));
    g.strokePath(curvePath, juce::PathStrokeType(3.0f));
    
    g.setColour(ModernLookAndFeel::ColorScheme::accentBright);
    g.strokePath(curvePath, juce::PathStrokeType(1.5f));
}

void FrequencyResponseCurve::drawFrequencyLabels(juce::Graphics& g)
{
    auto bounds = getLocalBounds();
    g.setFont(juce::FontOptions(10.0f));
    g.setColour(ModernLookAndFeel::ColorScheme::textDim);
    
    // More space for labels at the edges
    const std::vector<float> frequencies = {20.0f, 50.0f, 100.0f, 200.0f, 500.0f, 
                                           1000.0f, 2000.0f, 5000.0f, 10000.0f, 20000.0f};
    
    for (auto freq : frequencies)
    {
        float x = frequencyToX(freq);
        
        // Format frequency as integer (5K instead of 5.0K)
        juce::String label;
        if (freq >= 1000.0f)
        {
            int kHz = static_cast<int>(freq / 1000.0f);
            label = juce::String(kHz) + "K";
        }
        else
        {
            label = juce::String(static_cast<int>(freq));
        }
        
        // Adjust position for edge labels
        int labelWidth = 30;
        int labelX = static_cast<int>(x) - labelWidth / 2;
        
        // Special handling for 20Hz and 20K to ensure they're visible
        if (freq == 20.0f)
            labelX = 5;  // Left edge
        else if (freq == 20000.0f)
            labelX = bounds.getWidth() - 35;  // Right edge with more space
        
        g.drawText(label, labelX, bounds.getHeight() - 18, labelWidth, 12, 
                  juce::Justification::centred, false);
    }
}

void FrequencyResponseCurve::mouseDown(const juce::MouseEvent& event)
{
    auto mousePos = event.getPosition().toFloat();

    // Check if clicking on a filter node
    selectedFilterIndex = findFilterNearMouse(mousePos);

    if (selectedFilterIndex >= 0)
    {
        isDragging = true;
        dragStartPosition = mousePos;
        dragStartFrequency = getFilterParameter(selectedFilterIndex, "freq");
        dragStartGain = getFilterParameter(selectedFilterIndex, "gain");

        // Show tooltip
        auto filterType = getFilterTypeName(selectedFilterIndex);
        auto q = getFilterParameter(selectedFilterIndex, "q");
        tooltip->setParameters(dragStartFrequency, dragStartGain, q, filterType);
        tooltip->showAt(event.getPosition());

        // Notify control panel of selection
        if (onFilterSelected)
            onFilterSelected(selectedFilterIndex);
    }
    else
    {
        // Clicked on empty area: deselect
        if (onFilterSelected)
            onFilterSelected(-1);
    }

    repaint();
}

void FrequencyResponseCurve::mouseDrag(const juce::MouseEvent& event)
{
    if (!isDragging || selectedFilterIndex < 0)
        return;
    
    auto currentPos = event.getPosition().toFloat();
    auto delta = currentPos - dragStartPosition;
    
    // Horizontal drag → Frequency
    float newFrequency = xToFrequency(frequencyToX(dragStartFrequency) + delta.x);
    newFrequency = juce::jlimit(minFreq, maxFreq, newFrequency);
    
    // Vertical drag → Gain
    float newGain = yToGain(gainToY(dragStartGain) + delta.y);
    newGain = juce::jlimit(minGain, maxGain, newGain);
    
    // Update parameters
    updateFilterParameter(selectedFilterIndex, "freq", newFrequency);
    updateFilterParameter(selectedFilterIndex, "gain", newGain);
    
    // Mark response curve as needing recalculation
    responseNeedsUpdate = true;
    
    // Update tooltip
    auto filterType = getFilterTypeName(selectedFilterIndex);
    auto q = getFilterParameter(selectedFilterIndex, "q");
    tooltip->setParameters(newFrequency, newGain, q, filterType);
    tooltip->showAt(event.getPosition());
    
    repaint();
}

void FrequencyResponseCurve::mouseUp(const juce::MouseEvent& event)
{
    isDragging = false;
    tooltip->hide();
    repaint();
}

void FrequencyResponseCurve::mouseMove(const juce::MouseEvent& event)
{
    auto mousePos = event.getPosition().toFloat();
    int newHoveredIndex = findFilterNearMouse(mousePos);
    
    if (newHoveredIndex != hoveredFilterIndex)
    {
        hoveredFilterIndex = newHoveredIndex;
        
        // Change cursor based on hover
        if (hoveredFilterIndex >= 0)
            setMouseCursor(juce::MouseCursor::DraggingHandCursor);
        else
            setMouseCursor(juce::MouseCursor::NormalCursor);
        
        repaint();
    }
}

void FrequencyResponseCurve::mouseExit(const juce::MouseEvent&)
{
    hoveredFilterIndex = -1;
    setMouseCursor(juce::MouseCursor::NormalCursor);
    repaint();
}

void FrequencyResponseCurve::mouseDoubleClick(const juce::MouseEvent& event)
{
    // Double click to add new filter
    if (selectedFilterIndex < 0) // Not on existing node
    {
        addFilterAtMouse(event.getPosition().toFloat());
    }
}

void FrequencyResponseCurve::mouseWheelMove(const juce::MouseEvent& event, const juce::MouseWheelDetails& wheel)
{
    if (hoveredFilterIndex >= 0 || selectedFilterIndex >= 0)
    {
        int filterIndex = selectedFilterIndex >= 0 ? selectedFilterIndex : hoveredFilterIndex;
        
        // Mouse wheel → Q factor
        float currentQ = getFilterParameter(filterIndex, "q");
        float deltaQ = wheel.deltaY * (wheel.isReversed ? -1.0f : 1.0f) * 0.5f;
        float newQ = juce::jlimit(0.1f, 10.0f, currentQ + deltaQ);
        
        updateFilterParameter(filterIndex, "q", newQ);
        
        // Mark response curve as needing recalculation
        responseNeedsUpdate = true;
        
        // Show tooltip with updated Q
        auto freq = getFilterParameter(filterIndex, "freq");
        auto gain = getFilterParameter(filterIndex, "gain");
        auto filterType = getFilterTypeName(filterIndex);
        tooltip->setParameters(freq, gain, newQ, filterType);
        tooltip->showAt(event.getPosition());
        
        repaint();
    }
}

int FrequencyResponseCurve::findFilterNearMouse(juce::Point<float> mousePos)
{
    float minDistance = nodeRadius * 2.0f;
    int closestIndex = -1;
    
    for (int i = 0; i < maxFilters; ++i)
    {
        // Check if filter is enabled
        if (getFilterParameter(i, "enabled") < 0.5f)
            continue;
        
        auto nodePos = getFilterNodePosition(i);
        float distance = mousePos.getDistanceFrom(nodePos);
        
        if (distance < minDistance)
        {
            minDistance = distance;
            closestIndex = i;
        }
    }
    
    return closestIndex;
}

void FrequencyResponseCurve::addFilterAtMouse(juce::Point<float> mousePos)
{
    // Find first disabled filter
    for (int i = 0; i < maxFilters; ++i)
    {
        if (getFilterParameter(i, "enabled") < 0.5f)
        {
            // Enable and position the filter
            updateFilterParameter(i, "enabled", 1.0f);
            
            float freq = xToFrequency(mousePos.x);
            float gain = yToGain(mousePos.y);
            
            updateFilterParameter(i, "freq", freq);
            updateFilterParameter(i, "gain", gain);
            updateFilterParameter(i, "q", 0.707f);
            updateFilterParameter(i, "type", 2.0f); // Bell filter
            
            repaint();
            break;
        }
    }
}

void FrequencyResponseCurve::showContextMenu(int filterIndex)
{
    juce::PopupMenu menu;
    
    // Submenu for filter type
    juce::PopupMenu typeMenu;
    typeMenu.addItem(1, "Low Pass", true, getFilterParameter(filterIndex, "type") == 0);
    typeMenu.addItem(2, "High Pass", true, getFilterParameter(filterIndex, "type") == 1);
    typeMenu.addItem(3, "Bell", true, getFilterParameter(filterIndex, "type") == 2);
    typeMenu.addItem(4, "Low Shelf", true, getFilterParameter(filterIndex, "type") == 3);
    typeMenu.addItem(5, "High Shelf", true, getFilterParameter(filterIndex, "type") == 4);
    typeMenu.addItem(6, "Notch", true, getFilterParameter(filterIndex, "type") == 5);
    
    menu.addSubMenu("Change Type", typeMenu);
    menu.addSeparator();
    menu.addItem(10, "Reset Parameters");
    menu.addItem(11, "Remove Filter");
    
    menu.showMenuAsync(juce::PopupMenu::Options(), [this, filterIndex](int result) {
        if (result >= 1 && result <= 6)
        {
            // Change filter type
            updateFilterParameter(filterIndex, "type", static_cast<float>(result - 1));
        }
        else if (result == 10)
        {
            // Reset parameters
            updateFilterParameter(filterIndex, "freq", 1000.0f);
            updateFilterParameter(filterIndex, "gain", 0.0f);
            updateFilterParameter(filterIndex, "q", 0.707f);
        }
        else if (result == 11)
        {
            // Remove filter
            updateFilterParameter(filterIndex, "enabled", 0.0f);
        }
        repaint();
    });
}

juce::Point<float> FrequencyResponseCurve::getFilterNodePosition(int filterIndex)
{
    float freq = getFilterParameter(filterIndex, "freq");
    float gain = getFilterParameter(filterIndex, "gain");
    
    float x = frequencyToX(freq);
    float y = gainToY(gain);
    
    return juce::Point<float>(x, y);
}

void FrequencyResponseCurve::drawFilterNodes(juce::Graphics& g)
{
    for (int i = 0; i < maxFilters; ++i)
    {
        // Skip disabled filters
        if (getFilterParameter(i, "enabled") < 0.5f)
            continue;
        
        auto nodePos = getFilterNodePosition(i);
        bool isSelected = (i == selectedFilterIndex);
        bool isHovered = (i == hoveredFilterIndex);
        
        // Glow effect for hover/selection
        if (isHovered || isSelected)
        {
            for (int j = 4; j > 0; --j)
            {
                float alpha = (isSelected ? 0.15f : 0.08f) * (5 - j);
                g.setColour(ModernLookAndFeel::ColorScheme::accent.withAlpha(alpha));
                g.fillEllipse(nodePos.x - nodeRadius - j * 2, 
                             nodePos.y - nodeRadius - j * 2,
                             (nodeRadius + j * 2) * 2,
                             (nodeRadius + j * 2) * 2);
            }
        }
        
        // Outer ring
        float outerRadius = isSelected ? nodeRadius * 1.4f : nodeRadius * 1.2f;
        g.setColour(ModernLookAndFeel::ColorScheme::accent.withAlpha(0.6f));
        g.fillEllipse(nodePos.x - outerRadius, nodePos.y - outerRadius,
                     outerRadius * 2, outerRadius * 2);
        
        // Inner circle
        g.setColour(ModernLookAndFeel::ColorScheme::accentBright);
        g.fillEllipse(nodePos.x - nodeRadius, nodePos.y - nodeRadius,
                     nodeRadius * 2, nodeRadius * 2);
        
        // Center dot
        float centerRadius = nodeRadius * 0.4f;
        g.setColour(ModernLookAndFeel::ColorScheme::background);
        g.fillEllipse(nodePos.x - centerRadius, nodePos.y - centerRadius,
                     centerRadius * 2, centerRadius * 2);
    }
}

void FrequencyResponseCurve::updateFilterParameter(int filterIndex, const juce::String& paramName, float value)
{
    juce::String paramID = "filter" + juce::String(filterIndex) + "_" + paramName;
    
    if (auto* param = apvtsRef.getParameter(paramID))
    {
        float normalized = param->convertTo0to1(value);
        param->setValueNotifyingHost(normalized);
    }
}

float FrequencyResponseCurve::getFilterParameter(int filterIndex, const juce::String& paramName)
{
    juce::String paramID = "filter" + juce::String(filterIndex) + "_" + paramName;
    
    if (auto* param = apvtsRef.getRawParameterValue(paramID))
        return param->load();
    
    return 0.0f;
}

juce::String FrequencyResponseCurve::getFilterTypeName(int filterIndex)
{
    int typeIndex = static_cast<int>(getFilterParameter(filterIndex, "type"));
    
    switch (typeIndex)
    {
        case 0: return "Low Pass";
        case 1: return "High Pass";
        case 2: return "Bell";
        case 3: return "Low Shelf";
        case 4: return "High Shelf";
        case 5: return "Notch";
        default: return "Bell";
    }
}

void FrequencyResponseCurve::drawSpectrum(juce::Graphics& g)
{
    if (!spectrumAnalyzer)
        return;
    
    // Rebuild path only if new data available
    if (spectrumNeedsUpdate && spectrumAnalyzer->hasNewData())
    {
        auto bounds = getLocalBounds().toFloat();
        auto width = bounds.getWidth();
        
        cachedSpectrumPath.clear();
        bool firstPoint = true;
        
        // Sample every 2 pixels instead of every pixel for performance
        // This reduces the loop iterations by 50% with minimal visual quality loss
        for (int x = 0; x < width; x += 2)
        {
            float freq = xToFrequency(static_cast<float>(x));
            float magnitude = spectrumAnalyzer->getMagnitudeForFrequency(freq);
            
            // Map magnitude (-60 to 0 dB) to screen Y
            float y = gainToY(magnitude * 0.4f); // Scale down to fit in gain range
            
            if (firstPoint)
            {
                cachedSpectrumPath.startNewSubPath(x, bounds.getBottom());
                cachedSpectrumPath.lineTo(x, y);
                firstPoint = false;
            }
            else
            {
                cachedSpectrumPath.lineTo(x, y);
            }
        }
        
        // Close path at bottom
        cachedSpectrumPath.lineTo(width, bounds.getBottom());
        cachedSpectrumPath.closeSubPath();
        
        spectrumAnalyzer->clearNewDataFlag();
        spectrumNeedsUpdate = false;
    }
    
    // Draw cached path
    if (!cachedSpectrumPath.isEmpty())
    {
        auto bounds = getLocalBounds().toFloat();
        auto width = bounds.getWidth();
        
        // Create gradient for spectrum bars
        juce::ColourGradient gradient(
            juce::Colour(80, 40, 120).withAlpha(0.3f),  // Dark purple (low freqs)
            0, gainToY(0.0f),
            juce::Colour(40, 200, 255).withAlpha(0.3f), // Bright cyan (high freqs)
            width, gainToY(0.0f),
            false);
        
        // Fill with gradient
        g.setGradientFill(gradient);
        g.fillPath(cachedSpectrumPath);
        
        // Optional: add subtle outline
        g.setColour(juce::Colour(100, 150, 255).withAlpha(0.2f));
        g.strokePath(cachedSpectrumPath, juce::PathStrokeType(1.0f));
    }
}

void FrequencyResponseCurve::deselectFilter()
{
    selectedFilterIndex = -1;
    hoveredFilterIndex = -1;
    isDragging = false;
    repaint();
}
