#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include <vector>

class FrequencyResponsePlot : public juce::Component,
                             public juce::Timer
{
public:
    explicit FrequencyResponsePlot(juce::AudioProcessorValueTreeState& apvts);
    ~FrequencyResponsePlot() override;

    void paint(juce::Graphics& g) override;
    void resized() override;
    void timerCallback() override;

private:
    juce::AudioProcessorValueTreeState& valueTreeState;

    // Cache parameter pointers to avoid string lookups in timerCallback
    std::atomic<float>* delayParam      = nullptr;
    std::atomic<float>* blendParam      = nullptr;
    std::atomic<float>* feedforwardParam = nullptr;
    std::atomic<float>* feedbackParam   = nullptr;

    // Cached state to detect parameter changes
    float lastDelay = -1.0f;
    float lastBlend = -1.0f;
    float lastFF    = -1.0f;
    float lastFB    = -1.0f;

    std::vector<float> magnitudeDb;
    static constexpr int numPlotPoints = 300;

    void recalculateResponse();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(FrequencyResponsePlot)
};