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

    // Dynamic threshold
    dynThresholdKnob = std::make_unique<juce::Slider>();
    dynThresholdKnob->setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    dynThresholdKnob->setTextBoxStyle(juce::Slider::TextBoxBelow, false, 50, 20);
    addAndMakeVisible(dynThresholdKnob.get());

    dynThresholdLabel.setText("THR", juce::dontSendNotification);
    dynThresholdLabel.setJustificationType(juce::Justification::centred);
    dynThresholdLabel.setFont(juce::FontOptions(11.0f, juce::Font::bold));
    addAndMakeVisible(dynThresholdLabel);

    // Dynamic gain amount
    dynGainKnob = std::make_unique<juce::Slider>();
    dynGainKnob->setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    dynGainKnob->setTextBoxStyle(juce::Slider::TextBoxBelow, false, 50, 20);
    addAndMakeVisible(dynGainKnob.get());

    dynGainLabel.setText("DYN", juce::dontSendNotification);
    dynGainLabel.setJustificationType(juce::Justification::centred);
    dynGainLabel.setFont(juce::FontOptions(11.0f, juce::Font::bold));
    addAndMakeVisible(dynGainLabel);

    // Dynamic attack
    dynAttackKnob = std::make_unique<juce::Slider>();
    dynAttackKnob->setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    dynAttackKnob->setTextBoxStyle(juce::Slider::TextBoxBelow, false, 50, 20);
    addAndMakeVisible(dynAttackKnob.get());

    dynAttackLabel.setText("ATK", juce::dontSendNotification);
    dynAttackLabel.setJustificationType(juce::Justification::centred);
    dynAttackLabel.setFont(juce::FontOptions(11.0f, juce::Font::bold));
    addAndMakeVisible(dynAttackLabel);

    // Dynamic release
    dynReleaseKnob = std::make_unique<juce::Slider>();
    dynReleaseKnob->setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    dynReleaseKnob->setTextBoxStyle(juce::Slider::TextBoxBelow, false, 50, 20);
    addAndMakeVisible(dynReleaseKnob.get());

    dynReleaseLabel.setText("REL", juce::dontSendNotification);
    dynReleaseLabel.setJustificationType(juce::Justification::centred);
    dynReleaseLabel.setFont(juce::FontOptions(11.0f, juce::Font::bold));
    addAndMakeVisible(dynReleaseLabel);

    // Dynamic mode selector
    dynModeSelector = std::make_unique<juce::ComboBox>();
    dynModeSelector->addItem("Auto", 1);
    dynModeSelector->addItem("Cut", 2);
    dynModeSelector->addItem("Boost", 3);
    addAndMakeVisible(dynModeSelector.get());

    dynModeLabel.setText("MODE", juce::dontSendNotification);
    dynModeLabel.setJustificationType(juce::Justification::centred);
    dynModeLabel.setFont(juce::FontOptions(11.0f, juce::Font::bold));
    addAndMakeVisible(dynModeLabel);

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
    dynThresholdKnob->setVisible(false);
    dynGainKnob->setVisible(false);
    dynAttackKnob->setVisible(false);
    dynReleaseKnob->setVisible(false);
    dynModeSelector->setVisible(false);
    slopeSelector->setVisible(false);
    typeSelector->setVisible(false);
    removeButton->setVisible(false);
    resetButton->setVisible(false);
    freqLabel.setVisible(false);
    gainLabel.setVisible(false);
    qLabel.setVisible(false);
    panLabel.setVisible(false);
    dynThresholdLabel.setVisible(false);
    dynGainLabel.setVisible(false);
    dynAttackLabel.setVisible(false);
    dynReleaseLabel.setVisible(false);
    dynModeLabel.setVisible(false);
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

    // Layout: [TYPE] [SLOPE] [FREQ] [Q] [THR] [DYN] [ATK] [REL] [MODE] ... [GAIN] ... [PAN] [R] [X]

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

    // Dynamic threshold knob
    auto dynThrArea = bounds.removeFromLeft(70);
    dynThresholdLabel.setBounds(dynThrArea.removeFromTop(15));
    dynThresholdKnob->setBounds(dynThrArea.reduced(5));

    bounds.removeFromLeft(5);

    // Dynamic gain knob
    auto dynGainArea = bounds.removeFromLeft(70);
    dynGainLabel.setBounds(dynGainArea.removeFromTop(15));
    dynGainKnob->setBounds(dynGainArea.reduced(5));

    bounds.removeFromLeft(5);

    // Dynamic attack knob
    auto dynAttackArea = bounds.removeFromLeft(70);
    dynAttackLabel.setBounds(dynAttackArea.removeFromTop(15));
    dynAttackKnob->setBounds(dynAttackArea.reduced(5));

    bounds.removeFromLeft(5);

    // Dynamic release knob
    auto dynReleaseArea = bounds.removeFromLeft(70);
    dynReleaseLabel.setBounds(dynReleaseArea.removeFromTop(15));
    dynReleaseKnob->setBounds(dynReleaseArea.reduced(5));

    bounds.removeFromLeft(5);

    // Dynamic mode selector
    auto dynModeArea = bounds.removeFromLeft(78);
    dynModeLabel.setBounds(dynModeArea.removeFromTop(15));
    dynModeArea.removeFromTop(15);
    dynModeSelector->setBounds(dynModeArea.removeFromTop(28));

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
    dynThresholdKnob->setVisible(hasSelection);
    dynGainKnob->setVisible(hasSelection);
    dynAttackKnob->setVisible(hasSelection);
    dynReleaseKnob->setVisible(hasSelection);
    dynModeSelector->setVisible(hasSelection);
    slopeSelector->setVisible(hasSelection);
    typeSelector->setVisible(hasSelection);
    removeButton->setVisible(hasSelection);
    resetButton->setVisible(hasSelection);
    freqLabel.setVisible(hasSelection);
    gainLabel.setVisible(hasSelection);
    qLabel.setVisible(hasSelection);
    panLabel.setVisible(hasSelection);
    dynThresholdLabel.setVisible(hasSelection);
    dynGainLabel.setVisible(hasSelection);
    dynAttackLabel.setVisible(hasSelection);
    dynReleaseLabel.setVisible(hasSelection);
    dynModeLabel.setVisible(hasSelection);
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
    dynThresholdAttachment.reset();
    dynGainAttachment.reset();
    dynAttackAttachment.reset();
    dynReleaseAttachment.reset();
    dynModeAttachment.reset();

    juce::String prefix = "filter" + juce::String(selectedFilterIndex) + "_";

    freqAttachment = std::make_unique<SliderAttachment>(
        apvtsRef, prefix + "freq", *freqKnob);

    gainAttachment = std::make_unique<SliderAttachment>(
        apvtsRef, prefix + "gain", *gainKnob);

    qAttachment = std::make_unique<SliderAttachment>(
        apvtsRef, prefix + "q", *qKnob);

    panAttachment = std::make_unique<SliderAttachment>(
        apvtsRef, prefix + "pan", *panKnob);

    dynThresholdAttachment = std::make_unique<SliderAttachment>(
        apvtsRef, prefix + "dyn_threshold", *dynThresholdKnob);

    dynGainAttachment = std::make_unique<SliderAttachment>(
        apvtsRef, prefix + "dyn_gain", *dynGainKnob);

    dynAttackAttachment = std::make_unique<SliderAttachment>(
        apvtsRef, prefix + "dyn_attack_ms", *dynAttackKnob);

    dynReleaseAttachment = std::make_unique<SliderAttachment>(
        apvtsRef, prefix + "dyn_release_ms", *dynReleaseKnob);

    dynModeAttachment = std::make_unique<ComboAttachment>(
        apvtsRef, prefix + "dyn_mode", *dynModeSelector);

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
