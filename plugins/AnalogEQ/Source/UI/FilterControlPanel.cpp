#include "FilterControlPanel.h"
#include <ai_ui/ModernLookAndFeel.h>
#include "../Utils/ParameterHelper.h"

FilterControlPanel::FilterControlPanel(juce::AudioProcessorValueTreeState& apvts)
    : apvtsRef(apvts)
{
    // Freq knob
    freqKnob = std::make_unique<juce::Slider>();
    freqKnob->setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    freqKnob->setTextBoxStyle(juce::Slider::TextBoxBelow, false, 60, 20);
    addAndMakeVisible(freqKnob.get());

    freqLabel.setText("FREQ", juce::dontSendNotification);
    freqLabel.setJustificationType(juce::Justification::centred);
    freqLabel.setFont(juce::FontOptions(11.0f, juce::Font::bold));
    addAndMakeVisible(freqLabel);

    // Gain knob (large, rotary)
    gainKnob = std::make_unique<juce::Slider>();
    gainKnob->setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    gainKnob->setTextBoxStyle(juce::Slider::TextBoxBelow, false, 60, 20);
    addAndMakeVisible(gainKnob.get());

    gainLabel.setText("GAIN", juce::dontSendNotification);
    gainLabel.setJustificationType(juce::Justification::centred);
    gainLabel.setFont(juce::FontOptions(12.0f, juce::Font::bold));
    addAndMakeVisible(gainLabel);

    // Q knob (same size as pan)
    qKnob = std::make_unique<juce::Slider>();
    qKnob->setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    qKnob->setTextBoxStyle(juce::Slider::TextBoxBelow, false, 50, 20);
    addAndMakeVisible(qKnob.get());

    qLabel.setText("Q", juce::dontSendNotification);
    qLabel.setJustificationType(juce::Justification::centred);
    qLabel.setFont(juce::FontOptions(11.0f, juce::Font::bold));
    addAndMakeVisible(qLabel);

    // Pan knob (smaller)
    panKnob = std::make_unique<juce::Slider>();
    panKnob->setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    panKnob->setTextBoxStyle(juce::Slider::TextBoxBelow, false, 50, 20);
    addAndMakeVisible(panKnob.get());

    panLabel.setText("PAN", juce::dontSendNotification);
    panLabel.setJustificationType(juce::Justification::centred);
    panLabel.setFont(juce::FontOptions(11.0f, juce::Font::bold));
    addAndMakeVisible(panLabel);

    // Slope selector
    slopeSelector = std::make_unique<juce::ComboBox>();
    slopeSelector->addItem("6 dB", 1);
    slopeSelector->addItem("12 dB", 2);
    slopeSelector->addItem("24 dB", 3);
    slopeSelector->addItem("48 dB", 4);
    slopeSelector->addItem("96 dB", 5);
    addAndMakeVisible(slopeSelector.get());

    slopeLabel.setText("SLOPE", juce::dontSendNotification);
    slopeLabel.setJustificationType(juce::Justification::centred);
    slopeLabel.setFont(juce::FontOptions(11.0f, juce::Font::bold));
    addAndMakeVisible(slopeLabel);

    // Type selector
    typeSelector = std::make_unique<juce::ComboBox>();
    typeSelector->addItem("LP", 1);
    typeSelector->addItem("HP", 2);
    typeSelector->addItem("Bell", 3);
    typeSelector->addItem("L Sh", 4);
    typeSelector->addItem("H Sh", 5);
    typeSelector->addItem("Notch", 6);
    typeSelector->onChange = [this]() { updateSlopeAvailability(); };
    addAndMakeVisible(typeSelector.get());

    typeLabel.setText("TYPE", juce::dontSendNotification);
    typeLabel.setJustificationType(juce::Justification::centred);
    typeLabel.setFont(juce::FontOptions(11.0f, juce::Font::bold));
    addAndMakeVisible(typeLabel);

    // Remove button (X)
    removeButton = std::make_unique<juce::TextButton>("X");
    removeButton->onClick = [this]() {
        if (selectedFilterIndex >= 0)
        {
            juce::String prefix = "filter" + juce::String(selectedFilterIndex) + "_";
            if (auto* param = apvtsRef.getParameter(prefix + "enabled"))
                param->setValueNotifyingHost(0.0f);
            setSelectedFilter(-1);
            if (onFilterRemoved)
                onFilterRemoved();
        }
    };
    addAndMakeVisible(removeButton.get());

    // Reset button (R)
    resetButton = std::make_unique<juce::TextButton>("R");
    resetButton->onClick = [this]() {
        if (selectedFilterIndex >= 0)
        {
            juce::String prefix = "filter" + juce::String(selectedFilterIndex) + "_";
            if (auto* param = apvtsRef.getParameter(prefix + "freq"))
                param->setValueNotifyingHost(param->convertTo0to1(1000.0f));
            if (auto* param = apvtsRef.getParameter(prefix + "gain"))
                param->setValueNotifyingHost(param->convertTo0to1(0.0f));
            if (auto* param = apvtsRef.getParameter(prefix + "q"))
                param->setValueNotifyingHost(param->convertTo0to1(0.707f));
        }
    };
    addAndMakeVisible(resetButton.get());

    // No selection label
    noSelectionLabel.setText("No filter selected", juce::dontSendNotification);
    noSelectionLabel.setJustificationType(juce::Justification::centred);
    noSelectionLabel.setFont(juce::FontOptions(14.0f));
    noSelectionLabel.setColour(juce::Label::textColourId,
        ModernLookAndFeel::ColorScheme::textDim);
    addAndMakeVisible(noSelectionLabel);
    noSelectionLabel.setVisible(true);

    // Initially hide controls
    freqKnob->setVisible(false);
    gainKnob->setVisible(false);
    qKnob->setVisible(false);
    panKnob->setVisible(false);
    slopeSelector->setVisible(false);
    typeSelector->setVisible(false);
    removeButton->setVisible(false);
    resetButton->setVisible(false);
    freqLabel.setVisible(false);
    gainLabel.setVisible(false);
    qLabel.setVisible(false);
    panLabel.setVisible(false);
    slopeLabel.setVisible(false);
    typeLabel.setVisible(false);
}

