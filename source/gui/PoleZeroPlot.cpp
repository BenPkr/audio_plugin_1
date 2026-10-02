#include "PoleZeroPlot.h"
#include "../Parameters.h"

PoleZeroPlot::PoleZeroPlot(juce::AudioProcessorValueTreeState& apvts)
    : valueTreeState(apvts)
{
    delayParam      = valueTreeState.getRawParameterValue(ParameterIDs::delay);
    blendParam      = valueTreeState.getRawParameterValue(ParameterIDs::blend);
    feedforwardParam = valueTreeState.getRawParameterValue(ParameterIDs::feedforward);
    feedbackParam   = valueTreeState.getRawParameterValue(ParameterIDs::feedback);

    startTimer(33);
}

PoleZeroPlot::~PoleZeroPlot()
{
    stopTimer();
}

void PoleZeroPlot::timerCallback()
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

        recalculatePolesZeros();
        repaint();
    }
}

void PoleZeroPlot::recalculatePolesZeros()
{
    constexpr float sampleRate = 44100.0f;
    const int delaySamples = std::max(1, static_cast<int>(std::round((lastDelay / 1000.0f) * sampleRate)));

    // Cap pole/zero display count to keep UI responsive on large delays
    const int displayDelay = std::min(delaySamples, 64);

    poles = CombMath::calculatePoles(displayDelay, lastFB);
    zeros = CombMath::calculateZeros(displayDelay, lastBlend, lastFF);
}

void PoleZeroPlot::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colours::black.withAlpha(0.8f));
    g.setColour(juce::Colours::darkgrey);
    g.drawRect(getLocalBounds(), 1);

    const auto bounds = getLocalBounds().toFloat().reduced(8.0f);
    const float size = std::min(bounds.getWidth(), bounds.getHeight());
    const auto center = bounds.getCentre();
    const float radius = size * 0.4f; // Unit circle radius

    // Draw unit circle (|z| = 1)
    g.setColour(juce::Colours::grey.withAlpha(0.4f));
    g.drawEllipse(center.x - radius, center.y - radius, radius * 2.0f, radius * 2.0f, 1.0f);

    // Draw axes
    g.drawHorizontalLine(static_cast<int>(center.y), center.x - radius - 5.0f, center.x + radius + 5.0f);
    g.drawVerticalLine(static_cast<int>(center.x), center.y - radius - 5.0f, center.y + radius + 5.0f);

    // Helper lambda to map complex number to screen coordinates
    auto toScreen = [center, radius](const CombMath::Complex& z)
    {
        return juce::Point<float>{ center.x + z.real() * radius,
                                   center.y - z.imag() * radius };
    };

    // Draw Zeros ('O')
    g.setColour(juce::Colours::lightgreen);
    constexpr float zeroSize = 6.0f;
    for (const auto& z : zeros)
    {
        const auto p = toScreen(z);
        g.drawEllipse(p.x - zeroSize * 0.5f, p.y - zeroSize * 0.5f, zeroSize, zeroSize, 1.5f);
    }

    // Draw Poles ('X')
    g.setColour(juce::Colours::orangered);
    constexpr float poleSize = 5.0f;
    for (const auto& p : poles)
    {
        const auto pt = toScreen(p);
        g.drawLine(pt.x - poleSize, pt.y - poleSize, pt.x + poleSize, pt.y + poleSize, 1.5f);
        g.drawLine(pt.x - poleSize, pt.y + poleSize, pt.x + poleSize, pt.y - poleSize, 1.5f);
    }
}

void PoleZeroPlot::resized() {}