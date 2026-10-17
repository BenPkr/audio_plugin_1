#include "PluginProcessor.h"
#include "PluginEditor.h"

AudioPluginAudioProcessorEditor::AudioPluginAudioProcessorEditor (AudioPluginAudioProcessor& p)
    : AudioProcessorEditor (&p), 
      audioProcessor (p),
      frequencyResponsePlot (p, p.apvts),
      phaseResponsePlot (p.apvts),           // <--- INITIALIZE
      poleZeroPlot (p.apvts),
      modulationSignalPlot (p.apvts),
      audioScopePlot (p)
{
    addAndMakeVisible (frequencyResponsePlot);
    addAndMakeVisible (phaseResponsePlot);   // <--- ADD TO GUI
    addAndMakeVisible (poleZeroPlot);
    addAndMakeVisible (modulationSignalPlot);
    addAndMakeVisible (audioScopePlot);

    processorModeLabel.setText ("Processor Mode", juce::dontSendNotification);
    processorModeLabel.setJustificationType (juce::Justification::centred);
    addAndMakeVisible (processorModeLabel);

    processorModeBox.addItem ("Comb Filter", 1);
    processorModeBox.addItem ("Ring Mod", 2);
    processorModeBox.addItem ("Tremolo", 3);
    addAndMakeVisible (processorModeBox);
    processorModeAttachment = std::make_unique<ComboBoxAttachment> (
        p.apvts, ParameterIDs::mode, processorModeBox);

    // Setup Labeled Mode Selector Dial
    modeDial.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    modeDial.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 80, 18);
    modeDial.setRange (0, 4, 1);
    
    modeDial.textFromValueFunction = [](double val)
    {
        const int mode = static_cast<int>(val);
        switch (mode)
        {
            case 1:  return juce::String("Vibrato");
            case 2:  return juce::String("Flanger");
            case 3:  return juce::String("Chorus");
            case 4:  return juce::String("Doubling");
            default: return juce::String("Custom");
        }
    };
    modeDial.updateText();

    modeDial.onValueChange = [this]()
    {
        if (presetPanelsReady)
            updateActivePanel (static_cast<int> (modeDial.getValue()));
    };
    addAndMakeVisible (modeDial);
    presetAttachment = std::make_unique<SliderAttachment> (p.apvts, ParameterIDs::preset, modeDial);

    modeDialLabel.setText ("Comb Preset", juce::dontSendNotification);
    modeDialLabel.setJustificationType (juce::Justification::centred);
    addAndMakeVisible (modeDialLabel);

    modeControlsGroup.setText ("Mode Controls");
    addAndMakeVisible (modeControlsGroup);

    auto setupModeSlider = [this] (juce::Slider& slider, juce::Label& label, const juce::String& text)
    {
        slider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
        slider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 65, 18);
        label.setText (text, juce::dontSendNotification);
        label.setJustificationType (juce::Justification::centred);
        modeControlsGroup.addAndMakeVisible (slider);
        modeControlsGroup.addAndMakeVisible (label);
    };

    setupModeSlider (modeBlendSlider, modeBlendLabel, "Ring Mix");
    setupModeSlider (modeFrequencySlider, modeFrequencyLabel, "Carrier (Hz)");
    setupModeSlider (tremoloFrequencySlider, tremoloFrequencyLabel, "Rate (Hz)");
    setupModeSlider (modeDepthSlider, modeDepthLabel, "Depth (%)");

    modeBlendSlider.setRange (-1.0, 1.0, 0.01);
    modeBlendSlider.textFromValueFunction = [] (double value)
    {
        return juce::String ((value + 1.0) * 50.0, 1) + "%";
    };
    modeBlendSlider.valueFromTextFunction = [] (const juce::String& text)
    {
        return text.getDoubleValue() * 0.02 - 1.0;
    };
    modeFrequencySlider.setRange (0.05, 2000.0, 0.05);
    tremoloFrequencySlider.setRange (0.1, 20.0, 0.01);
    modeDepthSlider.setRange (0.0, 100.0, 0.01);
    modeBlendAttachment = std::make_unique<SliderAttachment> (p.apvts, ParameterIDs::blend, modeBlendSlider);
    modeFrequencyAttachment = std::make_unique<SliderAttachment> (
        p.apvts, ParameterIDs::modFrequency, modeFrequencySlider);
    tremoloFrequencyAttachment = std::make_unique<SliderAttachment> (
        p.apvts, ParameterIDs::tremFrequency, tremoloFrequencySlider);
    modeDepthAttachment = std::make_unique<SliderAttachment> (
        p.apvts, ParameterIDs::modDepth, modeDepthSlider);

    tremWaveformLabel.setText ("Waveform", juce::dontSendNotification);
    tremWaveformLabel.setJustificationType (juce::Justification::centred);
    tremWaveformBox.addItem ("Sine", 1);
    tremWaveformBox.addItem ("Triangle", 2);
    tremWaveformBox.addItem ("Square", 3);
    modeControlsGroup.addAndMakeVisible (tremWaveformLabel);
    modeControlsGroup.addAndMakeVisible (tremWaveformBox);
    tremWaveformAttachment = std::make_unique<ComboBoxAttachment> (
        p.apvts, ParameterIDs::tremWaveform, tremWaveformBox);

    // Setup Tame / Moorer Toggle Switches
    addAndMakeVisible (autoGainButton);
    addAndMakeVisible (dampingButton);

    autoGainAttachment = std::make_unique<ButtonAttachment> (p.apvts, ParameterIDs::autoGain, autoGainButton);
    dampingAttachment  = std::make_unique<ButtonAttachment> (p.apvts, ParameterIDs::damping,  dampingButton);

    // Build Preset Panels
    PresetPanel::Config customCfg  { "CUSTOM (FREE)", {0.0, 100.0, 10.0}, {0.0, 30.0, 1.0}, {0.05, 20.0, 1.0} };
    PresetPanel::Config vibCfg     { "VIBRATO", {0.1, 5.0, 0.1}, {0.0, 3.0, 2.0}, {0.1, 5.0, 2.0}, 0.0f, true, 1.0f, true, 0.0f, true, 0.1f, false, 1, true };
    PresetPanel::Config flangCfg   { "FLANGER", {0.1, 5.0, 0.1}, {0.0, 2.0, 1.5}, {0.1, 1.0, 0.5}, 0.7f, true, 0.7f, true, 0.7f, true, 0.1f, false, 1, true };
    PresetPanel::Config chorusCfg  { "CHORUS", {1.0, 30.0, 15.0}, {1.0, 30.0, 10.0}, {0.1, 5.0, 1.0}, 0.7f, true, 1.0f, true, -0.7f, true, 15.0f, false, 2, true };
    PresetPanel::Config doubleCfg  { "DOUBLING", {10.0, 100.0, 30.0}, {1.0, 100.0, 15.0}, {0.1, 5.0, 0.5}, 0.7f, true, 0.7f, true, 0.0f, true, 30.0f, false, 2, true };

    customPanel   = std::make_unique<PresetPanel> (p.apvts, customCfg);
    vibratoPanel  = std::make_unique<PresetPanel> (p.apvts, vibCfg);
    flangerPanel  = std::make_unique<PresetPanel> (p.apvts, flangCfg);
    chorusPanel   = std::make_unique<PresetPanel> (p.apvts, chorusCfg);
    doublingPanel = std::make_unique<PresetPanel> (p.apvts, doubleCfg);

    addAndMakeVisible (*customPanel);
    addAndMakeVisible (*vibratoPanel);
    addAndMakeVisible (*flangerPanel);
    addAndMakeVisible (*chorusPanel);
    addAndMakeVisible (*doublingPanel);

    presetPanelsReady = true;
    const auto* initialMode = p.apvts.getRawParameterValue (ParameterIDs::mode);
    lastGuiMode = initialMode != nullptr ? juce::jlimit (0, 2, static_cast<int> (initialMode->load())) : 0;
    updateGuiElementStates (lastGuiMode);
    updateActivePanel (static_cast<int> (modeDial.getValue()));

    setResizable (true, true);
    setSize (1050, 720);
    startTimerHz (30);
}

