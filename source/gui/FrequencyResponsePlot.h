#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>
#include <array>

class AudioPluginAudioProcessor;

class FrequencyResponsePlot : public juce::Component,
                              private juce::Timer
{
public:
    FrequencyResponsePlot (AudioPluginAudioProcessor& processor,
                           juce::AudioProcessorValueTreeState& apvts);
    ~FrequencyResponsePlot() override;

    void paint (juce::Graphics& g) override;
    void resized() override;
    void setProcessingMode (int mode) { processingMode = mode; repaint(); }

private:
    static constexpr int fftOrder = 12;
    static constexpr int fftSize = 1 << fftOrder;
    static constexpr int spectrumSize = fftSize / 2 + 1;

    AudioPluginAudioProcessor& audioProcessor;
    juce::AudioProcessorValueTreeState& valueTreeState;
    juce::dsp::FFT fft { fftOrder };
    std::array<float, fftSize * 2> fftData {};
    std::array<float, fftSize> analysisInput {};
    std::array<float, fftSize> analysisOutput {};
    std::array<float, spectrumSize> inputSpectrum {};
    std::array<float, spectrumSize> outputSpectrum {};
    bool analysisReady { false };
    float inputPeakFrequencyHz { 0.0f };
    float inputPeakAmplitude { 0.0f };
    float inputRms { 0.0f };

    std::atomic<float>* delayParam        { nullptr };
    std::atomic<float>* blendParam        { nullptr };
    std::atomic<float>* feedforwardParam  { nullptr };
    std::atomic<float>* feedbackParam     { nullptr };
    std::atomic<float>* autoGainParam     { nullptr };
    std::atomic<float>* dampingParam      { nullptr };

    // Modulation Parameter Pointers
    std::atomic<float>* modEnableParam    { nullptr };
    std::atomic<float>* modTypeParam      { nullptr };
    std::atomic<float>* modDepthParam     { nullptr };
    std::atomic<float>* modFrequencyParam { nullptr };
    std::atomic<float>* phaserStagesParam { nullptr };
    std::atomic<float>* phaserRateParam { nullptr };
    std::atomic<float>* phaserDepthParam { nullptr };
    std::atomic<float>* phaserMixParam { nullptr };

    // GUI LFO State Tracking
    float guiPhase { 0.0f };
    float phaserPhase { 0.0f };
    float guiNoiseState { 0.0f };
    int processingMode { 0 };
    juce::Random random;

    void timerCallback() override;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (FrequencyResponsePlot)
};