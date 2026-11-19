#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include <atomic>

class Compressor
{
public:
    void prepare (double sampleRate) noexcept;
    void reset() noexcept;
    void processBlock (juce::AudioBuffer<float>& buffer,
                       float thresholdDb,
                       float ratio,
                       float kneeWidthDb,
                       float attackMs,
                       float releaseMs,
                       float makeupGainDb) noexcept;

    float getCurrentInputLevelDb() const noexcept;
    float getCurrentGainReductionDb() const noexcept;

private:
    double currentSampleRate { 44100.0 };
    float previousGainReductionDb { 0.0f };
    std::atomic<float> currentInputLevelDb { -120.0f };
    std::atomic<float> currentGainReductionDb { 0.0f };
};
