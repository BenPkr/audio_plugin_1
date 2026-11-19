#include "CompressorEditor.h"

CompressorAudioProcessorEditor::CompressorAudioProcessorEditor (CompressorAudioProcessor& processorToEdit)
    : AudioProcessorEditor (&processorToEdit),
      audioProcessor (processorToEdit),
      transferCurve (processorToEdit)
{
    addAndMakeVisible (transferCurve);

    for (size_t index = 0; index < sliders.size(); ++index)
    {
        setupSlider (sliders[index], labels[index], parameterNames[index]);
        attachments[index] = std::make_unique<SliderAttachment> (
            audioProcessor.apvts, parameterIDs[index], sliders[index]);
    }

    sliders[0].setRange (-60.0, 0.0, 0.1);
    sliders[0].setTextValueSuffix (" dB");
    sliders[1].textFromValueFunction = [] (double value)
    {
        return value >= compressorLimiterRatio
             ? juce::String ("Inf:1")
             : juce::String (value, 1) + ":1";
    };
    sliders[1].valueFromTextFunction = [] (const juce::String& text)
    {
        const auto normalized = text.trim().toLowerCase();
        if (normalized == "inf" || normalized == "infinity"
            || normalized == "inf:1" || normalized == "infinity:1")
            return static_cast<double> (compressorLimiterRatio);
        return normalized.upToFirstOccurrenceOf (":", false, false).getDoubleValue();
    };
    sliders[2].setRange (0.0, 24.0, 0.1);
    sliders[2].setTextValueSuffix (" dB");
    sliders[3].setRange (0.1, 100.0, 0.1);
    sliders[3].setTextValueSuffix (" ms");
    sliders[4].setRange (10.0, 1000.0, 1.0);
    sliders[4].setTextValueSuffix (" ms");
    sliders[5].setRange (-12.0, 24.0, 0.1);
    sliders[5].setTextValueSuffix (" dB");

    setSize (760, 500);
    startTimerHz (30);
}

void CompressorAudioProcessorEditor::setupSlider (juce::Slider& slider,
                                                   juce::Label& label,
                                                   const juce::String& name)
{
    slider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    slider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 78, 20);
    slider.setNumDecimalPlacesToDisplay (1);
    label.setText (name, juce::dontSendNotification);
    label.setJustificationType (juce::Justification::centred);
    addAndMakeVisible (slider);
    addAndMakeVisible (label);
}

void CompressorAudioProcessorEditor::paint (juce::Graphics& graphics)
{
    graphics.fillAll (juce::Colour (0xff1e1e24));
    graphics.setColour (juce::Colours::white);
    graphics.setFont (18.0f);
    graphics.drawText ("Feedforward Compressor", getLocalBounds().removeFromTop (30),
                       juce::Justification::centred, true);
}

void CompressorAudioProcessorEditor::resized()
{
    auto bounds = getLocalBounds().reduced (14);
    bounds.removeFromTop (28);
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

void CompressorAudioProcessorEditor::timerCallback()
{
    transferCurve.repaint();
}
