#include "PluginEditor.h"

//==============================================================================
AudioPluginAudioProcessorEditor::AudioPluginAudioProcessorEditor(AudioPluginAudioProcessor& p)
    : AudioProcessorEditor(&p), processorRef(p)
{
    // Setup look and feel
    setLookAndFeel(&modernLookAndFeel);
    
    // Crea il componente della curva di frequenza (con APVTS per interattività e processor per spectrum)
    frequencyResponseCurve = std::make_unique<FrequencyResponseCurve>(
        processorRef.getFilterChain(), 
        processorRef.getAPVTS(),
        processorRef);
    addAndMakeVisible(frequencyResponseCurve.get());
    
    // Create filter control panel
    filterControlPanel = std::make_unique<FilterControlPanel>(processorRef.getAPVTS());
    addAndMakeVisible(filterControlPanel.get());
    
    // Connect filter selection to control panel
    frequencyResponseCurve->onFilterSelected = [this](int filterIndex) {
        filterControlPanel->setSelectedFilter(filterIndex);
    };

    // Connect filter removal from control panel back to curve
    filterControlPanel->onFilterRemoved = [this]() {
        frequencyResponseCurve->deselectFilter();
    };
    
    // Create dB meter (right side, full height)
    dbMeter = std::make_unique<DBMeter>();
    addAndMakeVisible(dbMeter.get());
    
    // General section controls (bottom-right)
    autoGainButton.setButtonText("AUTO GAIN");
    autoGainButton.setClickingTogglesState(true);
    addAndMakeVisible(autoGainButton);

    phaseModeCombo.addItem("Minimum", 1);
    phaseModeCombo.addItem("Natural", 2);
    phaseModeCombo.addItem("Linear Phase", 3);
    addAndMakeVisible(phaseModeCombo);

    phaseQualityCombo.addItem("Low", 1);
    phaseQualityCombo.addItem("Mid", 2);
    phaseQualityCombo.addItem("High", 3);
    addAndMakeVisible(phaseQualityCombo);

    phaseModeLabel.setText("PHASE", juce::dontSendNotification);
    phaseModeLabel.setFont(juce::FontOptions(11.0f, juce::Font::bold));
    phaseModeLabel.setJustificationType(juce::Justification::centredLeft);
    phaseModeLabel.setColour(juce::Label::textColourId, ModernLookAndFeel::ColorScheme::textDim);
    addAndMakeVisible(phaseModeLabel);

    phaseQualityLabel.setText("QUALITY", juce::dontSendNotification);
    phaseQualityLabel.setFont(juce::FontOptions(11.0f, juce::Font::bold));
    phaseQualityLabel.setJustificationType(juce::Justification::centredLeft);
    phaseQualityLabel.setColour(juce::Label::textColourId, ModernLookAndFeel::ColorScheme::textDim);
    addAndMakeVisible(phaseQualityLabel);

    globalSectionLabel.setText("GENERAL", juce::dontSendNotification);
    globalSectionLabel.setFont(juce::FontOptions(11.0f, juce::Font::bold));
    globalSectionLabel.setJustificationType(juce::Justification::centredLeft);
    globalSectionLabel.setColour(juce::Label::textColourId, ModernLookAndFeel::ColorScheme::textDim);
    addAndMakeVisible(globalSectionLabel);

    latencyLabel.setFont(juce::FontOptions(10.0f));
    latencyLabel.setJustificationType(juce::Justification::centredLeft);
    latencyLabel.setColour(juce::Label::textColourId, ModernLookAndFeel::ColorScheme::textDim);
    addAndMakeVisible(latencyLabel);

    sidechainSectionButton.setButtonText(juce::String("SIDECHAIN [+]"));
    sidechainSectionButton.onClick = [this]()
    {
        isSidechainSectionExpanded = !isSidechainSectionExpanded;
        sidechainEnableButton.setVisible(isSidechainSectionExpanded);
        sidechainSectionLabel.setVisible(isSidechainSectionExpanded);
        sidechainSectionButton.setButtonText(isSidechainSectionExpanded ? "SIDECHAIN [-]" : "SIDECHAIN [+]");
        resized();
    };
    addAndMakeVisible(sidechainSectionButton);

    sidechainEnableButton.setButtonText("SIDECHAIN OFF");
    sidechainEnableButton.setClickingTogglesState(true);
    sidechainEnableButton.setVisible(false);
    addAndMakeVisible(sidechainEnableButton);

    sidechainSectionLabel.setText("External key input for dynamic detector", juce::dontSendNotification);
    sidechainSectionLabel.setFont(juce::FontOptions(10.0f));
    sidechainSectionLabel.setJustificationType(juce::Justification::centred);
    sidechainSectionLabel.setColour(juce::Label::textColourId, ModernLookAndFeel::ColorScheme::textDim);
    sidechainSectionLabel.setVisible(false);
    addAndMakeVisible(sidechainSectionLabel);

    // Attach global parameters
    autoGainAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(
        processorRef.getAPVTS(), "auto_gain", autoGainButton);
    phaseModeAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(
        processorRef.getAPVTS(), "phase_mode", phaseModeCombo);
    phaseQualityAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(
        processorRef.getAPVTS(), "linear_phase_quality", phaseQualityCombo);
    sidechainEnabledAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(
        processorRef.getAPVTS(), "sidechain_enabled", sidechainEnableButton);

    // Setup title label
    titleLabel.setText("ANALOG EQ", juce::dontSendNotification);
    titleLabel.setFont(juce::FontOptions(24.0f, juce::Font::bold));
    titleLabel.setColour(juce::Label::textColourId, ModernLookAndFeel::ColorScheme::text);
    titleLabel.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(titleLabel);
    
    // Setup version info label (smaller, at bottom)
    versionLabel.setText("Version " + juce::String(JucePlugin_VersionString) + " | " + 
                        juce::String(JucePlugin_Manufacturer), juce::dontSendNotification);
    versionLabel.setFont(juce::FontOptions(10.0f));
    versionLabel.setColour(juce::Label::textColourId, ModernLookAndFeel::ColorScheme::textDim);
    versionLabel.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(versionLabel);
    
    // Start timer for meter updates (30 FPS)
    startTimerHz(30);
    
    // Dimensioni della finestra
    setSize(850, 720);  // Increased width for meter
    setResizable(true, true);
    setResizeLimits(600, 500, 1600, 1200);
}

