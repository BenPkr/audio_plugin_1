#pragma once

#include <juce_audio_processors/juce_audio_processors.h>

namespace ExpanderParameterIDs
{
    inline constexpr const char* threshold = "expanderThreshold";
    inline constexpr const char* ratio = "expanderRatio";
    inline constexpr const char* attack = "expanderAttack";
    inline constexpr const char* release = "expanderRelease";
}

inline juce::AudioProcessorValueTreeState::ParameterLayout createExpanderParameterLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;

    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { ExpanderParameterIDs::threshold, 1 }, "Threshold",
        juce::NormalisableRange<float> (-80.0f, 0.0f, 0.1f), -40.0f, "dB"));
    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { ExpanderParameterIDs::ratio, 1 }, "Ratio",
        juce::NormalisableRange<float> (1.0f, 10.0f, 0.1f), 2.0f, ":1"));
    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { ExpanderParameterIDs::attack, 1 }, "Attack",
        juce::NormalisableRange<float> (0.1f, 100.0f, 0.1f, 0.5f), 10.0f, "ms"));
    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { ExpanderParameterIDs::release, 1 }, "Release",
        juce::NormalisableRange<float> (10.0f, 1000.0f, 1.0f, 0.5f), 100.0f, "ms"));

    return layout;
}
