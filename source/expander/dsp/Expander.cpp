#include "Expander.h"
#include <algorithm>
#include <cmath>

namespace
{
    constexpr float silenceFloor = 1.0e-6f;
    constexpr float minimumLevelDb = -120.0f;
}

void Expander::prepare (double sampleRate) noexcept
{
    currentSampleRate = sampleRate > 0.0 ? sampleRate : 44100.0;
    reset();
}

void Expander::reset() noexcept
{
    previousGainReductionDb = 0.0f;
    currentInputLevelDb.store (minimumLevelDb, std::memory_order_relaxed);
    currentGainReductionDb.store (0.0f, std::memory_order_relaxed);
}

void Expander::processBlock (juce::AudioBuffer<float>& buffer,
                             float thresholdDb,
                             float ratio,
                             float attackMs,
                             float releaseMs) noexcept
{
    const int numChannels = buffer.getNumChannels();
    const int numSamples = buffer.getNumSamples();
    if (numChannels == 0 || numSamples == 0)
        return;

    ratio = juce::jlimit (1.0f, 10.0f, ratio);
    const float sampleRate = static_cast<float> (currentSampleRate);
    const float alphaAttack = attackMs > 0.0f
                            ? std::exp (-1.0f / (0.001f * sampleRate * attackMs))
                            : 0.0f;
    const float alphaRelease = releaseMs > 0.0f
                             ? std::exp (-1.0f / (0.001f * sampleRate * releaseMs))
                             : 0.0f;

    const float* inputLeft = buffer.getReadPointer (0);
    const float* inputRight = numChannels > 1 ? buffer.getReadPointer (1) : inputLeft;
    float* outputLeft = buffer.getWritePointer (0);
    float* outputRight = numChannels > 1 ? buffer.getWritePointer (1) : nullptr;

    float blockInputLevelSumDb = 0.0f;
    float blockGainReductionSumDb = 0.0f;

    for (int sample = 0; sample < numSamples; ++sample)
    {
        const float leftSample = inputLeft[sample];
        const float rightSample = numChannels > 1 ? inputRight[sample] : leftSample;
        const float sidechainSample = numChannels > 1
                                    ? 0.5f * (leftSample + rightSample)
                                    : leftSample;
        const float magnitude = std::abs (sidechainSample);
        const float inputLevelDb = magnitude < silenceFloor
                                 ? minimumLevelDb
                                 : 20.0f * std::log10 (magnitude);
        blockInputLevelSumDb += inputLevelDb;

        const float targetGainReductionDb = std::max (
            0.0f, thresholdDb - inputLevelDb) * (ratio - 1.0f);
        const float alpha = targetGainReductionDb > previousGainReductionDb
                          ? alphaAttack
                          : alphaRelease;
        previousGainReductionDb = alpha * previousGainReductionDb
                                + (1.0f - alpha) * targetGainReductionDb;
        blockGainReductionSumDb += previousGainReductionDb;

        const float gain = std::pow (10.0f, -previousGainReductionDb * 0.05f);
        outputLeft[sample] = leftSample * gain;
        if (outputRight != nullptr)
            outputRight[sample] = rightSample * gain;
    }

    currentInputLevelDb.store (blockInputLevelSumDb / static_cast<float> (numSamples),
                               std::memory_order_relaxed);
    currentGainReductionDb.store (blockGainReductionSumDb / static_cast<float> (numSamples),
                                  std::memory_order_relaxed);
}

float Expander::getCurrentInputLevelDb() const noexcept
{
    return currentInputLevelDb.load (std::memory_order_relaxed);
}

float Expander::getCurrentGainReductionDb() const noexcept
{
    return currentGainReductionDb.load (std::memory_order_relaxed);
}
