#pragma once

#include <juce_dsp/juce_dsp.h>
#include <vector>
#include <cmath>
#include "Modulator.h"

class CombFilter
{
public:
    CombFilter() = default;

    void prepare(double sampleRate, int samplesPerBlock, int numChannels);
    void reset();

    void setDelayMs(float newDelayMs);
    void setBlend(float newBlend);
    void setFeedforward(float newFeedforward);
    void setFeedback(float newFeedback);

    void setAutoGainEnabled(bool enabled);
    void setDampingEnabled(bool enabled);

    void setModEnabled(bool enabled);
    void setModType(int typeIndex);
    void setModDepthMs(float depthMs);
    void setModFrequencyHz(float freqHz);

    void processBlock(juce::AudioBuffer<float>& buffer);

private:
    double currentSampleRate { 44100.0 };
    float maxDelayMs { 100.0f };

    bool modEnabled { false };
    bool autoGainEnabled { true };
    bool dampingEnabled { true };

    juce::LinearSmoothedValue<float> smoothedDelayMs;
    juce::LinearSmoothedValue<float> smoothedBlend;
    juce::LinearSmoothedValue<float> smoothedFeedforward;
    juce::LinearSmoothedValue<float> smoothedFeedback;

    Modulator modulator;
    std::vector<juce::dsp::DelayLine<float, juce::dsp::DelayLineInterpolationTypes::Linear>> delayLines;
    std::vector<float> dampedFeedbackState;
};