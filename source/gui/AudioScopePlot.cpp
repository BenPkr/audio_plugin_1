#include "AudioScopePlot.h"
#include "../PluginProcessor.h"
#include "../Parameters.h"

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
    numAvailableSamples = audioProcessor.copyAnalysisSamples (
        inputSamples.data(), outputSamples.data(), gainSamples.data(), numPoints);
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
    g.drawText ("Input / Output Waveform", bounds.removeFromTop (16.0f), juce::Justification::left, false);

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
            const float gain = juce::jlimit (0.0f, 1.0f, gainSamples[static_cast<size_t> (i)]);
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
        const float inputY = centreY - juce::jlimit (-1.0f, 1.0f, inputSamples[static_cast<size_t> (i)]) * halfHeight;
        const float outputY = centreY - juce::jlimit (-1.0f, 1.0f, outputSamples[static_cast<size_t> (i)]) * halfHeight;
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
