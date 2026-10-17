#include "AudioScopePlot.h"
#include "../PluginProcessor.h"

AudioScopePlot::AudioScopePlot (AudioPluginAudioProcessor& processor)
    : audioProcessor (processor)
{
    startTimerHz (30);
}

AudioScopePlot::~AudioScopePlot()
{
    stopTimer();
}

void AudioScopePlot::timerCallback()
{
    numAvailableSamples = audioProcessor.copyScopeSamples (samples.data(), numPoints);
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
    g.drawText ("Output Waveform", bounds.removeFromTop (16.0f), juce::Justification::left, false);

    if (numAvailableSamples < 2 || bounds.isEmpty())
        return;

    const float centreY = bounds.getCentreY();
    const float halfHeight = bounds.getHeight() * 0.45f;
    juce::Path path;

    for (int i = 0; i < numAvailableSamples; ++i)
    {
        const float x = bounds.getX() + static_cast<float> (i) / static_cast<float> (numAvailableSamples - 1)
                                       * bounds.getWidth();
        const float y = centreY - juce::jlimit (-1.0f, 1.0f, samples[static_cast<size_t> (i)]) * halfHeight;
        if (i == 0)
            path.startNewSubPath (x, y);
        else
            path.lineTo (x, y);
    }

    g.setColour (juce::Colours::cyan);
    g.strokePath (path, juce::PathStrokeType (1.5f));
}
