#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include "PluginProcessor.h"
#include "gui/FrequencyResponsePlot.h"
#include "gui/PoleZeroPlot.h"
#include "gui/ModulationSignalPlot.h"

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

    // Visualizations
    FrequencyResponsePlot frequencyResponsePlot;
    PoleZeroPlot poleZeroPlot;
    ModulationSignalPlot modulationSignalPlot;

    // Base Filter Controls & Labels
    juce::Slider delaySlider;
    juce::Slider blendSlider;
    juce::Slider feedforwardSlider;
    juce::Slider feedbackSlider;

    juce::Label delayLabel;
    juce::Label blendLabel;
    juce::Label feedforwardLabel;
    juce::Label feedbackLabel;

    // Modulation Controls & Labels
    juce::ToggleButton modEnableButton { "Modulation" };
    juce::ComboBox modTypeComboBox;
    juce::Slider modDepthSlider;
    juce::Slider modFrequencySlider;

    juce::Label modTypeLabel;
    juce::Label modDepthLabel;
    juce::Label modFrequencyLabel;

    // APVTS Attachments
    using SliderAttachment   = juce::AudioProcessorValueTreeState::SliderAttachment;
    using ButtonAttachment   = juce::AudioProcessorValueTreeState::ButtonAttachment;
    using ComboBoxAttachment = juce::AudioProcessorValueTreeState::ComboBoxAttachment;

    std::unique_ptr<SliderAttachment> delayAttachment;
    std::unique_ptr<SliderAttachment> blendAttachment;
    std::unique_ptr<SliderAttachment> feedforwardAttachment;
    std::unique_ptr<SliderAttachment> feedbackAttachment;

    std::unique_ptr<ButtonAttachment> modEnableAttachment;
    std::unique_ptr<ComboBoxAttachment> modTypeAttachment;
    std::unique_ptr<SliderAttachment> modDepthAttachment;
    std::unique_ptr<SliderAttachment> modFrequencyAttachment;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AudioPluginAudioProcessorEditor)
};