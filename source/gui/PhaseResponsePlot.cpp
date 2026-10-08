#include "PhaseResponsePlot.h"
#include "../Parameters.h"
#include <cmath>
#include <complex>

PhaseResponsePlot::PhaseResponsePlot (juce::AudioProcessorValueTreeState& apvts)
    : valueTreeState (apvts)
{
    delayParam        = valueTreeState.getRawParameterValue (ParameterIDs::delay);
    blendParam        = valueTreeState.getRawParameterValue (ParameterIDs::blend);
    feedforwardParam  = valueTreeState.getRawParameterValue (ParameterIDs::feedforward);
    feedbackParam     = valueTreeState.getRawParameterValue (ParameterIDs::feedback);
    autoGainParam     = valueTreeState.getRawParameterValue (ParameterIDs::autoGain);
    dampingParam      = valueTreeState.getRawParameterValue (ParameterIDs::damping);

    modEnableParam    = valueTreeState.getRawParameterValue (ParameterIDs::modEnable);
    modTypeParam      = valueTreeState.getRawParameterValue (ParameterIDs::modType);
    modDepthParam     = valueTreeState.getRawParameterValue (ParameterIDs::modDepth);
    modFrequencyParam = valueTreeState.getRawParameterValue (ParameterIDs::modFrequency);

    startTimer (33); // ~30 FPS refresh rate
}

PhaseResponsePlot::~PhaseResponsePlot()
{
    stopTimer();
}

void PhaseResponsePlot::timerCallback()
{
    constexpr float dt = 0.033f;
    const float modFreq = modFrequencyParam ? modFrequencyParam->load() : 1.0f;
    const float modType = modTypeParam      ? modTypeParam->load()      : 0.0f;

    if (modType < 0.5f) // Sine
    {
        guiPhase += 2.0f * static_cast<float> (M_PI) * modFreq * dt;
        if (guiPhase >= 2.0f * static_cast<float> (M_PI))
            guiPhase -= 2.0f * static_cast<float> (M_PI);
    }
    else // Noise
    {
        const float white = (random.nextFloat() * 2.0f) - 1.0f;
        const float alpha = juce::jlimit (0.01f, 0.5f, 2.0f * static_cast<float> (M_PI) * modFreq * dt);
        guiNoiseState += alpha * (white - guiNoiseState);
    }

    repaint();
}

void PhaseResponsePlot::paint (juce::Graphics& g)
{
    g.fillAll (juce::Colours::black.withAlpha (0.85f));
    g.setColour (juce::Colours::darkgrey);
    g.drawRect (getLocalBounds(), 1);

    auto bounds = getLocalBounds().toFloat().reduced (4.0f);
    if (bounds.isEmpty())
        return;

    // Header Title
    g.setColour (juce::Colours::lightgrey);
    g.setFont (juce::FontOptions (11.0f));
    g.drawText ("Phase Response (Radians)", bounds.removeFromTop (16.0f), juce::Justification::left, false);

    const float delayMs    = delayParam       ? delayParam->load()       : 10.0f;
    const float BL         = blendParam       ? blendParam->load()       : 1.0f;
    const float FF         = feedforwardParam ? feedforwardParam->load() : 0.5f;
    const float FB         = feedbackParam    ? feedbackParam->load()    : 0.0f;
    const bool  autoGainOn = autoGainParam    ? (autoGainParam->load() > 0.5f) : true;
    const bool  dampingOn  = dampingParam     ? (dampingParam->load() > 0.5f)  : true;

    const bool  modOn      = modEnableParam   ? (modEnableParam->load() > 0.5f) : false;
    const float modType    = modTypeParam     ? modTypeParam->load()            : 0.0f;
    const float modDepth   = modDepthParam    ? modDepthParam->load()           : 0.0f;

    float modOffsetMs = 0.0f;
    if (modOn)
    {
        if (modType < 0.5f)
            modOffsetMs = std::sin (guiPhase) * modDepth;
        else
            modOffsetMs = guiNoiseState * modDepth;
    }

    const float totalDelayMs = juce::jlimit (0.1f, 100.0f, delayMs + modOffsetMs);

    const float absFb = std::abs (FB);
    const float normScale = autoGainOn ? std::sqrt (std::max (0.001f, 1.0f - (absFb * absFb))) : 1.0f;

    constexpr double fs = 44100.0;
    const double delaySamples = (totalDelayMs / 1000.0) * fs;

    juce::Path phasePath;
    const int numPixels = static_cast<int> (bounds.getWidth());

    for (int i = 0; i < numPixels; ++i)
    {
        const float normX = static_cast<float> (i) / static_cast<float> (numPixels - 1);
        const double freq = 20.0 * std::pow (1000.0, normX);
        const double omega = 2.0 * M_PI * freq / fs;

        const std::complex<double> z_inv_M = std::polar (1.0, -omega * delaySamples);

        constexpr double dampingCoeff = 0.25;
        std::complex<double> H_feedback = 1.0;

        if (dampingOn)
        {
            const std::complex<double> z_inv = std::polar (1.0, -omega);
            H_feedback = (1.0 - dampingCoeff) / (1.0 - (dampingCoeff * z_inv));
        }

        const std::complex<double> numerator   = static_cast<double> (BL * normScale) + (static_cast<double> (FF) * z_inv_M);
        const std::complex<double> denominator = 1.0 - (static_cast<double> (FB) * H_feedback * z_inv_M);

        const std::complex<double> H = numerator / denominator;
        
        // Phase angle in radians [-PI, +PI]
        const float phase = static_cast<float> (std::arg (H));

        // Map phase [-PI, +PI] to Y height
        const float normY = (phase + static_cast<float> (M_PI)) / (2.0f * static_cast<float> (M_PI));
        const float y = bounds.getBottom() - (normY * bounds.getHeight());
        const float x = bounds.getX() + static_cast<float> (i);

        if (i == 0)
            phasePath.startNewSubPath (x, y);
        else
            phasePath.lineTo (x, y);
    }

    // Zero-Phase Center Grid Line
    g.setColour (juce::Colours::grey.withAlpha (0.25f));
    const float zeroY = bounds.getBottom() - (0.5f * bounds.getHeight());
    g.drawHorizontalLine (static_cast<int> (zeroY), bounds.getX(), bounds.getRight());

    // Draw Phase Response Curve (Light Green)
    g.setColour (juce::Colours::lightgreen);
    g.strokePath (phasePath, juce::PathStrokeType (1.5f));
}

void PhaseResponsePlot::resized() {}