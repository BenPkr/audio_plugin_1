#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include "PluginProcessor.h"
#include "gui/FrequencyResponsePlot.h"
#include "gui/PoleZeroPlot.h"
#include "gui/ModulationSignalPlot.h"
#include "gui/PresetPanel.h"

class AudioPluginAudioProcessorEditor  : public juce::AudioProcessorEditor
{
public:
    explicit AudioPluginAudioProcessorEditor (AudioPluginAudioProcessor&);
    ~AudioPluginAudioProcessorEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    AudioPluginAudioProcessor& audioProcessor;

    // Visualizations
    FrequencyResponsePlot frequencyResponsePlot;
    PoleZeroPlot poleZeroPlot;
    ModulationSignalPlot modulationSignalPlot;

    // Mode Selector
    juce::Slider modeDial;
    juce::Label modeDialLabel;

    // 5 Dedicated Hardware Preset Panels
    std::unique_ptr<PresetPanel> customPanel;
    std::unique_ptr<PresetPanel> vibratoPanel;
    std::unique_ptr<PresetPanel> flangerPanel;
    std::unique_ptr<PresetPanel> chorusPanel;
    std::unique_ptr<PresetPanel> doublingPanel;

    using ComboBoxAttachment = juce::AudioProcessorValueTreeState::ComboBoxAttachment;
    std::unique_ptr<ComboBoxAttachment> presetAttachment;

    void updateActivePanel (int activeIndex);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AudioPluginAudioProcessorEditor)
};