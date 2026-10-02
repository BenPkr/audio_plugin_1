#include "FrequencyResponsePlot.h"
#include "../dsp/CombMath.h"
#include "../Parameters.h"
#include <cmath>

FrequencyResponsePlot::FrequencyResponsePlot(juce::AudioProcessorValueTreeState& apvts)
    : valueTreeState(apvts)
{
    delayParam      = valueTreeState.getRawParameterValue(ParameterIDs::delay);
    blendParam      = valueTreeState.getRawParameterValue(ParameterIDs::blend);
    feedforwardParam = valueTreeState.getRawParameterValue(ParameterIDs::feedforward);
    feedbackParam   = valueTreeState.getRawParameterValue(ParameterIDs::feedback);

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
    const float curDelay = delayParam      ? delayParam->load()      : 10.0f;
    const float curBlend = blendParam      ? blendParam->load()      : 1.0f;
    const float curFF    = feedforwardParam ? feedforwardParam->load() : 0.5f;
    const float curFB    = feedbackParam   ? feedbackParam->load()   : 0.0f;

    if (curDelay != lastDelay || curBlend != lastBlend || curFF != lastFF || curFB != lastFB)
    {
        lastDelay = curDelay;
        lastBlend = curBlend;
        lastFF    = curFF;
        lastFB    = curFB;

        recalculateResponse();
        repaint();
    }
}

void FrequencyResponsePlot::recalculateResponse()
{
    // Convert delay ms to samples assuming a nominal sample rate of 44100 Hz for plot visualization
    constexpr float sampleRate = 44100.0f;
    const float delaySamples = (lastDelay / 1000.0f) * sampleRate;

    for (int i = 0; i < numPlotPoints; ++i)
    {
        // Logarithmically spaced frequency from 20 Hz to 20 kHz
        const float normIndex = static_cast<float>(i) / static_cast<float>(numPlotPoints - 1);
        const float freqHz = 20.0f * std::pow(20000.0f / 20.0f, normIndex);
        const float omega = (2.0f * static_cast<float>(M_PI) * freqHz) / sampleRate;

        const auto H = CombMath::evaluateTransferFunction(omega, delaySamples, lastBlend, lastFF, lastFB);
        const float mag = std::abs(H);

        // Convert magnitude to dB (bounded between -24 dB and +24 dB)
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
    const float y0dB  = juce::jmap(0.0f, -24.0f, 24.0f, bounds.getBottom(), bounds.getY());
    const float y12dB = juce::jmap(12.0f, -24.0f, 24.0f, bounds.getBottom(), bounds.getY());
    const float y_12dB = juce::jmap(-12.0f, -24.0f, 24.0f, bounds.getBottom(), bounds.getY());

    g.drawHorizontalLine(static_cast<int>(y0dB), bounds.getX(), bounds.getRight());
    g.drawHorizontalLine(static_cast<int>(y12dB), bounds.getX(), bounds.getRight());
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

    g.setColour(juce::Colours::cyan);
    g.strokePath(responsePath, juce::PathStrokeType(2.0f));
}

void FrequencyResponsePlot::resized() {}