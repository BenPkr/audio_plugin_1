#include "CombFilter.h"
#include <algorithm>

void CombFilter::prepare(double sampleRate, int samplesPerBlock, int numChannels)
{
    currentSampleRate = sampleRate;

    // Ensure numChannels is at least 1 or 2
    const int channelsToPrepare = std::max(1, numChannels);

    // 1. Prepare parameter smoothers
    constexpr float rampDurationSec = 0.020f;
    smoothedDelayMs.reset(sampleRate, rampDurationSec);
    smoothedBlend.reset(sampleRate, rampDurationSec);
    smoothedFeedforward.reset(sampleRate, rampDurationSec);
    smoothedFeedback.reset(sampleRate, rampDurationSec);

    // Set initial target values explicitly so smoothers aren't stuck at 0
    smoothedDelayMs.setCurrentAndTargetValue(10.0f);
    smoothedBlend.setCurrentAndTargetValue(1.0f);
    smoothedFeedforward.setCurrentAndTargetValue(0.5f);
    smoothedFeedback.setCurrentAndTargetValue(0.0f);

    // 2. Size and prepare delay lines
    const auto maxDelaySamples = static_cast<int>(std::ceil((maxDelayMs / 1000.0f) * currentSampleRate));

    delayLines.resize(static_cast<size_t>(channelsToPrepare));

    // Notice: DelayLine::prepare spec expects 1 channel per DelayLine object!
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

    for (auto& delayLine : delayLines)
        delayLine.reset();
}

void CombFilter::setDelayMs(float newDelayMs)           { smoothedDelayMs.setTargetValue(juce::jlimit(0.1f, maxDelayMs, newDelayMs)); }
void CombFilter::setBlend(float newBlend)               { smoothedBlend.setTargetValue(juce::jlimit(-1.0f, 1.0f, newBlend)); }
void CombFilter::setFeedforward(float newFeedforward)   { smoothedFeedforward.setTargetValue(juce::jlimit(-1.0f, 1.0f, newFeedforward)); }
void CombFilter::setFeedback(float newFeedback)         { smoothedFeedback.setTargetValue(juce::jlimit(-0.99f, 0.99f, newFeedback)); }

void CombFilter::processBlock(juce::AudioBuffer<float>& buffer)
{
    const int numChannels = buffer.getNumChannels();
    const int numSamples = buffer.getNumSamples();

    if (numChannels == 0 || numSamples == 0)
        return;

    // Safety check size
    if (static_cast<size_t>(numChannels) > delayLines.size())
        return;

    for (int sample = 0; sample < numSamples; ++sample)
    {
        // Advance parameter smoothers
        const float curDelayMs     = smoothedDelayMs.getNextValue();
        const float curBlend       = smoothedBlend.getNextValue();
        const float curFeedforward = smoothedFeedforward.getNextValue();
        const float curFeedback    = smoothedFeedback.getNextValue();

        const float curDelaySamples = (curDelayMs / 1000.0f) * static_cast<float>(currentSampleRate);

        for (int channel = 0; channel < numChannels; ++channel)
        {
            auto* channelData = buffer.getWritePointer(channel);
            const float x = channelData[sample];

            auto& delayLine = delayLines[static_cast<size_t>(channel)];
            delayLine.setDelay(curDelaySamples);

            // FIX HERE: Pass 0 as the channel index to the single-channel delayLine object
            const float xh_delayed = delayLine.popSample(0);

            // Universal comb filter difference equations:
            // xh[n] = x[n] + FB * xh[n - M]
            const float xh = x + curFeedback * xh_delayed;

            // Push newest xh sample (also using channel index 0)
            delayLine.pushSample(0, xh);

            // Output: y[n] = BL * xh[n] + FF * xh[n - M]
            const float y = curBlend * xh + curFeedforward * xh_delayed;

            channelData[sample] = y;
        }
    }
}