AudioPluginAudioProcessorEditor::~AudioPluginAudioProcessorEditor()
{
    stopTimer();
}

void AudioPluginAudioProcessorEditor::updateActivePanel (int activeIndex)
{
    PresetPanel* panels[5] = { customPanel.get(), vibratoPanel.get(), flangerPanel.get(), chorusPanel.get(), doublingPanel.get() };
    const bool isComb = lastGuiMode == 0;

    for (int i = 0; i < 5; ++i)
    {
        if (i == activeIndex && isComb)
        {
            panels[i]->setEnabled (true);
            panels[i]->setAlpha (1.0f);
            panels[i]->applyToAPVTS();
        }
        else
        {
            panels[i]->setEnabled (false);
            panels[i]->setAlpha (0.25f);
        }
    }
}

void AudioPluginAudioProcessorEditor::timerCallback()
{
    const auto* mode = audioProcessor.apvts.getRawParameterValue (ParameterIDs::mode);
    const int activeMode = mode != nullptr ? juce::jlimit (0, 2, static_cast<int> (mode->load())) : 0;
    if (activeMode != lastGuiMode)
    {
        lastGuiMode = activeMode;
        updateGuiElementStates (activeMode);
        updateActivePanel (static_cast<int> (modeDial.getValue()));
    }
}

