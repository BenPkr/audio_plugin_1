#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "../ExpanderProcessor.h"

class ExpanderTransferCurve final : public juce::Component
{
public:
    explicit ExpanderTransferCurve (ExpanderAudioProcessor& processor);

    void paint (juce::Graphics& graphics) override;

private:
    ExpanderAudioProcessor& audioProcessor;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ExpanderTransferCurve)
};
