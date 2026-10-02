#pragma once
#include <juce_audio_processors/juce_audio_processors.h>

namespace ParameterIDs
{
    inline const char* delay      = "delay";
    inline const char* blend      = "blend";
    inline const char* feedforward = "feedforward";
    inline const char* feedback   = "feedback";
}

inline juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;

    // Example parameter bounds
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{ ParameterIDs::delay, 1 }, "Delay (ms)", 
        juce::NormalisableRange<float>(0.1f, 50.0f, 0.01f), 10.0f));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{ ParameterIDs::blend, 1 }, "Blend (BL)", 
        juce::NormalisableRange<float>(-1.0f, 1.0f, 0.01f), 1.0f));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{ ParameterIDs::feedforward, 1 }, "Feedforward (FF)", 
        juce::NormalisableRange<float>(-1.0f, 1.0f, 0.01f), 0.5f));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{ ParameterIDs::feedback, 1 }, "Feedback (FB)", 
        juce::NormalisableRange<float>(-0.99f, 0.99f, 0.01f), 0.0f));

    return layout;
}