#include "AudioScopePlot.h"
#include "../PluginProcessor.h"
#include "../Parameters.h"
#include <cmath>

AudioScopePlot::AudioScopePlot (AudioPluginAudioProcessor& processor)
    : audioProcessor (processor)
{
    modeParam = processor.apvts.getRawParameterValue (ParameterIDs::mode);
    startTimerHz (30);
}

AudioScopePlot::~AudioScopePlot()
{
    stopTimer();
}

void AudioScopePlot::timerCallback()
{
    const int capturedSamples = audioProcessor.copyAnalysisSamples (
        inputSamples.data(), outputSamples.data(), gainSamples.data(), capturePoints);
    numAvailableSamples = juce::jmin (capturedSamples, numPoints);
    displayStartIndex = capturedSamples - numAvailableSamples;
    autoTriggered = false;

    if (capturedSamples >= capturePoints)
    {
        float peakInput = 0.0f;
        for (int i = 0; i < capturedSamples; ++i)
            peakInput = juce::jmax (peakInput, std::abs (inputSamples[static_cast<size_t> (i)]));

        const float triggerLevel = juce::jmax (0.0001f, peakInput * 0.02f);
        constexpr int preTriggerSamples = numPoints / 4;
        const int firstTriggerIndex = preTriggerSamples + 1;
        const int lastTriggerIndex = capturedSamples - numPoints + preTriggerSamples;
        const int preferredTriggerIndex = capturedSamples / 2;
        int bestTriggerIndex = -1;
        int bestDistance = capturedSamples;

        for (int i = firstTriggerIndex; i <= lastTriggerIndex; ++i)
        {
            const float previous = inputSamples[static_cast<size_t> (i - 1)];
            const float current = inputSamples[static_cast<size_t> (i)];
            if (previous <= triggerLevel && current > triggerLevel)
            {
                const int distance = std::abs (i - preferredTriggerIndex);
                if (distance < bestDistance)
                {
                    bestTriggerIndex = i;
                    bestDistance = distance;
                }
            }
        }

        if (bestTriggerIndex >= 0)
        {
            displayStartIndex = bestTriggerIndex - preTriggerSamples;
            numAvailableSamples = numPoints;
            autoTriggered = true;
        }
    }

    showGainEnvelope = modeParam != nullptr && static_cast<int> (modeParam->load()) == 2;
    repaint();
}

void AudioScopePlot::paint (juce::Graphics& g)
{
    g.fillAll (juce::Colours::black.withAlpha (0.85f));
    g.setColour (juce::Colours::darkgrey);
    g.drawRect (getLocalBounds(), 1);

    auto bounds = getLocalBounds().toFloat().reduced (4.0f);
    g.setColour (juce::Colours::lightgrey);
    g.setFont (juce::FontOptions (11.0f));
    g.drawText (autoTriggered ? "Input / Output Waveform (Auto Trigger)"
                              : "Input / Output Waveform (Free Run)",
                bounds.removeFromTop (16.0f), juce::Justification::left, false);

    if (numAvailableSamples < 2 || bounds.isEmpty())
        return;

    if (showGainEnvelope)
    {
        const float envelopeHeight = bounds.getHeight() * 0.25f;
        auto envelopeBounds = bounds.removeFromBottom (envelopeHeight);
        bounds.removeFromBottom (3.0f);
        g.setColour (juce::Colours::grey.withAlpha (0.3f));
        g.drawHorizontalLine (static_cast<int> (envelopeBounds.getY()), envelopeBounds.getX(), envelopeBounds.getRight());
        g.setColour (juce::Colours::lightgreen);
        g.setFont (juce::FontOptions (9.0f));
        g.drawText ("Tremolo gain envelope", envelopeBounds.toNearestInt().removeFromTop (11),
                    juce::Justification::left, false);

        juce::Path envelopePath;
        for (int i = 0; i < numAvailableSamples; ++i)
        {
            const float x = envelopeBounds.getX()
                          + static_cast<float> (i) / static_cast<float> (numAvailableSamples - 1)
                          * envelopeBounds.getWidth();
            const float gain = juce::jlimit (
                0.0f, 1.0f, gainSamples[static_cast<size_t> (displayStartIndex + i)]);
            const float y = envelopeBounds.getBottom() - gain * envelopeBounds.getHeight();
            if (i == 0)
                envelopePath.startNewSubPath (x, y);
            else
                envelopePath.lineTo (x, y);
        }
        g.strokePath (envelopePath, juce::PathStrokeType (1.2f));
    }

    const float centreY = bounds.getCentreY();
    const float halfHeight = bounds.getHeight() * 0.45f;
    juce::Path inputPath;
    juce::Path outputPath;

    for (int i = 0; i < numAvailableSamples; ++i)
    {
        const float x = bounds.getX() + static_cast<float> (i) / static_cast<float> (numAvailableSamples - 1)
                                       * bounds.getWidth();
        const auto sampleIndex = static_cast<size_t> (displayStartIndex + i);
        const float inputY = centreY - juce::jlimit (-1.0f, 1.0f, inputSamples[sampleIndex]) * halfHeight;
        const float outputY = centreY - juce::jlimit (-1.0f, 1.0f, outputSamples[sampleIndex]) * halfHeight;
        if (i == 0)
        {
            inputPath.startNewSubPath (x, inputY);
            outputPath.startNewSubPath (x, outputY);
        }
        else
        {
            inputPath.lineTo (x, inputY);
            outputPath.lineTo (x, outputY);
        }
    }

    g.setColour (juce::Colours::orange);
    g.strokePath (inputPath, juce::PathStrokeType (1.2f));
    g.setColour (juce::Colours::cyan);
    g.strokePath (outputPath, juce::PathStrokeType (1.5f));
    g.setFont (juce::FontOptions (10.0f));
    g.setColour (juce::Colours::orange);
    g.drawText ("Input", getLocalBounds().reduced (8).removeFromTop (12), juce::Justification::right, false);
    g.setColour (juce::Colours::cyan);
    g.drawText ("Output", getLocalBounds().reduced (8).removeFromTop (12).withY (20),
                juce::Justification::right, false);
}
