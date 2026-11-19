#pragma once

#include "ExpanderProcessor.h"
#include "gui/ExpanderTransferCurve.h"
#include <juce_gui_basics/juce_gui_basics.h>
#include <array>
#include <memory>

class ExpanderAudioProcessorEditor final : public juce::AudioProcessorEditor,
                                           private juce::Timer
{
public:
    explicit ExpanderAudioProcessorEditor (ExpanderAudioProcessor& processor);

    void paint (juce::Graphics& graphics) override;
    void resized() override;

private:
    using SliderAttachment = juce::AudioProcessorValueTreeState::SliderAttachment;

    void timerCallback() override;

    ExpanderAudioProcessor& audioProcessor;
    ExpanderTransferCurve transferCurve;
    std::array<juce::Slider, 4> sliders;
    std::array<juce::Label, 4> labels;
    std::array<std::unique_ptr<SliderAttachment>, 4> attachments;
    const std::array<const char*, 4> parameterIDs {
        ExpanderParameterIDs::threshold,
        ExpanderParameterIDs::ratio,
        ExpanderParameterIDs::attack,
        ExpanderParameterIDs::release
    };
    const std::array<const char*, 4> parameterNames {
        "Threshold", "Ratio", "Attack", "Release"
    };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ExpanderAudioProcessorEditor)
};
