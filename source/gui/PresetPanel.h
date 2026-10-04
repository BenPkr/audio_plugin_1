#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include "../Parameters.h"

class PresetPanel : public juce::Component
{
public:
    struct Config
    {
        juce::String title;
        // Ranges (min, max, default)
        std::tuple<double, double, double> delayRange     { 0.1, 100.0, 10.0 };
        std::tuple<double, double, double> depthRange     { 0.0, 30.0, 1.0 };
        std::tuple<double, double, double> freqRange      { 0.05, 20.0, 1.0 };

        // Parameter values & Lock states
        float blendValue { 1.0f };            bool blendLocked { false };
        float ffValue    { 0.5f };            bool ffLocked    { false };
        float fbValue    { 0.0f };            bool fbLocked    { false };
        float delayValue { 10.0f };           bool delayLocked { false };
        
        // Mod Type: 0 = Off, 1 = Sine, 2 = Noise
        int   modTypeMode    { 0 };           bool modTypeLocked   { false };
        float modDepthValue  { 1.0f };        bool modDepthLocked  { false };
        float modFreqValue   { 1.0f };        bool modFreqLocked   { false };
    };

    PresetPanel (juce::AudioProcessorValueTreeState& apvts, Config config);
    ~PresetPanel() override = default;

    void paint (juce::Graphics& g) override;
    void resized() override;
    void applyToAPVTS();

private:
    juce::AudioProcessorValueTreeState& valueTreeState;
    Config panelConfig;

    juce::Slider delaySlider, blendSlider, feedforwardSlider, feedbackSlider;
    juce::Slider modTypeSlider, modDepthSlider, modFrequencySlider;

    juce::Label delayLabel, blendLabel, ffLabel, fbLabel;
    juce::Label modTypeLabel, depthLabel, freqLabel;

    void setupSlider (juce::Slider& slider, juce::Label& label, const juce::String& text, 
                      double min, double max, double initVal, double step, bool locked);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PresetPanel)
};