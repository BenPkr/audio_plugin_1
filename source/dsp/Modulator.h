#pragma once

#include <juce_core/juce_core.h>

class Modulator
{
public:
    enum class Type
    {
        Sine = 0,
        LowpassNoise = 1
    };

    Modulator() = default;

    void prepare(double newSampleRate)
    {
        sampleRate = newSampleRate;
        reset();
    }

    void reset()
    {
        phase = 0.0f;
        rawNoiseState = 0.0f;
        smoothedNoiseState = 0.0f;
    }

    void setType(Type newType)          { type = newType; }
    void setDepthMs(float newDepthMs)   { depthMs = newDepthMs; }
    void setFrequency(float newFreqHz)  { frequencyHz = newFreqHz; }

    float processSample()
    {
        if (sampleRate <= 0.0)
            return 0.0f;

        if (type == Type::Sine)
        {
            phase += (2.0f * static_cast<float>(M_PI) * frequencyHz) / static_cast<float>(sampleRate);
            if (phase >= 2.0f * static_cast<float>(M_PI))
                phase -= 2.0f * static_cast<float>(M_PI);

            return std::sin(phase) * depthMs;
        }
        else // Lowpass Noise Mode
        {
            // 1. Generate white noise sample
            const float white = (random.nextFloat() * 2.0f) - 1.0f;

            // 2. Heavy Lowpass Filter (Limit cutoff to max 3.0 Hz for smooth chorus gliding)
            const float effectiveCutoff = juce::jlimit(0.1f, 3.0f, frequencyHz);
            const float dt = 1.0f / static_cast<float>(sampleRate);
            const float alpha = juce::jlimit(0.00001f, 0.1f, 2.0f * static_cast<float>(M_PI) * effectiveCutoff * dt);

            rawNoiseState += alpha * (white - rawNoiseState);

            // 3. Second-stage exponential smoother to eliminate sample-to-sample clicks/jitter
            constexpr float smoothCoeff = 0.001f;
            smoothedNoiseState += smoothCoeff * (rawNoiseState - smoothedNoiseState);

            return juce::jlimit(-1.0f, 1.0f, smoothedNoiseState * 4.0f) * depthMs;
        }
    }

private:
    double sampleRate { 44100.0 };
    Type type { Type::Sine };

    float depthMs { 1.0f };
    float frequencyHz { 1.0f };

    float phase { 0.0f };
    float rawNoiseState { 0.0f };
    float smoothedNoiseState { 0.0f };

    juce::Random random;
};