#pragma once

#include "ExpanderParameters.h"
#include "dsp/Expander.h"
#include <juce_audio_processors/juce_audio_processors.h>

class ExpanderAudioProcessor final : public juce::AudioProcessor
{
public:
    ExpanderAudioProcessor();
    ~ExpanderAudioProcessor() override = default;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;
    const juce::String getName() const override;
    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;
    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int index, const juce::String& newName) override;
    void getStateInformation (juce::MemoryBlock& destination) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    float getCurrentInputLevelDb() const noexcept;
    float getCurrentGainReductionDb() const noexcept;

    juce::AudioProcessorValueTreeState apvts;

private:
    Expander expander;
    std::atomic<float>* thresholdParam { nullptr };
    std::atomic<float>* ratioParam { nullptr };
    std::atomic<float>* attackParam { nullptr };
    std::atomic<float>* releaseParam { nullptr };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ExpanderAudioProcessor)
};
