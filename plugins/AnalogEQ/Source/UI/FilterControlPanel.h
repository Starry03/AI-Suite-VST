#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_audio_processors/juce_audio_processors.h>

/**
 * FabFilter-style control panel for selected filter.
 * Shows gain knob (center), slope (left), pan (right),
 * filter type selector, remove and reset buttons.
 */
class FilterControlPanel : public juce::Component
{
public:
    FilterControlPanel(juce::AudioProcessorValueTreeState& apvts);
    ~FilterControlPanel() override;

    void paint(juce::Graphics& g) override;
    void resized() override;

    /**
     * Set which filter is being controlled.
     * @param filterIndex Index of filter (0-7), or -1 for none
     */
    void setSelectedFilter(int filterIndex);

    // Callback when filter is removed via X button
    std::function<void()> onFilterRemoved;

private:
    juce::AudioProcessorValueTreeState& apvtsRef;
    int selectedFilterIndex = -1;

    // Knobs
    std::unique_ptr<juce::Slider> freqKnob;
    std::unique_ptr<juce::Slider> gainKnob;
    std::unique_ptr<juce::Slider> qKnob;
    std::unique_ptr<juce::Slider> panKnob;

    // Selectors
    std::unique_ptr<juce::ComboBox> slopeSelector;
    std::unique_ptr<juce::ComboBox> typeSelector;

    // Action buttons
    std::unique_ptr<juce::TextButton> removeButton;
    std::unique_ptr<juce::TextButton> resetButton;

    // Labels
    juce::Label freqLabel, gainLabel, qLabel, panLabel, slopeLabel, typeLabel, noSelectionLabel;

    // Attachments
    using SliderAttachment = juce::AudioProcessorValueTreeState::SliderAttachment;
    using ComboAttachment = juce::AudioProcessorValueTreeState::ComboBoxAttachment;

    std::unique_ptr<SliderAttachment> freqAttachment;
    std::unique_ptr<SliderAttachment> gainAttachment;
    std::unique_ptr<SliderAttachment> qAttachment;
    std::unique_ptr<SliderAttachment> panAttachment;
    std::unique_ptr<ComboAttachment> slopeAttachment;
    std::unique_ptr<ComboAttachment> typeAttachment;

    void recreateAttachments();
    void updateSlopeAvailability();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(FilterControlPanel)
};
