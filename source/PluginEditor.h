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

    // Tame/Moorer Toggles
    juce::ToggleButton autoGainButton { "Auto Gain (L2)" };
    juce::ToggleButton dampingButton  { "Damp Tail" };

    // 5 Dedicated Hardware Preset Panels
    std::unique_ptr<PresetPanel> customPanel;
    std::unique_ptr<PresetPanel> vibratoPanel;
    std::unique_ptr<PresetPanel> flangerPanel;
    std::unique_ptr<PresetPanel> chorusPanel;
    std::unique_ptr<PresetPanel> doublingPanel;

    using ButtonAttachment = juce::AudioProcessorValueTreeState::ButtonAttachment;
    std::unique_ptr<ButtonAttachment> autoGainAttachment;
    std::unique_ptr<ButtonAttachment> dampingAttachment;

    void updateActivePanel (int activeIndex);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AudioPluginAudioProcessorEditor)
};