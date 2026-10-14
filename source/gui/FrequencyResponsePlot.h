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

    // Parameter pointers
    std::atomic<float>* delayParam       = nullptr;
    std::atomic<float>* blendParam       = nullptr;
    std::atomic<float>* feedforwardParam = nullptr;
    std::atomic<float>* feedbackParam    = nullptr;

    // Modulation Parameter pointers
    std::atomic<float>* modEnableParam    = nullptr;
    std::atomic<float>* modTypeParam      = nullptr;
    std::atomic<float>* modDepthParam     = nullptr;
    std::atomic<float>* modFrequencyParam = nullptr;

    // Cached values to avoid unnecessary recalculations when static
    float lastDelay = -1.0f;
    float lastBlend = -1.0f;
    float lastFF    = -1.0f;
    float lastFB    = -1.0f;

    float lastModDepth = -1.0f;
    float lastModFreq  = -1.0f;
    int   lastModType  = -1;
    bool  lastModEnable = false;

    // Elapsed time (seconds) for GUI LFO evaluation
    float guiPhaseTime = 0.0f;

    std::vector<float> magnitudeDb;
    static constexpr int numPlotPoints = 300;

    void recalculateResponse();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(FrequencyResponsePlot)
};