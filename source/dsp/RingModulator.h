#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include <cmath>

class RingModulator
{
public:
    void prepare(double newSampleRate) { sampleRate = newSampleRate; reset(); }
    void reset() { phase = 0.0; }
    void setCarrierFrequency(float frequencyHz) { carrierFrequencyHz = juce::jlimit(0.1f, 2000.0f, frequencyHz); }
    void setMix(float newMix) { mix = juce::jlimit(0.0f, 1.0f, newMix); }

    void processBlock(juce::AudioBuffer<float>& buffer)
    {
        if (sampleRate <= 0.0)
            return;

        const double phaseIncrement = juce::jlimit(0.0, sampleRate * 0.5, static_cast<double>(carrierFrequencyHz))
                                    / sampleRate * (2.0 * juce::MathConstants<double>::pi);

        for (int sample = 0; sample < buffer.getNumSamples(); ++sample)
        {
            const float carrier = static_cast<float>(std::sin(phase));
            phase += phaseIncrement;
            if (phase >= 2.0 * juce::MathConstants<double>::pi)
                phase -= 2.0 * juce::MathConstants<double>::pi;

            for (int channel = 0; channel < buffer.getNumChannels(); ++channel)
            {
                auto* data = buffer.getWritePointer(channel);
                const float input = data[sample];
                data[sample] = (1.0f - mix) * input + mix * (input * carrier);
            }
        }
    }

private:
    double sampleRate { 44100.0 };
    double phase { 0.0 };
    float carrierFrequencyHz { 440.0f };
    float mix { 1.0f };
};
