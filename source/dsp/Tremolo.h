#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include <cmath>

class Tremolo
{
public:
    enum class Waveform
    {
        Sine = 0,
        Triangle,
        Square
    };

    void prepare(double newSampleRate) { sampleRate = newSampleRate; reset(); }
    void reset() { phase = 0.0; }
    void setFrequencyHz(float frequency) { frequencyHz = juce::jlimit(0.1f, 2000.0f, frequency); }
    void setDepth(float newDepth) { depth = juce::jlimit(0.0f, 1.0f, newDepth); }
    void setWaveform(Waveform newWaveform) { waveform = newWaveform; }

    void processBlock(juce::AudioBuffer<float>& buffer)
    {
        if (sampleRate <= 0.0)
            return;

        const double phaseIncrement = juce::jlimit(0.0, sampleRate * 0.5, static_cast<double>(frequencyHz))
                                    / sampleRate;

        for (int sample = 0; sample < buffer.getNumSamples(); ++sample)
        {
            const float lfo = getUnipolarLfo();
            const float gain = 1.0f - depth * (1.0f - lfo);

            for (int channel = 0; channel < buffer.getNumChannels(); ++channel)
                buffer.getWritePointer(channel)[sample] *= gain;

            phase += phaseIncrement;
            if (phase >= 1.0)
                phase -= 1.0;
        }
    }

private:
    float getUnipolarLfo() const
    {
        switch (waveform)
        {
            case Waveform::Triangle:
                return static_cast<float>(1.0 - std::abs(2.0 * phase - 1.0));
            case Waveform::Square:
                return phase < 0.5 ? 0.0f : 1.0f;
            case Waveform::Sine:
            default:
                return static_cast<float>(0.5 * (std::sin(2.0 * juce::MathConstants<double>::pi * phase) + 1.0));
        }
    }

    double sampleRate { 44100.0 };
    double phase { 0.0 };
    float frequencyHz { 5.0f };
    float depth { 0.8f };
    Waveform waveform { Waveform::Sine };
};
