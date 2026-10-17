#include "ModulationSignalPlot.h"
#include "../Parameters.h"
#include <cmath>

ModulationSignalPlot::ModulationSignalPlot(juce::AudioProcessorValueTreeState& apvts)
    : valueTreeState(apvts)
{
    modEnableParam    = valueTreeState.getRawParameterValue(ParameterIDs::modEnable);
    modTypeParam      = valueTreeState.getRawParameterValue(ParameterIDs::modType);
    modDepthParam     = valueTreeState.getRawParameterValue(ParameterIDs::modDepth);
    modFrequencyParam = valueTreeState.getRawParameterValue(ParameterIDs::modFrequency);

    signalHistory.resize(static_cast<size_t>(numHistoryPoints), 0.0f);

    startTimer(33); // ~30 FPS UI refresh rate
}

ModulationSignalPlot::~ModulationSignalPlot()
{
    stopTimer();
}

void ModulationSignalPlot::timerCallback()
{
    const bool  enabled = modEnableParam    ? (modEnableParam->load() > 0.5f) : false;
    const float typeVal = modTypeParam      ? modTypeParam->load() : 0.0f;
    const float depth   = modDepthParam     ? modDepthParam->load()     : 0.0f;
    const float freq    = modFrequencyParam ? modFrequencyParam->load() : 1.0f;

    // Explicit Threshold: > 0.5f forces Lowpass Noise Mode
    const int type = (typeVal > 0.5f) ? 1 : 0;

    constexpr float dt = 0.033f; // ~33ms timer step
    guiPhaseTime += dt;

    float currentVal = 0.0f;

    if (enabled && depth > 0.0f)
    {
        if (type == 0) // Sine LFO
        {
            const float combRate = juce::jlimit(0.05f, 20.0f, freq);
            currentVal = 0.5f * (std::sin(2.0f * static_cast<float>(M_PI) * combRate * guiPhaseTime) + 1.0f) * depth;
        }
        else // Lowpass Noise Mode
        {
            const float white = random.nextFloat() * 2.0f - 1.0f;
            const float cutoff = juce::jlimit(0.1f, 20.0f, freq);
            const float alpha = juce::jlimit(0.01f, 1.0f, 2.0f * static_cast<float>(M_PI) * cutoff * dt);

            noiseFilterState += alpha * (white - noiseFilterState);
            const float bipolarLfo = juce::jlimit(-1.0f, 1.0f, noiseFilterState * 4.0f);
            currentVal = 0.5f * (bipolarLfo + 1.0f) * depth;
        }
    }

    // Shift buffer to left and append newest frame
    std::rotate(signalHistory.begin(), signalHistory.begin() + 1, signalHistory.end());
    signalHistory.back() = currentVal;

    repaint();
}

void ModulationSignalPlot::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colours::black.withAlpha(0.85f));
    g.setColour(juce::Colours::darkgrey);
    g.drawRect(getLocalBounds(), 1);

    auto bounds = getLocalBounds().toFloat().reduced(4.0f);
    if (bounds.isEmpty())
        return;

    // Title Overlay
    g.setColour(juce::Colours::lightgrey);
    g.setFont(juce::FontOptions(11.0f));
    g.drawText("Delay Modulation (0 to Depth)", bounds.removeFromTop(16.0f), juce::Justification::left, false);

    const float bottomY = bounds.getBottom();
    g.setColour(juce::Colours::grey.withAlpha(0.3f));
    g.drawHorizontalLine(static_cast<int>(bottomY), bounds.getX(), bounds.getRight());

    const bool enabled = modEnableParam ? (modEnableParam->load() > 0.5f) : false;
    const float depth = modDepthParam ? modDepthParam->load() : 0.0f;

    // Draw Waveform Path
    juce::Path wavePath;
    const float halfHeight = (bounds.getHeight() * 0.45f);
    const float maxOffset = depth > 0.0f ? depth : 1.0f;

    for (size_t i = 0; i < signalHistory.size(); ++i)
    {
        const float x = bounds.getX() + (static_cast<float>(i) / static_cast<float>(numHistoryPoints - 1)) * bounds.getWidth();
        const float normalizedOffset = juce::jlimit(0.0f, 1.0f, signalHistory[i] / maxOffset);
        const float y = bottomY - (normalizedOffset * halfHeight * 2.0f);

        if (i == 0)
            wavePath.startNewSubPath(x, y);
        else
            wavePath.lineTo(x, y);
    }

    g.setColour(enabled ? juce::Colours::magenta : juce::Colours::grey);
    g.strokePath(wavePath, juce::PathStrokeType(1.8f));
}

void ModulationSignalPlot::resized() {}