FilterControlPanel::~FilterControlPanel()
{
}

void FilterControlPanel::paint(juce::Graphics& g)
{
    auto bounds = getLocalBounds();

    // Background
    g.setColour(ModernLookAndFeel::ColorScheme::background);
    g.fillRoundedRectangle(bounds.toFloat(), 8.0f);

    // Border
    g.setColour(ModernLookAndFeel::ColorScheme::gridLine);
    g.drawRoundedRectangle(bounds.toFloat().reduced(1), 8.0f, 1.5f);
}

void FilterControlPanel::resized()
{
    auto bounds = getLocalBounds().reduced(10, 5);

    if (selectedFilterIndex < 0)
    {
        noSelectionLabel.setBounds(bounds);
        return;
    }

    // Layout: [TYPE] [SLOPE] [FREQ] [Q] ... [GAIN centered] ... [PAN] [R] [X]

    // Far right: [R] [X] buttons
    auto rightBtns = bounds.removeFromRight(65);
    int btnH = 28;
    int btnY = rightBtns.getCentreY() - btnH / 2;
    removeButton->setBounds(rightBtns.getRight() - 28, btnY, 28, btnH);
    resetButton->setBounds(rightBtns.getRight() - 28 - 5 - 28, btnY, 28, btnH);

    bounds.removeFromRight(5);

    // PAN knob (right of gain)
    auto panArea = bounds.removeFromRight(70);
    panLabel.setBounds(panArea.removeFromTop(15));
    panKnob->setBounds(panArea.reduced(5));

    bounds.removeFromRight(5);

    // TYPE selector (far left)
    auto typeArea = bounds.removeFromLeft(70);
    typeLabel.setBounds(typeArea.removeFromTop(15));
    typeArea.removeFromTop(15);
    typeSelector->setBounds(typeArea.removeFromTop(28));

    bounds.removeFromLeft(5);

    // SLOPE selector
    auto slopeArea = bounds.removeFromLeft(70);
    slopeLabel.setBounds(slopeArea.removeFromTop(15));
    slopeArea.removeFromTop(15);
    slopeSelector->setBounds(slopeArea.removeFromTop(28));

    bounds.removeFromLeft(5);

    // FREQ knob
    auto freqArea = bounds.removeFromLeft(70);
    freqLabel.setBounds(freqArea.removeFromTop(15));
    freqKnob->setBounds(freqArea.reduced(5));

    bounds.removeFromLeft(5);

    // Q knob
    auto qArea = bounds.removeFromLeft(70);
    qLabel.setBounds(qArea.removeFromTop(15));
    qKnob->setBounds(qArea.reduced(5));

    bounds.removeFromLeft(5);

    // GAIN knob (centered in remaining space)
    auto gainArea = bounds;
    int maxGainW = 130;
    if (gainArea.getWidth() > maxGainW)
        gainArea = gainArea.withSizeKeepingCentre(maxGainW, gainArea.getHeight());
    gainLabel.setBounds(gainArea.removeFromTop(15));
    gainKnob->setBounds(gainArea.reduced(5));
}

