#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_basics/juce_gui_basics.h>
#include "PluginProcessor.h"
#include "UI/FrequencyResponseCurve.h"
#include "UI/FilterControlPanel.h"
#include <ai_ui/DBMeter.h>
#include <ai_ui/ModernLookAndFeel.h>

//==============================================================================
/**
 * Editor principale del plugin.
 */
class AudioPluginAudioProcessorEditor : public juce::AudioProcessorEditor,
                                         public juce::Timer
{
public:
    explicit AudioPluginAudioProcessorEditor(AudioPluginAudioProcessor&);
    ~AudioPluginAudioProcessorEditor() override;

    //==============================================================================
    void paint(juce::Graphics&) override;
    void resized() override;
    void timerCallback() override;

private:
    AudioPluginAudioProcessor& processorRef;
    
    // UI Components
    std::unique_ptr<FrequencyResponseCurve> frequencyResponseCurve;
    std::unique_ptr<FilterControlPanel> filterControlPanel;
    std::unique_ptr<DBMeter> dbMeter;
    juce::ToggleButton autoGainToggle;
    
    // Auto-gain parameter attachment
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> autoGainAttachment;
    
    ModernLookAndFeel modernLookAndFeel;
    
    // Header label
    juce::Label titleLabel;
    juce::Label versionLabel;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AudioPluginAudioProcessorEditor)
};
