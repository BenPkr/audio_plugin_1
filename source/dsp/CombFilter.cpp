#include "CombFilter.h"
#include <algorithm>

void CombFilter::prepare(double sampleRate, int samplesPerBlock, int numChannels)
{
    currentSampleRate = sampleRate;
    const int channelsToPrepare = std::max(1, numChannels);

    constexpr float rampDurationSec = 0.020f;
    smoothedDelayMs.reset(sampleRate, rampDurationSec);
    smoothedBlend.reset(sampleRate, rampDurationSec);
    smoothedFeedforward.reset(sampleRate, rampDurationSec);
    smoothedFeedback.reset(sampleRate, rampDurationSec);

    smoothedDelayMs.setCurrentAndTargetValue(10.0f);
    smoothedBlend.setCurrentAndTargetValue(1.0f);
    smoothedFeedforward.setCurrentAndTargetValue(0.5f);
    smoothedFeedback.setCurrentAndTargetValue(0.0f);

    modulator.prepare(sampleRate);

    const auto maxDelaySamples = static_cast<int>(std::ceil((maxDelayMs / 1000.0f) * currentSampleRate));
    delayLines.resize(static_cast<size_t>(channelsToPrepare));
    dampedFeedbackState.assign(static_cast<size_t>(channelsToPrepare), 0.0f);

    juce::dsp::ProcessSpec singleChannelSpec { sampleRate, static_cast<juce::uint32>(samplesPerBlock), 1 };

    for (auto& delayLine : delayLines)
    {
        delayLine.prepare(singleChannelSpec);
        delayLine.setMaximumDelayInSamples(maxDelaySamples);
        delayLine.reset();
    }
}

void CombFilter::reset()
{
    smoothedDelayMs.reset(currentSampleRate, 0.020f);
    smoothedBlend.reset(currentSampleRate, 0.020f);
    smoothedFeedforward.reset(currentSampleRate, 0.020f);
    smoothedFeedback.reset(currentSampleRate, 0.020f);

    modulator.reset();
    std::fill(dampedFeedbackState.begin(), dampedFeedbackState.end(), 0.0f);

    for (auto& delayLine : delayLines)
        delayLine.reset();
}

void CombFilter::setDelayMs(float newDelayMs)           { smoothedDelayMs.setTargetValue(juce::jlimit(0.1f, maxDelayMs, newDelayMs)); }
void CombFilter::setBlend(float newBlend)               { smoothedBlend.setTargetValue(juce::jlimit(-1.0f, 1.0f, newBlend)); }
void CombFilter::setFeedforward(float newFeedforward)   { smoothedFeedforward.setTargetValue(juce::jlimit(-1.0f, 1.0f, newFeedforward)); }
void CombFilter::setFeedback(float newFeedback)         { smoothedFeedback.setTargetValue(juce::jlimit(-0.99f, 0.99f, newFeedback)); }

void CombFilter::setAutoGainEnabled(bool enabled)       { autoGainEnabled = enabled; }
void CombFilter::setDampingEnabled(bool enabled)        { dampingEnabled = enabled; }

void CombFilter::setModEnabled(bool enabled)        { modEnabled = enabled; }
void CombFilter::setModType(int typeIndex)         { modulator.setType(static_cast<Modulator::Type>(typeIndex)); }
void CombFilter::setModDepthMs(float depthMs)       { modulator.setDepthMs(depthMs); }
void CombFilter::setModFrequencyHz(float freqHz)   { modulator.setFrequency(freqHz); }

void CombFilter::processBlock(juce::AudioBuffer<float>& buffer)
{
    const int numChannels = buffer.getNumChannels();
    const int numSamples = buffer.getNumSamples();

    if (numChannels == 0 || numSamples == 0 || static_cast<size_t>(numChannels) > delayLines.size())
        return;

    for (int sample = 0; sample < numSamples; ++sample)
    {
        const float curDelayMs     = smoothedDelayMs.getNextValue();
        const float curBlend       = smoothedBlend.getNextValue();
        const float curFeedforward = smoothedFeedforward.getNextValue();
        const float curFeedback    = smoothedFeedback.getNextValue();

        // L2 Energy Normalization Scaling Factor
        const float absFb = std::abs(curFeedback);
        const float normScale = autoGainEnabled ? std::sqrt(1.0f - (absFb * absFb)) : 1.0f;

        const float modOffsetMs = modEnabled ? modulator.processSample() : 0.0f;
        const float totalDelayMs = juce::jlimit(0.1f, maxDelayMs, curDelayMs + modOffsetMs);
        const float curDelaySamples = (totalDelayMs / 1000.0f) * static_cast<float>(currentSampleRate);

        for (int channel = 0; channel < numChannels; ++channel)
        {
            auto* channelData = buffer.getWritePointer(channel);
            const float x = channelData[sample];

            auto& delayLine = delayLines[static_cast<size_t>(channel)];
            delayLine.setDelay(curDelaySamples);

            const float xh_delayed = delayLine.popSample(0);

            // Moorer Lowpass Feedback Filter
            constexpr float dampingCoeff = 0.25f;
            auto& lpState = dampedFeedbackState[static_cast<size_t>(channel)];
            lpState = dampingEnabled ? (((1.0f - dampingCoeff) * xh_delayed) + (dampingCoeff * lpState)) : xh_delayed;

            // Universal Comb Difference Equation
            const float x_scaled = x * normScale;
            const float xh = x_scaled + (curFeedback * lpState);

            delayLine.pushSample(0, xh);

            const float y = (curBlend * xh) + (curFeedforward * xh_delayed);
            channelData[sample] = y;
        }
    }
}