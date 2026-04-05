#include <ai_ui/ParameterTooltip.h>
#include <ai_ui/ModernLookAndFeel.h>

ParameterTooltip::ParameterTooltip()
{
    setSize(180, 80);
    setAlwaysOnTop(true);
    setVisible(false);
}

void ParameterTooltip::paint(juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat();
    
    // Background semi-trasparente con blur
    g.setColour(ModernLookAndFeel::ColorScheme::backgroundDark.withAlpha(0.95f * alpha));
    g.fillRoundedRectangle(bounds, 8.0f);
    
    // Border con glow
    g.setColour(ModernLookAndFeel::ColorScheme::accent.withAlpha(0.6f * alpha));
    g.drawRoundedRectangle(bounds.reduced(0.5f), 8.0f, 2.0f);
    
    // Glow esterno
    for (int i = 1; i <= 3; ++i)
    {
        float glowAlpha = (0.1f * (4 - i)) * alpha;
        g.setColour(ModernLookAndFeel::ColorScheme::accent.withAlpha(glowAlpha));
        g.drawRoundedRectangle(bounds.expanded(i * 2), 8.0f + i, 1.0f);
    }
    
    // Testo
    g.setColour(ModernLookAndFeel::ColorScheme::text.withAlpha(alpha));
    
    auto textBounds = bounds.reduced(12, 8);
    
    // Filter type (header)
    g.setFont(juce::FontOptions(14.0f, juce::Font::bold));
    g.drawText(filterTypeName, textBounds.removeFromTop(20), juce::Justification::centred);
    
    textBounds.removeFromTop(4); // Spacing
    
    // Parameters
    g.setFont(juce::FontOptions(11.0f));

    // Frequency
    juce::String freqText = currentFreq >= 1000.0f 
        ? juce::String(currentFreq / 1000.0f, 2) + " kHz"
        : juce::String(currentFreq, 1) + " Hz";
    g.drawText("Freq: " + freqText, textBounds.removeFromTop(16), juce::Justification::left);
    
    // Gain
    g.drawText("Gain: " + juce::String(currentGain, 1) + " dB", 
              textBounds.removeFromTop(16), juce::Justification::left);
    
    // Q
    g.drawText("Q: " + juce::String(currentQ, 2), 
              textBounds.removeFromTop(16), juce::Justification::left);
}

void ParameterTooltip::setParameters(float frequency, float gain, float q, const juce::String& filterType)
{
    currentFreq = frequency;
    currentGain = gain;
    currentQ = q;
    filterTypeName = filterType;
    repaint();
}

void ParameterTooltip::showAt(juce::Point<int> position)
{
    // Posiziona il tooltip vicino al mouse, ma evita i bordi
    auto parent = getParentComponent();
    if (parent == nullptr)
        return;
    
    int x = position.x + 15;
    int y = position.y - getHeight() / 2;
    
    // Evita bordi
    if (x + getWidth() > parent->getWidth())
        x = position.x - getWidth() - 15;
    if (y < 0)
        y = 0;
    if (y + getHeight() > parent->getHeight())
        y = parent->getHeight() - getHeight();
    
    setTopLeftPosition(x, y);
    setVisible(true);
    
    // Fade in
    alpha = 1.0f;
    repaint();
}

void ParameterTooltip::hide()
{
    alpha = 0.0f;
    setVisible(false);
}
