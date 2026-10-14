#pragma once

#include <juce_audio_processors/juce_audio_processors.h>

namespace ParameterIDs
{
    inline const char* preset       = "preset";
    inline const char* delay        = "delay";
    inline const char* blend        = "blend";
    inline const char* feedforward  = "feedforward";
    inline const char* feedback     = "feedback";

    inline const char* modEnable    = "modEnable";
    inline const char* modType      = "modType";
    inline const char* modDepth     = "modDepth";
    inline const char* modFrequency = "modFrequency";
}

inline juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;

    // Presets Dropdown
    layout.add(std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID{ ParameterIDs::preset, 1 }, "Preset",
        juce::StringArray{ "Custom (Free)", "Vibrato", "Flanger", "Chorus", "Doubling" }, 0));

    // Base Comb Parameters
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{ ParameterIDs::delay, 1 }, "Delay (ms)", 
        juce::NormalisableRange<float>(0.1f, 100.0f, 0.01f), 10.0f));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{ ParameterIDs::blend, 1 }, "Blend (BL)", 
        juce::NormalisableRange<float>(-1.0f, 1.0f, 0.01f), 1.0f));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{ ParameterIDs::feedforward, 1 }, "Feedforward (FF)", 
        juce::NormalisableRange<float>(-1.0f, 1.0f, 0.01f), 0.5f));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{ ParameterIDs::feedback, 1 }, "Feedback (FB)", 
        juce::NormalisableRange<float>(-0.99f, 0.99f, 0.01f), 0.0f));

    // Modulation Parameters
    layout.add(std::make_unique<juce::AudioParameterBool>(
        juce::ParameterID{ ParameterIDs::modEnable, 1 }, "Modulation Enable", false));

    layout.add(std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID{ ParameterIDs::modType, 1 }, "Modulation Type", 
        juce::StringArray{ "Sine", "Lowpass Noise" }, 0));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{ ParameterIDs::modDepth, 1 }, "Mod Depth (ms)", 
        juce::NormalisableRange<float>(0.0f, 100.0f, 0.01f), 1.0f));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{ ParameterIDs::modFrequency, 1 }, "Mod Rate (Hz)", 
        juce::NormalisableRange<float>(0.05f, 20.0f, 0.05f, 0.5f), 1.0f));

    return layout;
}