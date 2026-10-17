#include "RingModulator.h"

#include <cmath>

void RingModulator::prepare(double newSampleRate)
{
    sampleRate = newSampleRate;
    reset();
}

void RingModulator::reset()
{
    phase = 0.0;
}

void RingModulator::setCarrierFrequency(float frequencyHz)
{
    carrierFrequencyHz = juce::jlimit(0.1f, 2000.0f, frequencyHz);
}

void RingModulator::setMix(float newMix)
{
    mix = juce::jlimit(0.0f, 1.0f, newMix);
}

void RingModulator::processBlock(juce::AudioBuffer<float>& buffer)
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
