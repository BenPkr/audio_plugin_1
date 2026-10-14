#include "PluginProcessor.h"
#include "PluginEditor.h"

AudioPluginAudioProcessorEditor::AudioPluginAudioProcessorEditor (AudioPluginAudioProcessor& p)
    : AudioProcessorEditor (&p), 
      audioProcessor (p),
      frequencyResponsePlot (p.apvts),
      poleZeroPlot (p.apvts),
      modulationSignalPlot (p.apvts)
{
    addAndMakeVisible (frequencyResponsePlot);
    addAndMakeVisible (poleZeroPlot);
    addAndMakeVisible (modulationSignalPlot);

    // Setup Labeled Mode Selector Dial
    modeDial.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    modeDial.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 80, 18);
    modeDial.setRange (0, 4, 1);
    
    // Map Mode Selector Dial numbers to explicit preset names
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

    modeDial.onValueChange = [this]() { updateActivePanel (static_cast<int>(modeDial.getValue())); };
    addAndMakeVisible (modeDial);

    modeDialLabel.setText ("Mode Select", juce::dontSendNotification);
    modeDialLabel.setJustificationType (juce::Justification::centred);
    modeDialLabel.attachToComponent (&modeDial, false);
    addAndMakeVisible (modeDialLabel);

    // Build Zölzer preset panel configurations (ModMode: 0 = Off, 1 = Sine, 2 = Noise)
    PresetPanel::Config customCfg  { "CUSTOM (FREE)", {0.1, 100.0, 10.0}, {0.0, 30.0, 1.0}, {0.05, 20.0, 1.0} };
    
    // Vibrato: BL=0 (locked), FF=1 (locked), FB=0 (locked), Sine LFO (ModMode=1)[cite: 1]
    PresetPanel::Config vibCfg     { "VIBRATO", {0.1, 5.0, 0.1}, {0.0, 3.0, 2.0}, {0.1, 5.0, 2.0}, 0.0f, true, 1.0f, true, 0.0f, true, 0.1f, false, 1, true };
    
    // Flanger: BL=0.7 (locked), FF=0.7 (locked), FB=0.7 (locked), Sine LFO (ModMode=1)[cite: 1]
    PresetPanel::Config flangCfg   { "FLANGER", {0.1, 5.0, 0.1}, {0.0, 2.0, 1.5}, {0.1, 1.0, 0.5}, 0.7f, true, 0.7f, true, 0.7f, true, 0.1f, false, 1, true };
    
    // Chorus: BL=0.7 (locked), FF=1.0 (locked), FB=-0.7 (locked), Noise LFO (ModMode=2)[cite: 1]
    PresetPanel::Config chorusCfg  { "CHORUS", {1.0, 30.0, 15.0}, {1.0, 30.0, 10.0}, {0.1, 5.0, 1.0}, 0.7f, true, 1.0f, true, -0.7f, true, 15.0f, false, 2, true };
    
    // Doubling: BL=0.7 (locked), FF=0.7 (locked), FB=0 (locked), Noise LFO (ModMode=2)[cite: 1]
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

    updateActivePanel (0);

    setResizable (true, true);
    setSize (900, 720);
}

AudioPluginAudioProcessorEditor::~AudioPluginAudioProcessorEditor() {}

void AudioPluginAudioProcessorEditor::updateActivePanel (int activeIndex)
{
    PresetPanel* panels[5] = { customPanel.get(), vibratoPanel.get(), flangerPanel.get(), chorusPanel.get(), doublingPanel.get() };

    for (int i = 0; i < 5; ++i)
    {
        if (i == activeIndex)
        {
            panels[i]->setEnabled (true);
            panels[i]->setAlpha (1.0f);  // Fully lit & active
            panels[i]->applyToAPVTS();   // Sync settings to DSP engine
        }
        else
        {
            panels[i]->setEnabled (false);
            panels[i]->setAlpha (0.25f); // Greyed out & locked out
        }
    }
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

    // --- Top Header: Mode Dial + Oscilloscope ---
    auto headerArea = bounds.removeFromTop (125);
    
    auto dialArea = headerArea.removeFromLeft (110);
    modeDialLabel.setBounds (dialArea.removeFromTop (16));
    modeDial.setBounds (dialArea);

    headerArea.removeFromLeft (10); // Spacing gap
    modulationSignalPlot.setBounds (headerArea);

    bounds.removeFromTop (12);

    // --- Middle Section: Frequency Response & Pole-Zero Plots ---
    auto plotArea = bounds.removeFromTop (170);
    const int plotW = (plotArea.getWidth() - 10) / 2;
    frequencyResponsePlot.setBounds (plotArea.removeFromLeft (plotW));
    plotArea.removeFromLeft (10);
    poleZeroPlot.setBounds (plotArea);

    bounds.removeFromTop (15);

    // --- Bottom Section: 5 Preset Channel Panels ---
    auto panelsArea = bounds;
    const int panelW = (panelsArea.getWidth() - 20) / 5;

    customPanel->setBounds   (panelsArea.removeFromLeft (panelW)); panelsArea.removeFromLeft (5);
    vibratoPanel->setBounds  (panelsArea.removeFromLeft (panelW)); panelsArea.removeFromLeft (5);
    flangerPanel->setBounds  (panelsArea.removeFromLeft (panelW)); panelsArea.removeFromLeft (5);
    chorusPanel->setBounds   (panelsArea.removeFromLeft (panelW)); panelsArea.removeFromLeft (5);
    doublingPanel->setBounds (panelsArea);
}