void AudioPluginAudioProcessorEditor::updateGuiElementStates (int activeMode)
{
    const bool isComb = activeMode == 0;
    const bool isRing = activeMode == 1;
    const bool isTrem = activeMode == 2;

    modeDial.setVisible (isComb);
    modeDialLabel.setVisible (isComb);
    autoGainButton.setEnabled (isComb);
    dampingButton.setEnabled (isComb);

    customPanel->setVisible (isComb);
    vibratoPanel->setVisible (isComb);
    flangerPanel->setVisible (isComb);
    chorusPanel->setVisible (isComb);
    doublingPanel->setVisible (isComb);

    const bool hasModeControls = isRing || isTrem;
    modeControlsGroup.setVisible (hasModeControls);
    modeBlendSlider.setVisible (isRing);
    modeBlendLabel.setVisible (isRing);
    modeFrequencySlider.setVisible (isRing);
    modeFrequencyLabel.setVisible (isRing);
    tremoloFrequencySlider.setVisible (isTrem);
    tremoloFrequencyLabel.setVisible (isTrem);
    modeDepthSlider.setVisible (isTrem);
    modeDepthLabel.setVisible (isTrem);
    tremWaveformBox.setVisible (isTrem);
    tremWaveformLabel.setVisible (isTrem);

    modulationSignalPlot.setVisible (isComb);
    audioScopePlot.setVisible (true);
    frequencyResponsePlot.setProcessingMode (activeMode);
    phaseResponsePlot.setProcessingMode (activeMode);
    poleZeroPlot.setProcessingMode (activeMode);
    frequencyResponsePlot.setAlpha (isComb ? 1.0f : 0.35f);
    phaseResponsePlot.setAlpha (isComb ? 1.0f : 0.35f);
    poleZeroPlot.setAlpha (isComb ? 1.0f : 0.35f);
}

void AudioPluginAudioProcessorEditor::paint (juce::Graphics& g)
{
    g.fillAll (juce::Colour (0xff1e1e24));
    g.setColour (juce::Colours::white);
    g.setFont (16.0f);
    g.drawText ("Universal Comb Filter", getLocalBounds().removeFromTop (30), juce::Justification::centred, true);
}

void AudioPluginAudioProcessorEditor::resized()
{
    auto bounds = getLocalBounds().reduced (10);
    bounds.removeFromTop (25);

    // --- Top Header: Processor Mode, Comb Preset, Toggles, and Scopes ---
    auto headerArea = bounds.removeFromTop (165);
    
    auto leftHeader = headerArea.removeFromLeft (150);
    processorModeLabel.setBounds (leftHeader.removeFromTop (14));
    processorModeBox.setBounds (leftHeader.removeFromTop (22));
    modeDialLabel.setBounds (leftHeader.removeFromTop (14));
    modeDial.setBounds (leftHeader.removeFromTop (58));
    
    leftHeader.removeFromTop (2);
    autoGainButton.setBounds (leftHeader.removeFromTop (21));
    dampingButton.setBounds  (leftHeader.removeFromTop (21));

    headerArea.removeFromLeft (8);
    auto plotWidth = (headerArea.getWidth() - 8) / 2;
    modulationSignalPlot.setBounds (headerArea.removeFromLeft (plotWidth));
    headerArea.removeFromLeft (8);
    audioScopePlot.setBounds (headerArea);

    bounds.removeFromTop (12);

    // --- Middle Section: Frequency Response | Phase Response | Pole-Zero Plots ---
    auto plotArea = bounds.removeFromTop (170);
    const int plotW = (plotArea.getWidth() - 20) / 3;

    frequencyResponsePlot.setBounds (plotArea.removeFromLeft (plotW)); plotArea.removeFromLeft (10);
    phaseResponsePlot.setBounds     (plotArea.removeFromLeft (plotW)); plotArea.removeFromLeft (10);
    poleZeroPlot.setBounds          (plotArea);

    bounds.removeFromTop (15);

    // --- Bottom Section: Comb Presets or Mode-Specific Controls ---
    auto panelsArea = bounds;
    modeControlsGroup.setBounds (panelsArea);
    auto controls = modeControlsGroup.getLocalBounds().reduced (35, 25);
    const int controlWidth = controls.getWidth() / 4;
    auto addControl = [&controls, controlWidth] (juce::Slider& slider, juce::Label& label)
    {
        auto column = controls.removeFromLeft (controlWidth);
        label.setBounds (column.removeFromTop (20));
        slider.setBounds (column);
    };
    addControl (modeBlendSlider, modeBlendLabel);
    addControl (modeFrequencySlider, modeFrequencyLabel);
    tremoloFrequencySlider.setBounds (modeFrequencySlider.getBounds());
    tremoloFrequencyLabel.setBounds (modeFrequencyLabel.getBounds());
    addControl (modeDepthSlider, modeDepthLabel);
    auto waveformArea = controls.removeFromLeft (controlWidth);
    tremWaveformLabel.setBounds (waveformArea.removeFromTop (20));
    tremWaveformBox.setBounds (waveformArea.removeFromTop (26));

    const int panelW = (panelsArea.getWidth() - 20) / 5;

    customPanel->setBounds   (panelsArea.removeFromLeft (panelW)); panelsArea.removeFromLeft (5);
    vibratoPanel->setBounds  (panelsArea.removeFromLeft (panelW)); panelsArea.removeFromLeft (5);
    flangerPanel->setBounds  (panelsArea.removeFromLeft (panelW)); panelsArea.removeFromLeft (5);
    chorusPanel->setBounds   (panelsArea.removeFromLeft (panelW)); panelsArea.removeFromLeft (5);
    doublingPanel->setBounds (panelsArea);
}