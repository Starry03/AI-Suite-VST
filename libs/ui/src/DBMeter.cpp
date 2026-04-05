#include <ai_ui/DBMeter.h>
#include <ai_ui/ModernLookAndFeel.h>

DBMeter::DBMeter()
{
    startTimerHz(30);
}

DBMeter::~DBMeter()
{
    stopTimer();
}

void DBMeter::paint(juce::Graphics& g)
{
    auto bounds = getLocalBounds().reduced(2);

    // Split: labels on the left, meter bar on the right.
    auto labelArea = bounds.removeFromLeft(28);
    auto meterArea = bounds;

    // Background
    g.setColour(ModernLookAndFeel::ColorScheme::backgroundDark);
    g.fillRoundedRectangle(meterArea.toFloat(), 4.0f);

    // dB range: -60 to +6
    float normalizedLevel = juce::jmap(currentMeterDB, -60.0f, 6.0f, 0.0f, 1.0f);
    normalizedLevel = juce::jlimit(0.0f, 1.0f, normalizedLevel);
    
    auto fillHeight = static_cast<int>(meterArea.getHeight() * normalizedLevel);
    auto fillBounds = meterArea.withTrimmedTop(meterArea.getHeight() - fillHeight);

    // Gradient: green -> yellow -> red
    juce::ColourGradient gradient(
        juce::Colour(0xff00ff00),  // Green at bottom
        static_cast<float>(meterArea.getCentreX()), static_cast<float>(meterArea.getBottom()),
        juce::Colour(0xffff0000),  // Red at top
        static_cast<float>(meterArea.getCentreX()), static_cast<float>(meterArea.getY()),
        false);
    gradient.addColour(0.7, juce::Colour(0xffffff00));  // Yellow in middle
    
    g.setGradientFill(gradient);
    g.fillRoundedRectangle(fillBounds.toFloat(), 4.0f);
    
    // Peak hold line
    if (meterPeakDB > -60.0f && peakHoldCounter > 0)
    {
        float peakNormalized = juce::jmap(meterPeakDB, -60.0f, 6.0f, 0.0f, 1.0f);
        int peakY = meterArea.getY() + static_cast<int>(meterArea.getHeight() * (1.0f - peakNormalized));

        g.setColour(juce::Colours::white);
        g.drawHorizontalLine(static_cast<float>(peakY), 
            static_cast<float>(meterArea.getX()),
            static_cast<float>(meterArea.getRight()));
    }
    
    // dB labels on the left
    g.setColour(ModernLookAndFeel::ColorScheme::textDim);
    g.setFont(juce::FontOptions(9.0f));
    
    for (float db : {6.0f, 0.0f, -12.0f, -24.0f, -48.0f, -60.0f})
    {
        float y = juce::jmap(db, -60.0f, 6.0f, 
            static_cast<float>(meterArea.getBottom()),
            static_cast<float>(meterArea.getY()));

        juce::String dbText;
        if (db > 0.0f)
            dbText << "+";
        dbText << juce::String(static_cast<int>(db));

        g.drawText(dbText,
            labelArea.getX(), static_cast<int>(y) - 6,
            labelArea.getWidth() - 2, 12,
            juce::Justification::centredRight, false);

        g.setColour(ModernLookAndFeel::ColorScheme::gridLine.withAlpha(0.7f));
        g.drawHorizontalLine(static_cast<int>(y), static_cast<float>(meterArea.getX()), static_cast<float>(meterArea.getX() + 5));
        g.setColour(ModernLookAndFeel::ColorScheme::textDim);
    }
    
    // Border
    g.setColour(ModernLookAndFeel::ColorScheme::gridLine);
    g.drawRoundedRectangle(meterArea.toFloat(), 4.0f, 1.5f);
}

void DBMeter::resized()
{
    // Nothing to layout
}

void DBMeter::timerCallback()
{
    // Decay peak hold
    if (peakHoldCounter > 0)
    {
        peakHoldCounter--;
        if (peakHoldCounter == 0)
            meterPeakDB = currentMeterDB;
    }
    
    repaint();
}

void DBMeter::update(float dbValue)
{
    currentMeterDB = juce::jlimit(-60.0f, 6.0f, dbValue);
    
    // Update peak hold
    if (currentMeterDB > meterPeakDB)
    {
        meterPeakDB = currentMeterDB;
        peakHoldCounter = peakHoldFrames;
    }
}
