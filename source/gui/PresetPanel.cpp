#include "PresetPanel.h"

PresetPanel::PresetPanel (juce::AudioProcessorValueTreeState& apvts, Config config)
    : valueTreeState (apvts), panelConfig (config)
{
    auto [delayMin, delayMax, delayDef] = panelConfig.delayRange;
    auto [depthMin, depthMax, depthDef] = panelConfig.depthRange;
    auto [freqMin, freqMax, freqDef]    = panelConfig.freqRange;

    // 1. Setup Base Comb Filter Sliders
    setupSlider (blendSlider,       blendLabel, "BL",  -1.0,  1.0, panelConfig.blendValue, 0.01, panelConfig.blendLocked);
    setupSlider (feedforwardSlider, ffLabel,    "FF",  -1.0,  1.0, panelConfig.ffValue,    0.01, panelConfig.ffLocked);
    setupSlider (feedbackSlider,    fbLabel,    "FB",  -0.99, 0.99, panelConfig.fbValue,  0.01, panelConfig.fbLocked);
    setupSlider (delaySlider,       delayLabel, "Dly", delayMin, delayMax, panelConfig.delayValue, 0.01, panelConfig.delayLocked);

    // 2. Setup 3-Position Mod Type Knob (0: Off, 1: Sin, 2: Noise)
    setupSlider (modTypeSlider, modTypeLabel, "Mod", 0.0, 2.0, static_cast<double>(panelConfig.modTypeMode), 1.0, panelConfig.modTypeLocked);
    
    modTypeSlider.textFromValueFunction = [](double val)
    {
        const int mode = static_cast<int>(std::round(val));
        if (mode == 1) return juce::String("Sin");
        if (mode == 2) return juce::String("Noise");
        return juce::String("Off");
    };
    modTypeSlider.updateText();

    // 3. Setup Mod Depth & Rate Sliders
    setupSlider (modDepthSlider,     depthLabel, "Dep",  depthMin, depthMax, panelConfig.modDepthValue, 0.01, panelConfig.modDepthLocked);
    setupSlider (modFrequencySlider, freqLabel,  "Rate", freqMin,  freqMax,  panelConfig.modFreqValue,  0.01, panelConfig.modFreqLocked);
}

void PresetPanel::setupSlider (juce::Slider& slider, juce::Label& label, const juce::String& text, 
                               double min, double max, double initVal, double step, bool locked)
{
    slider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    slider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 42, 14);
    slider.setRange (min, max, step);
    slider.setValue (initVal, juce::dontSendNotification);
    slider.setEnabled (!locked);

    slider.onValueChange = [this]()
    {
        if (isEnabled())
            applyToAPVTS();
    };

    addAndMakeVisible (slider);

    label.setText (text, juce::dontSendNotification);
    label.setJustificationType (juce::Justification::centred);
    label.setFont (juce::FontOptions (10.0f));
    addAndMakeVisible (label);
}

void PresetPanel::applyToAPVTS()
{
    const int modMode = static_cast<int>(std::round(modTypeSlider.getValue()));

    // 1. Mod Enable Parameter (Bool: 0.0f or 1.0f)
    if (auto* param = valueTreeState.getParameter(ParameterIDs::modEnable))
        param->setValueNotifyingHost(modMode > 0 ? 1.0f : 0.0f);

    // 2. Mod Type Parameter (Choice 0 = 0.0f [Sine], Choice 1 = 1.0f [Noise])
    if (auto* param = valueTreeState.getParameter(ParameterIDs::modType))
    {
        const float normChoice = (modMode == 2) ? 1.0f : 0.0f;
        param->setValueNotifyingHost(normChoice);
    }

    // 3. Helper for Continuous Float Parameters
    auto setFloatParam = [this](const char* paramID, float rawVal)
    {
        if (auto* param = valueTreeState.getParameter(paramID))
        {
            // Convert value using APVTS parameter text parser
            const float normVal = param->getValueForText(juce::String(rawVal));
            param->setValueNotifyingHost(normVal);
        }
    };

    setFloatParam (ParameterIDs::blend,        static_cast<float>(blendSlider.getValue()));
    setFloatParam (ParameterIDs::feedforward,  static_cast<float>(feedforwardSlider.getValue()));
    setFloatParam (ParameterIDs::feedback,     static_cast<float>(feedbackSlider.getValue()));
    setFloatParam (ParameterIDs::delay,        static_cast<float>(delaySlider.getValue()));
    setFloatParam (ParameterIDs::modDepth,     static_cast<float>(modDepthSlider.getValue()));
    setFloatParam (ParameterIDs::modFrequency, static_cast<float>(modFrequencySlider.getValue()));
}

void PresetPanel::paint (juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat();
    
    // Panel Background
    g.setColour (juce::Colours::black.withAlpha (0.4f));
    g.fillRoundedRectangle (bounds, 6.0f);

    // Active Border
    g.setColour (isEnabled() ? juce::Colours::cyan : juce::Colours::darkgrey);
    g.drawRoundedRectangle (bounds, 6.0f, 1.5f);

    // Header Title
    g.setFont (juce::FontOptions (11.0f));
    g.drawText (panelConfig.title, bounds.removeFromTop (20), juce::Justification::centred, true);

    // --- Draw Modulation Group Box Outline ---
    auto modArea = getLocalBounds().reduced (4);
    modArea.removeFromTop (105);
    auto modBounds = modArea.toFloat();

    g.setColour (isEnabled() ? juce::Colours::cyan.withAlpha (0.3f) : juce::Colours::grey.withAlpha (0.2f));
    g.drawRoundedRectangle (modBounds, 4.0f, 1.0f);

    g.setFont (juce::FontOptions (9.0f));
    g.drawText ("MODULATION", modBounds.removeFromTop (12), juce::Justification::centred, false);
}

void PresetPanel::resized()
{
    auto bounds = getLocalBounds().reduced (2);
    bounds.removeFromTop (18);

    const int colW = bounds.getWidth() / 4;

    auto layoutKnobWithLabel = [](juce::Slider& slider, juce::Label& label, juce::Rectangle<int> area)
    {
        label.setBounds (area.removeFromTop (12));
        slider.setBounds (area);
    };

    // --- Row 1: Base Comb Knobs (BL, FF, FB, Delay) ---
    auto row1 = bounds.removeFromTop (85);
    layoutKnobWithLabel (blendSlider,       blendLabel, row1.removeFromLeft (colW));
    layoutKnobWithLabel (feedforwardSlider, ffLabel,    row1.removeFromLeft (colW));
    layoutKnobWithLabel (feedbackSlider,    fbLabel,    row1.removeFromLeft (colW));
    layoutKnobWithLabel (delaySlider,       delayLabel, row1.removeFromLeft (colW));

    bounds.removeFromTop (14);

    // --- Row 2: Grouped Modulation Knobs (Mod Type, Depth, Rate) ---
    auto modRow = bounds.removeFromTop (80);
    modRow.removeFromTop (10);

    const int modColW = modRow.getWidth() / 3;
    layoutKnobWithLabel (modTypeSlider,      modTypeLabel, modRow.removeFromLeft (modColW));
    layoutKnobWithLabel (modDepthSlider,     depthLabel,   modRow.removeFromLeft (modColW));
    layoutKnobWithLabel (modFrequencySlider, freqLabel,    modRow.removeFromLeft (modColW));
}