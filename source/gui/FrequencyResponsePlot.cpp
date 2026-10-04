#include "FrequencyResponsePlot.h"
#include "../dsp/CombMath.h"
#include "../Parameters.h"
#include <cmath>

FrequencyResponsePlot::FrequencyResponsePlot(juce::AudioProcessorValueTreeState& apvts)
    : valueTreeState(apvts)
{
    delayParam       = valueTreeState.getRawParameterValue(ParameterIDs::delay);
    blendParam       = valueTreeState.getRawParameterValue(ParameterIDs::blend);
    feedforwardParam = valueTreeState.getRawParameterValue(ParameterIDs::feedforward);
    feedbackParam    = valueTreeState.getRawParameterValue(ParameterIDs::feedback);

    modEnableParam    = valueTreeState.getRawParameterValue(ParameterIDs::modEnable);
    modTypeParam      = valueTreeState.getRawParameterValue(ParameterIDs::modType);
    modDepthParam     = valueTreeState.getRawParameterValue(ParameterIDs::modDepth);
    modFrequencyParam = valueTreeState.getRawParameterValue(ParameterIDs::modFrequency);

    magnitudeDb.resize(numPlotPoints, 0.0f);

    // Start timer at ~30 FPS (33ms)
    startTimer(33);
}

FrequencyResponsePlot::~FrequencyResponsePlot()
{
    stopTimer();
}

void FrequencyResponsePlot::timerCallback()
{
    const float curDelay = delayParam       ? delayParam->load()       : 10.0f;
    const float curBlend = blendParam       ? blendParam->load()       : 1.0f;
    const float curFF    = feedforwardParam ? feedforwardParam->load() : 0.5f;
    const float curFB    = feedbackParam    ? feedbackParam->load()    : 0.0f;

    const bool  curModEnable = modEnableParam    ? (modEnableParam->load() > 0.5f) : false;
    const int   curModType   = modTypeParam      ? static_cast<int>(modTypeParam->load()) : 0;
    const float curModDepth  = modDepthParam     ? modDepthParam->load()     : 0.0f;
    const float curModFreq   = modFrequencyParam ? modFrequencyParam->load() : 1.0f;

    constexpr float threshold = 1e-5f;

    // Advance GUI phase clock by ~33ms
    const float timerIntervalSec = 0.033f;
    guiPhaseTime += timerIntervalSec;

    // Check if parameters changed or if modulation is actively running
    const bool paramsChanged = (std::abs(curDelay - lastDelay) > threshold ||
                               std::abs(curBlend - lastBlend) > threshold ||
                               std::abs(curFF    - lastFF)    > threshold ||
                               std::abs(curFB    - lastFB)    > threshold ||
                               std::abs(curModDepth - lastModDepth) > threshold ||
                               std::abs(curModFreq  - lastModFreq)  > threshold ||
                               curModType != lastModType ||
                               curModEnable != lastModEnable);

    // Force recalculation every frame if modulation is enabled
    if (paramsChanged || (curModEnable && curModDepth > 0.0f))
    {
        lastDelay     = curDelay;
        lastBlend     = curBlend;
        lastFF        = curFF;
        lastFB        = curFB;
        lastModEnable = curModEnable;
        lastModType   = curModType;
        lastModDepth  = curModDepth;
        lastModFreq   = curModFreq;

        recalculateResponse();
        repaint();
    }
}

void FrequencyResponsePlot::recalculateResponse()
{
    constexpr float sampleRate = 44100.0f;

    // Calculate instantaneous modulation offset
    float modOffsetMs = 0.0f;

    if (lastModEnable && lastModDepth > 0.0f)
    {
        if (lastModType == 0) // Sine LFO
        {
            modOffsetMs = std::sin(2.0f * static_cast<float>(M_PI) * lastModFreq * guiPhaseTime) * lastModDepth;
        }
        else // Lowpass Noise approximation
        {
            // Simple multi-sine pseudo-random summation for GUI display smoothness
            const float noiseSim = 0.6f * std::sin(2.0f * static_cast<float>(M_PI) * lastModFreq * guiPhaseTime)
                                 + 0.4f * std::sin(2.0f * static_cast<float>(M_PI) * (lastModFreq * 2.3f) * guiPhaseTime);
            modOffsetMs = noiseSim * lastModDepth;
        }
    }

    const float effectiveDelayMs = juce::jlimit(0.1f, 100.0f, lastDelay + modOffsetMs);
    const float delaySamples = (effectiveDelayMs / 1000.0f) * sampleRate;

    for (int i = 0; i < numPlotPoints; ++i)
    {
        const float normIndex = static_cast<float>(i) / static_cast<float>(numPlotPoints - 1);
        const float freqHz = 20.0f * std::pow(20000.0f / 20.0f, normIndex);
        const float omega = (2.0f * static_cast<float>(M_PI) * freqHz) / sampleRate;

        const auto H = CombMath::evaluateTransferFunction(omega, delaySamples, lastBlend, lastFF, lastFB);
        const float mag = std::abs(H);

        float db = (mag > 1e-5f) ? 20.0f * std::log10(mag) : -100.0f;
        magnitudeDb[static_cast<size_t>(i)] = juce::jlimit(-24.0f, 24.0f, db);
    }
}

void FrequencyResponsePlot::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colours::black.withAlpha(0.8f));
    g.setColour(juce::Colours::darkgrey);
    g.drawRect(getLocalBounds(), 1);

    const auto bounds = getLocalBounds().toFloat().reduced(4.0f);
    if (bounds.isEmpty())
        return;

    // Draw grid lines (-12 dB, 0 dB, +12 dB)
    g.setColour(juce::Colours::grey.withAlpha(0.3f));
    const float y0dB   = juce::jmap(0.0f,   -24.0f, 24.0f, bounds.getBottom(), bounds.getY());
    const float y12dB  = juce::jmap(12.0f,  -24.0f, 24.0f, bounds.getBottom(), bounds.getY());
    const float y_12dB = juce::jmap(-12.0f, -24.0f, 24.0f, bounds.getBottom(), bounds.getY());

    g.drawHorizontalLine(static_cast<int>(y0dB),   bounds.getX(), bounds.getRight());
    g.drawHorizontalLine(static_cast<int>(y12dB),  bounds.getX(), bounds.getRight());
    g.drawHorizontalLine(static_cast<int>(y_12dB), bounds.getX(), bounds.getRight());

    // Construct response path
    juce::Path responsePath;
    for (size_t i = 0; i < magnitudeDb.size(); ++i)
    {
        const float x = bounds.getX() + (static_cast<float>(i) / static_cast<float>(numPlotPoints - 1)) * bounds.getWidth();
        const float y = juce::jmap(magnitudeDb[i], -24.0f, 24.0f, bounds.getBottom(), bounds.getY());

        if (i == 0)
            responsePath.startNewSubPath(x, y);
        else
            responsePath.lineTo(x, y);
    }

    // Change curve color to vibrant magenta/pink when modulation is active to indicate motion
    g.setColour(lastModEnable && lastModDepth > 0.0f ? juce::Colours::magenta : juce::Colours::cyan);
    g.strokePath(responsePath, juce::PathStrokeType(2.0f));
}

void FrequencyResponsePlot::resized() {}