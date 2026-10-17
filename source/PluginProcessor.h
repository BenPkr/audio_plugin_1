#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include "Parameters.h"
#include "dsp/CombFilter.h"
#include "dsp/RingModulator.h"
#include "dsp/Tremolo.h"
#include <array>
#include <cstdint>

//==============================================================================
class AudioPluginAudioProcessor  : public juce::AudioProcessor
{
public:
    //==============================================================================
    AudioPluginAudioProcessor();
    ~AudioPluginAudioProcessor() override;

    //==============================================================================
    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;

    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    int copyScopeSamples (float* destination, int maxSamples) const;

    //==============================================================================
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    //==============================================================================
    const juce::String getName() const override;

    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;

    //==============================================================================
    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int index, const juce::String& newName) override;

    //==============================================================================
    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    // --- AudioProcessorValueTreeState ---
    juce::AudioProcessorValueTreeState apvts;

private:
    // --- DSP Processing Engine ---
    CombFilter combFilter;
    RingModulator ringModulator;
    Tremolo tremolo;

    static constexpr size_t scopeBufferSize = 2048;
    std::array<std::atomic<float>, scopeBufferSize> scopeSamples {};
    std::atomic<std::uint64_t> scopeWriteIndex { 0 };
    juce::AudioBuffer<float> transitionBuffer;
    int currentMode { 0 };
    int transitionFromMode { 0 };
    int transitionSamplesRemaining { 0 };
    int transitionLengthSamples { 1 };

    // Base parameter pointers
    std::atomic<float>* modeParam         = nullptr;
    std::atomic<float>* delayParam       = nullptr;
    std::atomic<float>* blendParam       = nullptr;
    std::atomic<float>* feedforwardParam = nullptr;
    std::atomic<float>* feedbackParam    = nullptr;

    // Tame / Moorer toggle parameter pointers
    std::atomic<float>* autoGainParam    = nullptr;
    std::atomic<float>* dampingParam     = nullptr;

    // Modulation parameter pointers
    std::atomic<float>* modEnableParam    = nullptr;
    std::atomic<float>* modTypeParam      = nullptr;
    std::atomic<float>* modDepthParam     = nullptr;
    std::atomic<float>* modFrequencyParam = nullptr;
    std::atomic<float>* tremWaveformParam = nullptr;

    void processMode (juce::AudioBuffer<float>& buffer, int mode);
    void resetMode (int mode);
    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AudioPluginAudioProcessor)
};