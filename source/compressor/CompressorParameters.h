#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include "dsp/CompressorMath.h"

namespace CompressorParameterIDs
{
    inline constexpr const char* threshold = "threshold";
    inline constexpr const char* ratio = "ratio";
    inline constexpr const char* knee = "knee";
    inline constexpr const char* attack = "attack";
    inline constexpr const char* release = "release";
    inline constexpr const char* makeupGain = "makeupGain";
}

inline juce::AudioProcessorValueTreeState::ParameterLayout createCompressorParameterLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;

    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { CompressorParameterIDs::threshold, 1 }, "Threshold",
        juce::NormalisableRange<float> (-80.0f, 0.0f, 0.1f), -18.0f, "dB"));
    auto ratioAttributes = juce::AudioParameterFloatAttributes()
        .withStringFromValueFunction ([] (float value, int maximumLength)
        {
            const auto text = value >= compressorLimiterRatio
                            ? juce::String ("Inf:1")
                            : juce::String (value, 1) + ":1";
            return text.substring (0, maximumLength);
        })
        .withValueFromStringFunction ([] (const juce::String& text)
        {
            const auto normalized = text.trim().toLowerCase();
            if (normalized == "inf" || normalized == "infinity"
                || normalized == "inf:1" || normalized == "infinity:1")
                return compressorLimiterRatio;

            return juce::jlimit (1.0f, compressorLimiterRatio,
                                 normalized.upToFirstOccurrenceOf (":", false, false).getFloatValue());
        });
    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { CompressorParameterIDs::ratio, 1 }, "Ratio",
        juce::NormalisableRange<float> (1.0f, compressorLimiterRatio, 0.1f),
        4.0f, ratioAttributes));
    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { CompressorParameterIDs::knee, 1 }, "Knee",
        juce::NormalisableRange<float> (0.0f, 24.0f, 0.1f), 6.0f, "dB"));
    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { CompressorParameterIDs::attack, 1 }, "Attack",
        juce::NormalisableRange<float> (0.1f, 100.0f, 0.1f, 0.5f), 10.0f, "ms"));
    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { CompressorParameterIDs::release, 1 }, "Release",
        juce::NormalisableRange<float> (10.0f, 1000.0f, 1.0f, 0.5f), 100.0f, "ms"));
    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { CompressorParameterIDs::makeupGain, 1 }, "Make-Up Gain",
        juce::NormalisableRange<float> (-12.0f, 24.0f, 0.1f), 0.0f, "dB"));

    return layout;
}
