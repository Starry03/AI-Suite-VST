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
    
    // Create auto-gain toggle (global, in bottom bar)
    autoGainToggle.setButtonText("AUTO GAIN");
    addAndMakeVisible(autoGainToggle);
    
    // Attach auto-gain to parameter
    autoGainAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(
        processorRef.getAPVTS(), "auto_gain", autoGainToggle);
    
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
}

void AudioPluginAudioProcessorEditor::resized()
{
    auto bounds = getLocalBounds();
    
    // Title at top
    titleLabel.setBounds(bounds.removeFromTop(50));
    
    // Version info at bottom
    versionLabel.setBounds(bounds.removeFromBottom(20));
    
    // Meter on right side (full height) - 40px wide with 30px space for labels
    auto meterArea = bounds.removeFromRight(70);
    dbMeter->setBounds(meterArea.reduced(30, 10).withTrimmedLeft(5));
    
    // Control panel at bottom (140px height)
    auto controlPanelRow = bounds.removeFromBottom(140).reduced(10);
    
    // Auto-gain section on right of control panel (100px wide)
    auto autoGainArea = controlPanelRow.removeFromRight(100).reduced(5);
    autoGainArea.removeFromTop(40);  // Vertical centering
    autoGainToggle.setBounds(autoGainArea.removeFromTop(30));
    
    // Filter control panel takes remaining space on left
    filterControlPanel->setBounds(controlPanelRow);
    
    // Main area per la curva di frequenza (remaining space)
    auto mainArea = bounds.reduced(20);
    frequencyResponseCurve->setBounds(mainArea);
}
