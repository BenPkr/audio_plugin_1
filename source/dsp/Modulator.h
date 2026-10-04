#pragma once

#include <juce_core/juce_core.h>
#include <cmath>

class Modulator
{
public:
    enum class Type
    {
        Sine = 0,
        LowpassNoise
    };

    Modulator() = default;

    void prepare(double sampleRate)
    {
        currentSampleRate = sampleRate;
        reset();
    }

    void reset()
    {
        sinePhase = 0.0f;
        smoothedNoise = 0.0f;
    }

    void setFrequency(float freqHz) { frequencyHz = freqHz; }
    void setDepthMs(float depth)    { depthMs = depth; }
    void setType(Type newType)      { type = newType; }

    /** Returns the current delay modulation offset in milliseconds for a single sample frame. */
    float processSample()
    {
        if (depthMs <= 0.0f)
            return 0.0f;

        float modSignal = 0.0f;

        if (type == Type::Sine)
        {
            modSignal = std::sin(sinePhase);

            // Advance phase
            const float phaseIncrement = (2.0f * static_cast<float>(M_PI) * frequencyHz) / static_cast<float>(currentSampleRate);
            sinePhase += phaseIncrement;
            if (sinePhase >= 2.0f * static_cast<float>(M_PI))
                sinePhase -= 2.0f * static_cast<float>(M_PI);
        }
        else if (type == Type::LowpassNoise)
        {
            // Generate white noise bounded [-1, 1]
            const float whiteNoise = random.nextFloat() * 2.0f - 1.0f;

            // One-pole lowpass filter tuned by frequency parameter
            // Fc = frequencyHz
            const float cutoff = juce::jlimit(0.1f, 100.0f, frequencyHz);
            const float x = std::exp(-2.0f * static_cast<float>(M_PI) * cutoff / static_cast<float>(currentSampleRate));
            const float alpha = 1.0f - x;

            smoothedNoise += alpha * (whiteNoise - smoothedNoise);
            modSignal = smoothedNoise * 3.0f; // Scale up noise variance
        }

        return modSignal * depthMs;
    }

private:
    double currentSampleRate { 44100.0 };
    float sinePhase { 0.0f };
    float smoothedNoise { 0.0f };

    float frequencyHz { 1.0f };
    float depthMs { 0.0f };
    Type type { Type::Sine };

    juce::Random random;
};