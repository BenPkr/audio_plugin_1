#include "ExpanderEditor.h"

ExpanderAudioProcessorEditor::ExpanderAudioProcessorEditor (ExpanderAudioProcessor& processorToEdit)
    : AudioProcessorEditor (&processorToEdit),
      audioProcessor (processorToEdit),
      transferCurve (processorToEdit)
{
    addAndMakeVisible (transferCurve);

    for (size_t index = 0; index < sliders.size(); ++index)
    {
        auto& slider = sliders[index];
        auto& label = labels[index];
        slider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
        slider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 78, 20);
        slider.setNumDecimalPlacesToDisplay (1);
        label.setText (parameterNames[index], juce::dontSendNotification);
        label.setJustificationType (juce::Justification::centred);
        addAndMakeVisible (slider);
        addAndMakeVisible (label);
        attachments[index] = std::make_unique<SliderAttachment> (
            audioProcessor.apvts, parameterIDs[index], slider);
    }

    sliders[0].setRange (-80.0, 0.0, 0.1);
    sliders[0].setTextValueSuffix (" dB");
    sliders[1].setRange (1.0, 10.0, 0.1);
    sliders[1].textFromValueFunction = [] (double value)
    {
        return juce::String (value, 1) + ":1";
    };
    sliders[2].setRange (0.1, 100.0, 0.1);
    sliders[2].setTextValueSuffix (" ms");
    sliders[3].setRange (10.0, 1000.0, 1.0);
    sliders[3].setTextValueSuffix (" ms");

    setSize (760, 500);
    startTimerHz (30);
}

void ExpanderAudioProcessorEditor::paint (juce::Graphics& graphics)
{
    graphics.fillAll (juce::Colour (0xff1e1e24));
    graphics.setColour (juce::Colours::white);
    graphics.setFont (18.0f);
    graphics.drawText ("Downward Expander", getLocalBounds().removeFromTop (32),
                       juce::Justification::centred, true);
}

void ExpanderAudioProcessorEditor::resized()
{
    auto bounds = getLocalBounds().reduced (12);
    bounds.removeFromTop (32);
    transferCurve.setBounds (bounds.removeFromTop (280));
    bounds.removeFromTop (8);
    const int columnWidth = bounds.getWidth() / static_cast<int> (sliders.size());

    for (size_t index = 0; index < sliders.size(); ++index)
    {
        auto column = bounds.removeFromLeft (columnWidth);
        labels[index].setBounds (column.removeFromTop (24));
        sliders[index].setBounds (column);
    }
}

void ExpanderAudioProcessorEditor::timerCallback()
{
    transferCurve.repaint();
}
