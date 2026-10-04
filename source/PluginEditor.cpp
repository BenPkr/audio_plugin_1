#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
AudioPluginAudioProcessorEditor::AudioPluginAudioProcessorEditor (AudioPluginAudioProcessor& p)
    : AudioProcessorEditor (&p), 
      audioProcessor (p),
      frequencyResponsePlot (p.apvts),
      poleZeroPlot (p.apvts)
{
    // 1. Add Visual Plot Components
    addAndMakeVisible (frequencyResponsePlot);
    addAndMakeVisible (poleZeroPlot);

    // Helper lambda to configure sliders cleanly
    auto setupSlider = [this](juce::Slider& slider, juce::Label& label, const juce::String& text)
    {
        slider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
        slider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 60, 20);
        addAndMakeVisible (slider);

        label.setText (text, juce::dontSendNotification);
        label.setJustificationType (juce::Justification::centred);
        label.attachToComponent (&slider, false);
        addAndMakeVisible (label);
    };

    // 2. Setup Base Sliders & Labels
    setupSlider (delaySlider,       delayLabel,       "Delay (ms)");
    setupSlider (blendSlider,       blendLabel,       "Blend (BL)");
    setupSlider (feedforwardSlider, feedforwardLabel, "Feedforward");
    setupSlider (feedbackSlider,    feedbackLabel,    "Feedback");

    // 3. Setup Modulation Controls
    addAndMakeVisible (modEnableButton);

    modTypeComboBox.addItemList ({ "Sine", "Lowpass Noise" }, 1);
    addAndMakeVisible (modTypeComboBox);

    modTypeLabel.setText ("Type", juce::dontSendNotification);
    modTypeLabel.setJustificationType (juce::Justification::centred);
    modTypeLabel.attachToComponent (&modTypeComboBox, false);
    addAndMakeVisible (modTypeLabel);

    setupSlider (modDepthSlider,     modDepthLabel,     "Mod Depth (ms)");
    setupSlider (modFrequencySlider, modFrequencyLabel, "Mod Rate (Hz)");

    // 4. Attach Controls to APVTS Parameters
    delayAttachment       = std::make_unique<SliderAttachment> (audioProcessor.apvts, ParameterIDs::delay,       delaySlider);
    blendAttachment       = std::make_unique<SliderAttachment> (audioProcessor.apvts, ParameterIDs::blend,       blendSlider);
    feedforwardAttachment = std::make_unique<SliderAttachment> (audioProcessor.apvts, ParameterIDs::feedforward, feedforwardSlider);
    feedbackAttachment    = std::make_unique<SliderAttachment> (audioProcessor.apvts, ParameterIDs::feedback,    feedbackSlider);

    modEnableAttachment    = std::make_unique<ButtonAttachment>   (audioProcessor.apvts, ParameterIDs::modEnable,    modEnableButton);
    modTypeAttachment      = std::make_unique<ComboBoxAttachment> (audioProcessor.apvts, ParameterIDs::modType,      modTypeComboBox);
    modDepthAttachment     = std::make_unique<SliderAttachment>   (audioProcessor.apvts, ParameterIDs::modDepth,     modDepthSlider);
    modFrequencyAttachment = std::make_unique<SliderAttachment>   (audioProcessor.apvts, ParameterIDs::modFrequency, modFrequencySlider);

    // Make window resizable with expanded height for modulation controls
    setResizable (true, true);
    setSize (700, 560);
}

AudioPluginAudioProcessorEditor::~AudioPluginAudioProcessorEditor()
{
}

//==============================================================================
void AudioPluginAudioProcessorEditor::paint (juce::Graphics& g)
{
    g.fillAll (juce::Colour (0xff1e1e24)); // Dark background

    g.setColour (juce::Colours::white);
    g.setFont (16.0f);
    g.drawText ("Universal Comb Filter", getLocalBounds().removeFromTop (30), juce::Justification::centred, true);

    // Draw thin section separator between base controls and modulation controls
    g.setColour (juce::Colours::darkgrey.withAlpha (0.5f));
    g.drawHorizontalLine (415, 15.0f, static_cast<float> (getWidth() - 15));
}

void AudioPluginAudioProcessorEditor::resized()
{
    auto bounds = getLocalBounds().reduced (10);
    bounds.removeFromTop (25); // Reserve title area

    // --- Top Section: Plots ---
    auto plotArea = bounds.removeFromTop (200);
    const int plotWidth = (plotArea.getWidth() - 10) / 2;

    frequencyResponsePlot.setBounds (plotArea.removeFromLeft (plotWidth));
    plotArea.removeFromLeft (10); // Gap between plots
    poleZeroPlot.setBounds (plotArea);

    bounds.removeFromTop (15); // Gap between plots and controls

    // --- Middle Section: Base Filter Rotary Knobs ---
    auto knobArea = bounds.removeFromTop (130);
    const int numKnobs = 4;
    const int knobWidth = knobArea.getWidth() / numKnobs;

    delaySlider.setBounds       (knobArea.removeFromLeft (knobWidth).reduced (10, 0));
    blendSlider.setBounds       (knobArea.removeFromLeft (knobWidth).reduced (10, 0));
    feedforwardSlider.setBounds (knobArea.removeFromLeft (knobWidth).reduced (10, 0));
    feedbackSlider.setBounds    (knobArea.removeFromLeft (knobWidth).reduced (10, 0));

    bounds.removeFromTop (20); // Gap for section separator

    // --- Bottom Section: Modulation Controls ---
    auto modArea = bounds;
    const int modSectionWidth = modArea.getWidth() / 4;

    // Col 1: Toggle Button
    auto toggleArea = modArea.removeFromLeft (modSectionWidth);
    modEnableButton.setBounds (toggleArea.withSizeKeepingCentre (120, 30));

    // Col 2: Waveform Choice Dropdown
    auto comboArea = modArea.removeFromLeft (modSectionWidth);
    modTypeComboBox.setBounds (comboArea.withSizeKeepingCentre (120, 24));

    // Col 3 & 4: Mod Depth and Rate Rotary Knobs
    modDepthSlider.setBounds     (modArea.removeFromLeft (modSectionWidth).reduced (10, 0));
    modFrequencySlider.setBounds (modArea.removeFromLeft (modSectionWidth).reduced (10, 0));
}