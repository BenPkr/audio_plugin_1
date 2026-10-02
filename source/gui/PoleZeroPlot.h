#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include "../dsp/CombMath.h"
#include <vector>

class PoleZeroPlot : public juce::Component,
                     public juce::Timer
{
public:
    explicit PoleZeroPlot(juce::AudioProcessorValueTreeState& apvts);
    ~PoleZeroPlot() override;

    void paint(juce::Graphics& g) override;
    void resized() override;
    void timerCallback() override;

private:
    juce::AudioProcessorValueTreeState& valueTreeState;

    std::atomic<float>* delayParam      = nullptr;
    std::atomic<float>* blendParam      = nullptr;
    std::atomic<float>* feedforwardParam = nullptr;
    std::atomic<float>* feedbackParam   = nullptr;

    float lastDelay = -1.0f;
    float lastBlend = -1.0f;
    float lastFF    = -1.0f;
    float lastFB    = -1.0f;

    std::vector<CombMath::Complex> poles;
    std::vector<CombMath::Complex> zeros;

    void recalculatePolesZeros();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PoleZeroPlot)
};