#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
AudioPluginAudioProcessorEditor::AudioPluginAudioProcessorEditor (AudioPluginAudioProcessor& p)
    : AudioProcessorEditor (&p), 
      audioProcessor (p),
      frequencyResponsePlot (p.apvts),
      poleZeroPlot (p.apvts),
      modulationSignalPlot (p.apvts)
{
    // 1. Add Visual Plot Components
    addAndMakeVisible (frequencyResponsePlot);
    addAndMakeVisible (poleZeroPlot);
    addAndMakeVisible (modulationSignalPlot);

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

    // Resizable window sized to fit all 3 plots and controls nicely
    setResizable (true, true);
    setSize (720, 620);
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

    // Section separator line above modulation controls
    g.setColour (juce::Colours::darkgrey.withAlpha (0.5f));
    g.drawHorizontalLine (385, 15.0f, static_cast<float> (getWidth() - 15));
}

void AudioPluginAudioProcessorEditor::resized()
{
    auto bounds = getLocalBounds().reduced (10);
    bounds.removeFromTop (25); // Title space

    // --- Top Section: Frequency & Pole-Zero Plots ---
    auto topPlotArea = bounds.removeFromTop (190);
    const int plotWidth = (topPlotArea.getWidth() - 10) / 2;

    frequencyResponsePlot.setBounds (topPlotArea.removeFromLeft (plotWidth));
    topPlotArea.removeFromLeft (10); // Gap
    poleZeroPlot.setBounds (topPlotArea);

    bounds.removeFromTop (12);

    // --- Middle Section: Base Filter Rotary Knobs ---
    auto knobArea = bounds.removeFromTop (125);
    const int numKnobs = 4;
    const int knobWidth = knobArea.getWidth() / numKnobs;

    delaySlider.setBounds       (knobArea.removeFromLeft (knobWidth).reduced (10, 0));
    blendSlider.setBounds       (knobArea.removeFromLeft (knobWidth).reduced (10, 0));
    feedforwardSlider.setBounds (knobArea.removeFromLeft (knobWidth).reduced (10, 0));
    feedbackSlider.setBounds    (knobArea.removeFromLeft (knobWidth).reduced (10, 0));

    bounds.removeFromTop (15);

    // --- Bottom Section Left: Modulation Controls ---
    auto bottomArea = bounds;
    auto modControlsArea = bottomArea.removeFromTop (115);

    const int modColWidth = modControlsArea.getWidth() / 4;

    // Col 1: Toggle Button
    auto toggleArea = modControlsArea.removeFromLeft (modColWidth);
    modEnableButton.setBounds (toggleArea.withSizeKeepingCentre (110, 30));

    // Col 2: Type Dropdown
    auto comboArea = modControlsArea.removeFromLeft (modColWidth);
    modTypeComboBox.setBounds (comboArea.withSizeKeepingCentre (110, 24));

    // Col 3 & 4: Mod Depth and Rate Knobs
    modDepthSlider.setBounds     (modControlsArea.removeFromLeft (modColWidth).reduced (10, 0));
    modFrequencySlider.setBounds (modControlsArea.removeFromLeft (modColWidth).reduced (10, 0));

    // --- Bottom Section Right/Full Width: Time-Domain Oscilloscope Plot ---
    modulationSignalPlot.setBounds (bottomArea.reduced (5, 0));
}