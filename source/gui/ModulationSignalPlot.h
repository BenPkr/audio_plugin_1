#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include <vector>

class ModulationSignalPlot : public juce::Component,
                            public juce::Timer
{
public:
    explicit ModulationSignalPlot(juce::AudioProcessorValueTreeState& apvts);
    ~ModulationSignalPlot() override;

    void paint(juce::Graphics& g) override;
    void resized() override;
    void timerCallback() override;

private:
    juce::AudioProcessorValueTreeState& valueTreeState;

    // Parameter pointers
    std::atomic<float>* modEnableParam    = nullptr;
    std::atomic<float>* modTypeParam      = nullptr;
    std::atomic<float>* modDepthParam     = nullptr;
    std::atomic<float>* modFrequencyParam = nullptr;

    float guiPhaseTime { 0.0f };
    float noiseFilterState { 0.0f };

    static constexpr int numHistoryPoints = 200;
    std::vector<float> signalHistory;

    juce::Random random;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ModulationSignalPlot)
};