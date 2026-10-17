#pragma once

#include <juce_audio_basics/juce_audio_basics.h>

class Tremolo
{
public:
    enum class Waveform
    {
        Sine = 0,
        Triangle,
        Square
    };

    void prepare(double newSampleRate);
    void reset();
    void setFrequencyHz(float frequency);
    void setDepth(float newDepth);
    void setWaveform(Waveform newWaveform);
    void processBlock(juce::AudioBuffer<float>& buffer, float* gainOutput = nullptr);

private:
    float getUnipolarLfo() const;

    double sampleRate { 44100.0 };
    double phase { 0.0 };
    float frequencyHz { 5.0f };
    float depth { 0.8f };
    Waveform waveform { Waveform::Sine };
};
