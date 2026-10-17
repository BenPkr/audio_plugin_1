#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <array>

class AudioPluginAudioProcessor;

class AudioScopePlot : public juce::Component,
                       private juce::Timer
{
public:
    explicit AudioScopePlot (AudioPluginAudioProcessor& processor);
    ~AudioScopePlot() override;

    void paint (juce::Graphics& g) override;

private:
    AudioPluginAudioProcessor& audioProcessor;
    static constexpr int numPoints = 512;
    std::array<float, numPoints> samples {};
    int numAvailableSamples { 0 };

    void timerCallback() override;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AudioScopePlot)
};
