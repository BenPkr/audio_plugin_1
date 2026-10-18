#pragma once

#include <juce_audio_processors/juce_audio_processors.h>

namespace ParameterIDs
{
    inline const char* mode         = "mode";
    inline const char* preset       = "preset";
    inline const char* delay        = "delay";
    inline const char* blend        = "blend";
    inline const char* feedforward  = "feedforward";
    inline const char* feedback     = "feedback";

    inline const char* modEnable    = "modEnable";
    inline const char* modType      = "modType";
    inline const char* modDepth     = "modDepth";
    inline const char* modFrequency = "modFrequency";
    inline const char* tremFrequency = "tremFrequency";
    inline const char* phaserStages = "phaserStages";
    inline const char* phaserRate = "phaserRate";
    inline const char* phaserDepth = "phaserDepth";
    inline const char* phaserMix = "phaserMix";

    // --- Tame / Moorer Toggles ---
    inline const char* autoGain     = "autoGain";
    inline const char* damping      = "damping";
    inline const char* tremWaveform = "tremWaveform";
}

inline juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;

    layout.add(std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID{ ParameterIDs::mode, 1 }, "Processing Mode",
        juce::StringArray{ "Comb Filter", "Ring Modulator", "Tremolo", "Phaser" }, 0));

    layout.add(std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID{ ParameterIDs::preset, 1 }, "Preset",
        juce::StringArray{ "Custom (Free)", "Vibrato", "Flanger", "Chorus", "Doubling" }, 0));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{ ParameterIDs::delay, 1 }, "Delay (ms)", 
        juce::NormalisableRange<float>(0.0f, 100.0f, 0.01f), 10.0f));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{ ParameterIDs::blend, 1 }, "Blend / Ring Mix", 
        juce::NormalisableRange<float>(-1.0f, 1.0f, 0.01f), 1.0f));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{ ParameterIDs::feedforward, 1 }, "Feedforward (FF)", 
        juce::NormalisableRange<float>(-1.0f, 1.0f, 0.01f), 0.5f));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{ ParameterIDs::feedback, 1 }, "Feedback (FB)", 
        juce::NormalisableRange<float>(-0.99f, 0.99f, 0.01f), 0.0f));

    layout.add(std::make_unique<juce::AudioParameterBool>(
        juce::ParameterID{ ParameterIDs::modEnable, 1 }, "Modulation Enable", false));

    layout.add(std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID{ ParameterIDs::modType, 1 }, "Modulation Type", 
        juce::StringArray{ "Sine", "Lowpass Noise" }, 0));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{ ParameterIDs::modDepth, 1 }, "Modulation Depth", 
        juce::NormalisableRange<float>(0.0f, 100.0f, 0.01f), 1.0f));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{ ParameterIDs::modFrequency, 1 }, "LFO / Carrier Frequency (Hz)", 
        juce::NormalisableRange<float>(0.05f, 2000.0f, 0.05f, 0.5f), 1.0f));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{ ParameterIDs::tremFrequency, 1 }, "Tremolo Rate (Hz)",
        juce::NormalisableRange<float>(0.1f, 20.0f, 0.01f, 0.5f), 5.0f));

    layout.add(std::make_unique<juce::AudioParameterInt>(
        juce::ParameterID{ ParameterIDs::phaserStages, 1 }, "Phaser Notches", 2, 8, 4));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{ ParameterIDs::phaserRate, 1 }, "Phaser Rate (Hz)",
        juce::NormalisableRange<float>(0.05f, 8.0f, 0.01f, 0.5f), 0.5f));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{ ParameterIDs::phaserDepth, 1 }, "Phaser Sweep Depth (%)",
        juce::NormalisableRange<float>(0.0f, 100.0f, 0.1f), 75.0f));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{ ParameterIDs::phaserMix, 1 }, "Phaser Mix (%)",
        juce::NormalisableRange<float>(0.0f, 100.0f, 0.1f), 50.0f));

    // --- New Tame Toggles ---
    layout.add(std::make_unique<juce::AudioParameterBool>(
        juce::ParameterID{ ParameterIDs::autoGain, 1 }, "Auto Gain Balance (L-infinity)", true));

    layout.add(std::make_unique<juce::AudioParameterBool>(
        juce::ParameterID{ ParameterIDs::damping, 1 }, "Moorer High-Damping", true));

    layout.add(std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID{ ParameterIDs::tremWaveform, 1 }, "Tremolo Waveform",
        juce::StringArray{ "Sine", "Triangle", "Square" }, 0));

    return layout;
}