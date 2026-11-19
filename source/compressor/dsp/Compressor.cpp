#include "Compressor.h"
#include "CompressorMath.h"
#include <algorithm>
#include <cmath>

namespace
{
    constexpr float silenceFloor = 1.0e-6f;
    constexpr float minimumLevelDb = -120.0f;
    constexpr float decibelsPerNeper = 20.0f;

    float getLevelDb (float amplitude) noexcept
    {
        return amplitude < silenceFloor
             ? minimumLevelDb
             : decibelsPerNeper * std::log10 (amplitude);
    }

}

void Compressor::prepare (double sampleRate) noexcept
{
    currentSampleRate = sampleRate > 0.0 ? sampleRate : 44100.0;
    reset();
}

void Compressor::reset() noexcept
{
    previousGainReductionDb = 0.0f;
    currentInputLevelDb.store (minimumLevelDb, std::memory_order_relaxed);
    currentGainReductionDb.store (0.0f, std::memory_order_relaxed);
}

void Compressor::processBlock (juce::AudioBuffer<float>& buffer,
                               float thresholdDb,
                               float ratio,
                               float kneeWidthDb,
                               float attackMs,
                               float releaseMs,
                               float makeupGainDb) noexcept
{
    const int numChannels = buffer.getNumChannels();
    const int numSamples = buffer.getNumSamples();
    if (numChannels == 0 || numSamples == 0)
        return;

    thresholdDb = std::clamp (thresholdDb, -120.0f, 24.0f);
    ratio = std::max (1.0f, ratio);
    kneeWidthDb = std::max (0.0f, kneeWidthDb);
    attackMs = std::max (0.0f, attackMs);
    releaseMs = std::max (0.0f, releaseMs);

    const float sampleRate = static_cast<float> (currentSampleRate);
    const float alphaAttack = attackMs > 0.0f
                            ? std::exp (-1.0f / (0.001f * sampleRate * attackMs))
                            : 0.0f;
    const float alphaRelease = releaseMs > 0.0f
                             ? std::exp (-1.0f / (0.001f * sampleRate * releaseMs))
                             : 0.0f;
    const float makeupGain = std::pow (10.0f, makeupGainDb * 0.05f);

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
        const float inputLevelDb = getLevelDb (std::abs (sidechainSample));
        blockInputLevelSumDb += inputLevelDb;

        const float targetLevelDb = getCompressorTargetLevelDb (
            inputLevelDb, thresholdDb, ratio, kneeWidthDb);
        const float targetGainReductionDb = std::max (0.0f, inputLevelDb - targetLevelDb);
        const float alpha = targetGainReductionDb > previousGainReductionDb
                          ? alphaAttack
                          : alphaRelease;
        previousGainReductionDb = alpha * previousGainReductionDb
                                + (1.0f - alpha) * targetGainReductionDb;
        blockGainReductionSumDb += previousGainReductionDb;

        const float controlGain = makeupGain
                                * std::pow (10.0f, -previousGainReductionDb * 0.05f);
        outputLeft[sample] = leftSample * controlGain;
        if (outputRight != nullptr)
            outputRight[sample] = rightSample * controlGain;
    }

    const float blockAverageInputDb = blockInputLevelSumDb / static_cast<float> (numSamples);
    const float blockAverageGainReductionDb = blockGainReductionSumDb
                                            / static_cast<float> (numSamples);
    currentInputLevelDb.store (blockAverageInputDb, std::memory_order_relaxed);
    currentGainReductionDb.store (blockAverageGainReductionDb, std::memory_order_relaxed);
}

float Compressor::getCurrentInputLevelDb() const noexcept
{
    return currentInputLevelDb.load (std::memory_order_relaxed);
}

float Compressor::getCurrentGainReductionDb() const noexcept
{
    return currentGainReductionDb.load (std::memory_order_relaxed);
}
