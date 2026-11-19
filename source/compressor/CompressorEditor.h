#pragma once

#include "CompressorProcessor.h"
#include "gui/CompressorTransferCurve.h"
#include <juce_gui_basics/juce_gui_basics.h>
#include <array>
#include <memory>

class CompressorAudioProcessorEditor final : public juce::AudioProcessorEditor,
                                             private juce::Timer
{
public:
    explicit CompressorAudioProcessorEditor (CompressorAudioProcessor& processor);
    ~CompressorAudioProcessorEditor() override = default;

    void paint (juce::Graphics& graphics) override;
    void resized() override;

private:
    using SliderAttachment = juce::AudioProcessorValueTreeState::SliderAttachment;

    void setupSlider (juce::Slider& slider, juce::Label& label, const juce::String& name);
    void timerCallback() override;

    CompressorAudioProcessor& audioProcessor;
    CompressorTransferCurve transferCurve;
    std::array<juce::Slider, 6> sliders;
    std::array<juce::Label, 6> labels;
    std::array<std::unique_ptr<SliderAttachment>, 6> attachments;
    const std::array<const char*, 6> parameterIDs {
        CompressorParameterIDs::threshold,
        CompressorParameterIDs::ratio,
        CompressorParameterIDs::knee,
        CompressorParameterIDs::attack,
        CompressorParameterIDs::release,
        CompressorParameterIDs::makeupGain
    };
    const std::array<const char*, 6> parameterNames {
        "Threshold", "Ratio", "Knee", "Attack", "Release", "Make-Up"
    };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (CompressorAudioProcessorEditor)
};
