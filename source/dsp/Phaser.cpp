#include "Phaser.h"
#include "PhaserMath.h"

#include <algorithm>
#include <cmath>

void Phaser::prepare(double newSampleRate, int numChannels)
{
    sampleRate = newSampleRate;
    channelStates.resize(static_cast<size_t>(juce::jmax(1, numChannels)));

    constexpr double rampDurationSeconds = 0.02;
    rateHz.reset(sampleRate, rampDurationSeconds);
    depth.reset(sampleRate, rampDurationSeconds);
    mix.reset(sampleRate, rampDurationSeconds);
    rateHz.setCurrentAndTargetValue(0.5f);
    depth.setCurrentAndTargetValue(0.75f);
    mix.setCurrentAndTargetValue(0.5f);
    reset();
}

void Phaser::reset()
{
    phase = 0.0;
    for (auto& channel : channelStates)
        for (auto& state : channel)
            state = {};
}

void Phaser::setStages(int newStages)
{
    stages = juce::jlimit(2, maximumStages, newStages);
}

void Phaser::setRateHz(float newRateHz)
{
    rateHz.setTargetValue(juce::jlimit(0.05f, 8.0f, newRateHz));
}

void Phaser::setDepth(float newDepth)
{
    depth.setTargetValue(juce::jlimit(0.0f, 1.0f, newDepth));
}

void Phaser::setMix(float newMix)
{
    mix.setTargetValue(juce::jlimit(0.0f, 1.0f, newMix));
}

void Phaser::processBlock(juce::AudioBuffer<float>& buffer)
{
    const int numChannels = buffer.getNumChannels();
    if (numChannels == 0 || static_cast<size_t>(numChannels) > channelStates.size()
        || sampleRate <= 0.0)
        return;

    const double maximumFrequency = std::min(6000.0, sampleRate * 0.4);
    const double minimumFrequency = std::min(150.0, maximumFrequency * 0.25);
    const double logFrequencyRange = std::log(maximumFrequency / minimumFrequency);
    const double phaseIncrementScale = 1.0 / sampleRate;

    for (int sample = 0; sample < buffer.getNumSamples(); ++sample)
    {
        const float currentRate = rateHz.getNextValue();
        const float currentDepth = depth.getNextValue();
        const float currentMix = mix.getNextValue();
        const float lfo = static_cast<float>(
            0.5 * (std::sin(2.0 * juce::MathConstants<double>::pi * phase) + 1.0));
        const double octaveOffset = (static_cast<double>(lfo) - 0.5)
                                  * 2.0 * static_cast<double>(currentDepth);
        const double frequencyScale = std::exp2(octaveOffset);

        for (int channel = 0; channel < numChannels; ++channel)
        {
            auto* data = buffer.getWritePointer(channel);
            const float dry = data[sample];
            float wet = dry;
            auto& states = channelStates[static_cast<size_t>(channel)];

            for (int stage = 0; stage < stages; ++stage)
            {
                const double stagePosition = static_cast<double>(stage)
                                           / static_cast<double>(stages - 1);
                const double baseFrequency = minimumFrequency
                    * std::exp(stagePosition * logFrequencyRange);
                const double frequency = std::min(
                    baseFrequency * frequencyScale, sampleRate * 0.45);
                const auto coefficients = PhaserMath::getAllPassCoefficients(frequency, sampleRate);
                const double a1 = coefficients.d * (1.0 - coefficients.c);
                auto& state = states[static_cast<size_t>(stage)];
                const float xhNew = static_cast<float>(
                    static_cast<double>(wet) - a1 * state.xh1 + coefficients.c * state.xh2);
                const float output = static_cast<float>(
                    -coefficients.c * xhNew + a1 * state.xh1 + state.xh2);

                state.xh2 = state.xh1;
                state.xh1 = xhNew;
                wet = output;
            }

            data[sample] = dry + currentMix * (wet - dry);
        }

        phase += static_cast<double>(currentRate) * phaseIncrementScale;
        if (phase >= 1.0)
            phase -= std::floor(phase);
    }
}
