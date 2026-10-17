#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include <array>
#include <vector>

class Phaser
{
public:
    void prepare(double newSampleRate, int numChannels);
    void reset();
    void setStages(int newStages);
    void setRateHz(float newRateHz);
    void setDepth(float newDepth);
    void setMix(float newMix);
    void processBlock(juce::AudioBuffer<float>& buffer);

private:
    struct AllPassState
    {
        float xh1 { 0.0f };
        float xh2 { 0.0f };
    };

    static constexpr int maximumStages = 8;
    double sampleRate { 44100.0 };
    double phase { 0.0 };
    int stages { 4 };
    juce::SmoothedValue<float> rateHz;
    juce::SmoothedValue<float> depth;
    juce::SmoothedValue<float> mix;
    std::vector<std::array<AllPassState, maximumStages>> channelStates;
};
