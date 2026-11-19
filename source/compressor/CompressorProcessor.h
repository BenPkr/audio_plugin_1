#pragma once

#include "CompressorParameters.h"
#include "dsp/Compressor.h"
#include <juce_audio_processors/juce_audio_processors.h>

class CompressorAudioProcessor final : public juce::AudioProcessor
{
public:
    CompressorAudioProcessor();
    ~CompressorAudioProcessor() override = default;

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
    Compressor compressor;
    std::atomic<float>* thresholdParam { nullptr };
    std::atomic<float>* ratioParam { nullptr };
    std::atomic<float>* kneeParam { nullptr };
    std::atomic<float>* attackParam { nullptr };
    std::atomic<float>* releaseParam { nullptr };
    std::atomic<float>* makeupGainParam { nullptr };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (CompressorAudioProcessor)
};
