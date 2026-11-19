#pragma once

#include <algorithm>
#include <cmath>

inline constexpr float compressorLimiterRatio = 20.0f;

inline float getCompressorTargetLevelDb (float inputLevelDb,
                                         float thresholdDb,
                                         float ratio,
                                         float kneeWidthDb) noexcept
{
    const float overshoot = inputLevelDb - thresholdDb;
    const float halfKnee = 0.5f * kneeWidthDb;
    const float inverseRatio = ratio >= compressorLimiterRatio ? 0.0f : 1.0f / ratio;

    if (kneeWidthDb > 0.0f && std::abs (overshoot) <= halfKnee)
    {
        const float kneeInput = overshoot + halfKnee;
        return inputLevelDb + ((inverseRatio - 1.0f) * kneeInput * kneeInput)
                            / (2.0f * kneeWidthDb);
    }

    if (overshoot > halfKnee)
        return thresholdDb + overshoot * inverseRatio;

    return inputLevelDb;
}
