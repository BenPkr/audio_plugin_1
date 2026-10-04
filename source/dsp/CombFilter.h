#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_dsp/juce_dsp.h>
#include <vector>
#include "Modulator.h"

class CombFilter
{
public:
    CombFilter() = default;

    /** Allocates delay lines and sets up parameter smoothing on the main thread. */
    void prepare(double sampleRate, int samplesPerBlock, int numChannels);

    /** Resets inner states and clears delay line history. */
    void reset();

    /** Processes an entire audio block in real-time on the audio thread. */
    void processBlock(juce::AudioBuffer<float>& buffer);

    /** Setters for active DSP parameters (call from processBlock / parameter update). */
    void setDelayMs(float newDelayMs);
    void setBlend(float newBlend);
    void setFeedforward(float newFeedforward);
    void setFeedback(float newFeedback);

    // Modulation Setters
    void setModEnabled(bool enabled);
    void setModType(int typeIndex);
    void setModDepthMs(float depthMs);
    void setModFrequencyHz(float freqHz);

private:
    double currentSampleRate { 44100.0 };

    // Smoothed values prevent clicking when tweaking knobs live
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> smoothedDelayMs;
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> smoothedBlend;
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> smoothedFeedforward;
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> smoothedFeedback;

    bool modEnabled { false };
    Modulator modulator;

    // Fractional delay line per channel using Lagrange 3rd order interpolation
    std::vector<juce::dsp::DelayLine<float, juce::dsp::DelayLineInterpolationTypes::Lagrange3rd>> delayLines;
    
    static constexpr float maxDelayMs = 100.0f;
};