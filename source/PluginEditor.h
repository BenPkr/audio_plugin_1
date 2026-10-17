#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include "PluginProcessor.h"
#include "gui/FrequencyResponsePlot.h"
#include "gui/PhaseResponsePlot.h"
#include "gui/PoleZeroPlot.h"
#include "gui/ModulationSignalPlot.h"
#include "gui/PresetPanel.h"
#include "gui/AudioScopePlot.h"

class AudioPluginAudioProcessorEditor  : public juce::AudioProcessorEditor,
                                         private juce::Timer
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
    PhaseResponsePlot phaseResponsePlot;
    PoleZeroPlot poleZeroPlot;
    ModulationSignalPlot modulationSignalPlot;
    AudioScopePlot audioScopePlot;

    juce::ComboBox processorModeBox;
    juce::Label processorModeLabel;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> processorModeAttachment;

    // Existing comb preset selector
    juce::Slider modeDial;
    juce::Label modeDialLabel;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> presetAttachment;

    juce::GroupComponent modeControlsGroup;
    juce::Slider modeBlendSlider, modeFrequencySlider, tremoloFrequencySlider, modeDepthSlider;
    juce::Slider phaserMixSlider, phaserStagesSlider, phaserRateSlider, phaserDepthSlider;
    juce::Label modeBlendLabel, modeFrequencyLabel, tremoloFrequencyLabel, modeDepthLabel, tremWaveformLabel;
    juce::Label phaserMixLabel, phaserStagesLabel, phaserRateLabel, phaserDepthLabel;
    juce::ComboBox tremWaveformBox;

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
    using SliderAttachment = juce::AudioProcessorValueTreeState::SliderAttachment;
    using ComboBoxAttachment = juce::AudioProcessorValueTreeState::ComboBoxAttachment;
    std::unique_ptr<ButtonAttachment> autoGainAttachment;
    std::unique_ptr<ButtonAttachment> dampingAttachment;
    std::unique_ptr<SliderAttachment> modeBlendAttachment;
    std::unique_ptr<SliderAttachment> modeFrequencyAttachment;
    std::unique_ptr<SliderAttachment> tremoloFrequencyAttachment;
    std::unique_ptr<SliderAttachment> modeDepthAttachment;
    std::unique_ptr<ComboBoxAttachment> tremWaveformAttachment;
    std::unique_ptr<SliderAttachment> phaserMixAttachment;
    std::unique_ptr<SliderAttachment> phaserStagesAttachment;
    std::unique_ptr<SliderAttachment> phaserRateAttachment;
    std::unique_ptr<SliderAttachment> phaserDepthAttachment;

    void updateActivePanel (int activeIndex);
    void updateGuiElementStates (int activeMode);
    void timerCallback() override;
    int lastGuiMode { -1 };
    bool presetPanelsReady { false };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AudioPluginAudioProcessorEditor)
};