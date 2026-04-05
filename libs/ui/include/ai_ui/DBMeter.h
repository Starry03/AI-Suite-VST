#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

/**
 * Global dB meter displayed on the right side of the window.
 * Shows output level with gradient and peak hold.
 */
class DBMeter : public juce::Component,
                public juce::Timer
{
public:
    DBMeter();
    ~DBMeter() override;
    
    void paint(juce::Graphics& g) override;
    void resized() override;
    void timerCallback() override;
    
    /**
     * Update meter with current dB level.
     * @param dbValue The dB value to display (-60 to +6)
     */
    void update(float dbValue);
    
private:
    float currentMeterDB = -60.0f;
    float meterPeakDB = -60.0f;
    int peakHoldCounter = 0;
    static constexpr int peakHoldFrames = 30;  // ~1 second at 30 FPS
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(DBMeter)
};
