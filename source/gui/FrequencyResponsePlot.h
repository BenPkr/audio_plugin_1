#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_audio_processors/juce_audio_processors.h>

class FrequencyResponsePlot : public juce::Component,
                              private juce::Timer
{
public:
    explicit FrequencyResponsePlot (juce::AudioProcessorValueTreeState& apvts);
    ~FrequencyResponsePlot() override;

    void paint (juce::Graphics& g) override;
    void resized() override;

private:
    juce::AudioProcessorValueTreeState& valueTreeState;

    std::atomic<float>* delayParam        { nullptr };
    std::atomic<float>* blendParam        { nullptr };
    std::atomic<float>* feedforwardParam  { nullptr };
    std::atomic<float>* feedbackParam     { nullptr };
    std::atomic<float>* autoGainParam     { nullptr };
    std::atomic<float>* dampingParam      { nullptr };

    // Modulation Parameter Pointers
    std::atomic<float>* modEnableParam    { nullptr };
    std::atomic<float>* modTypeParam      { nullptr };
    std::atomic<float>* modDepthParam     { nullptr };
    std::atomic<float>* modFrequencyParam { nullptr };

    // GUI LFO State Tracking
    float guiPhase { 0.0f };
    float guiNoiseState { 0.0f };
    juce::Random random;

    void timerCallback() override;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (FrequencyResponsePlot)
};