AudioPluginAudioProcessorEditor::~AudioPluginAudioProcessorEditor()
{
    setLookAndFeel(nullptr);
    stopTimer();
}

//==============================================================================
void AudioPluginAudioProcessorEditor::paint(juce::Graphics& g)
{
    // Background con gradient
    auto bounds = getLocalBounds().toFloat();
    
    juce::ColourGradient gradient(
        ModernLookAndFeel::ColorScheme::backgroundDark, 
        bounds.getCentreX(), bounds.getY(),
        ModernLookAndFeel::ColorScheme::background, 
        bounds.getCentreX(), bounds.getBottom(),
        false);
    
    g.setGradientFill(gradient);
    g.fillAll();
    
    // Border sottile
    g.setColour(ModernLookAndFeel::ColorScheme::gridLine);
    g.drawRect(bounds, 1.0f);
}

void AudioPluginAudioProcessorEditor::timerCallback()
{
    // Update meter with current audio level from processor
    float level = processorRef.getMeterLevel();
    dbMeter->update(level);

    autoGainButton.setButtonText(autoGainButton.getToggleState() ? "AUTO GAIN ON" : "AUTO GAIN OFF");
    sidechainEnableButton.setButtonText(sidechainEnableButton.getToggleState() ? "SIDECHAIN ON" : "SIDECHAIN OFF");

    const bool linearPhaseSelected = (phaseModeCombo.getSelectedItemIndex() == 2);
    phaseQualityCombo.setEnabled(linearPhaseSelected);
    phaseQualityLabel.setAlpha(linearPhaseSelected ? 1.0f : 0.45f);

    const auto latencySamples = processorRef.getCurrentLatencySamplesForUI();
    const auto sr = processorRef.getSampleRate();
    const auto latencyMs = (sr > 0.0) ? (1000.0 * static_cast<double>(latencySamples) / sr) : 0.0;
    const auto qualitySuffix = processorRef.getCurrentPhaseModeName() == "Linear Phase"
        ? " (" + processorRef.getCurrentLinearPhaseQualityName() + ")"
        : juce::String();
    latencyLabel.setText(
        processorRef.getCurrentPhaseModeName() + qualitySuffix + " | "
            + juce::String(latencySamples) + " smp / " + juce::String(latencyMs, 2) + " ms",
        juce::dontSendNotification);
}

void AudioPluginAudioProcessorEditor::resized()
{
    auto bounds = getLocalBounds();

    sidechainEnableButton.setVisible(isSidechainSectionExpanded);
    sidechainSectionLabel.setVisible(isSidechainSectionExpanded);

    // Title at top
    titleLabel.setBounds(bounds.removeFromTop(50));
    
    // Version info at bottom
    versionLabel.setBounds(bounds.removeFromBottom(20));
    
    // Meter on right side with enough space for dB labels
    auto meterArea = bounds.removeFromRight(95);
    dbMeter->setBounds(meterArea.reduced(8, 10));

    // Control panel at bottom (140px height)
    auto controlPanelRow = bounds.removeFromBottom(140).reduced(10);

    // Sidechain collapsible section (bottom-center)
    auto sidechainStrip = controlPanelRow.removeFromBottom(36);
    auto sidechainHeader = sidechainStrip.withSizeKeepingCentre(180, 28);
    sidechainSectionButton.setBounds(sidechainHeader);

    if (isSidechainSectionExpanded)
    {
        auto sidechainPanel = sidechainStrip.withSizeKeepingCentre(340, 34);
        sidechainSectionLabel.setBounds(sidechainPanel.removeFromLeft(200));
        sidechainEnableButton.setBounds(sidechainPanel.reduced(4));
    }

    // Global section on right of control panel
    auto generalArea = controlPanelRow.removeFromRight(235).reduced(5);
    globalSectionLabel.setBounds(generalArea.removeFromTop(16));
    generalArea.removeFromTop(4);

    auto topRow = generalArea.removeFromTop(30);
    phaseModeLabel.setBounds(topRow.removeFromLeft(50));
    phaseModeCombo.setBounds(topRow.reduced(2));

    generalArea.removeFromTop(4);
    auto qualityRow = generalArea.removeFromTop(30);
    phaseQualityLabel.setBounds(qualityRow.removeFromLeft(50));
    phaseQualityCombo.setBounds(qualityRow.reduced(2));

    generalArea.removeFromTop(4);
    autoGainButton.setBounds(generalArea.removeFromTop(30));

    generalArea.removeFromTop(4);
    latencyLabel.setBounds(generalArea.removeFromTop(20));

    // Filter control panel takes remaining space on left
    filterControlPanel->setBounds(controlPanelRow);
    
    // Main area per la curva di frequenza (remaining space)
    auto mainArea = bounds.reduced(20);
    frequencyResponseCurve->setBounds(mainArea);
}
