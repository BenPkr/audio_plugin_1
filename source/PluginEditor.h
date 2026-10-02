#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include "PluginProcessor.h"
#include "gui/FrequencyResponsePlot.h"
#include "gui/PoleZeroPlot.h"

//==============================================================================
class AudioPluginAudioProcessorEditor  : public juce::AudioProcessorEditor
{
public:
    explicit AudioPluginAudioProcessorEditor (AudioPluginAudioProcessor&);
    ~AudioPluginAudioProcessorEditor() override;

    //==============================================================================
    void paint (juce::Graphics&) override;
    void resized() override;

private:
    AudioPluginAudioProcessor& audioProcessor;

    // Custom Visualizations
    FrequencyResponsePlot frequencyResponsePlot;
    PoleZeroPlot poleZeroPlot;

    // GUI Controls & Labels
    juce::Slider delaySlider;
    juce::Slider blendSlider;
    juce::Slider feedforwardSlider;
    juce::Slider feedbackSlider;

    juce::Label delayLabel;
    juce::Label blendLabel;
    juce::Label feedforwardLabel;
    juce::Label feedbackLabel;

    // APVTS Attachments to link Sliders to Parameters automatically
    using SliderAttachment = juce::AudioProcessorValueTreeState::SliderAttachment;

    std::unique_ptr<SliderAttachment> delayAttachment;
    std::unique_ptr<SliderAttachment> blendAttachment;
    std::unique_ptr<SliderAttachment> feedforwardAttachment;
    std::unique_ptr<SliderAttachment> feedbackAttachment;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AudioPluginAudioProcessorEditor)
};