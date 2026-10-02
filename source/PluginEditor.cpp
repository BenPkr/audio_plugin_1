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

    // 2. Setup Sliders and Labels
    setupSlider (delaySlider,       delayLabel,       "Delay (ms)");
    setupSlider (blendSlider,       blendLabel,       "Blend (BL)");
    setupSlider (feedforwardSlider, feedforwardLabel, "Feedforward");
    setupSlider (feedbackSlider,    feedbackLabel,    "Feedback");

    // 3. Attach Sliders to APVTS Parameters
    delayAttachment       = std::make_unique<SliderAttachment> (audioProcessor.apvts, ParameterIDs::delay,       delaySlider);
    blendAttachment       = std::make_unique<SliderAttachment> (audioProcessor.apvts, ParameterIDs::blend,       blendSlider);
    feedforwardAttachment = std::make_unique<SliderAttachment> (audioProcessor.apvts, ParameterIDs::feedforward, feedforwardSlider);
    feedbackAttachment    = std::make_unique<SliderAttachment> (audioProcessor.apvts, ParameterIDs::feedback,    feedbackSlider);

    // Make window resizable with default size
    setResizable (true, true);
    setSize (700, 450);
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
}

void AudioPluginAudioProcessorEditor::resized()
{
    auto bounds = getLocalBounds().reduced (10);
    bounds.removeFromTop (25); // Reserve title area

    // --- Top Section: Plots ---
    auto plotArea = bounds.removeFromTop (220);
    const int plotWidth = (plotArea.getWidth() - 10) / 2;

    frequencyResponsePlot.setBounds (plotArea.removeFromLeft (plotWidth));
    plotArea.removeFromLeft (10); // Gap between plots
    poleZeroPlot.setBounds (plotArea);

    bounds.removeFromTop (15); // Gap between plots and controls

    // --- Bottom Section: Rotary Knobs ---
    auto knobArea = bounds;
    const int numKnobs = 4;
    const int knobWidth = knobArea.getWidth() / numKnobs;

    delaySlider.setBounds       (knobArea.removeFromLeft (knobWidth).reduced (10, 0));
    blendSlider.setBounds       (knobArea.removeFromLeft (knobWidth).reduced (10, 0));
    feedforwardSlider.setBounds (knobArea.removeFromLeft (knobWidth).reduced (10, 0));
    feedbackSlider.setBounds    (knobArea.removeFromLeft (knobWidth).reduced (10, 0));
}