void FilterControlPanel::setSelectedFilter(int filterIndex)
{
    if (filterIndex == selectedFilterIndex)
        return;

    selectedFilterIndex = filterIndex;
    bool hasSelection = (filterIndex >= 0);

    // Show/hide controls
    freqKnob->setVisible(hasSelection);
    gainKnob->setVisible(hasSelection);
    qKnob->setVisible(hasSelection);
    panKnob->setVisible(hasSelection);
    slopeSelector->setVisible(hasSelection);
    typeSelector->setVisible(hasSelection);
    removeButton->setVisible(hasSelection);
    resetButton->setVisible(hasSelection);
    freqLabel.setVisible(hasSelection);
    gainLabel.setVisible(hasSelection);
    qLabel.setVisible(hasSelection);
    panLabel.setVisible(hasSelection);
    slopeLabel.setVisible(hasSelection);
    typeLabel.setVisible(hasSelection);
    noSelectionLabel.setVisible(!hasSelection);

    if (hasSelection)
    {
        recreateAttachments();
        updateSlopeAvailability();
    }

    resized();
    repaint();
}

void FilterControlPanel::recreateAttachments()
{
    // Destroy old attachments first
    typeAttachment.reset();
    slopeAttachment.reset();
    freqAttachment.reset();
    gainAttachment.reset();
    qAttachment.reset();
    panAttachment.reset();

    juce::String prefix = "filter" + juce::String(selectedFilterIndex) + "_";

    freqAttachment = std::make_unique<SliderAttachment>(
        apvtsRef, prefix + "freq", *freqKnob);

    gainAttachment = std::make_unique<SliderAttachment>(
        apvtsRef, prefix + "gain", *gainKnob);

    qAttachment = std::make_unique<SliderAttachment>(
        apvtsRef, prefix + "q", *qKnob);

    panAttachment = std::make_unique<SliderAttachment>(
        apvtsRef, prefix + "pan", *panKnob);

    slopeAttachment = std::make_unique<ComboAttachment>(
        apvtsRef, prefix + "slope", *slopeSelector);

    typeAttachment = std::make_unique<ComboAttachment>(
        apvtsRef, prefix + "type", *typeSelector);
}

void FilterControlPanel::updateSlopeAvailability()
{
    slopeSelector->setEnabled(true);

    // Disable gain for LowPass (0) and HighPass (1)
    if (selectedFilterIndex >= 0)
    {
        auto typeParam = apvtsRef.getRawParameterValue(
            ParameterHelper::getParameterID(selectedFilterIndex, "type"));

        if (typeParam)
        {
            int filterType = static_cast<int>(typeParam->load());
            bool hasGain = (filterType != 0 && filterType != 1);
            gainKnob->setEnabled(hasGain);
            gainLabel.setAlpha(hasGain ? 1.0f : 0.4f);
        }
    }
}
