#pragma once

#include <juce_audio_basics/juce_audio_basics.h>

class RingModulator
{
public:
    void prepare(double newSampleRate);
    void reset();
    void setCarrierFrequency(float frequencyHz);
    void setMix(float newMix);
    void processBlock(juce::AudioBuffer<float>& buffer);

private:
    double sampleRate { 44100.0 };
    double phase { 0.0 };
    float carrierFrequencyHz { 440.0f };
    float mix { 1.0f };
};
