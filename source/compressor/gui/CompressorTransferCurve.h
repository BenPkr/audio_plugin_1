#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "../CompressorProcessor.h"

class CompressorTransferCurve final : public juce::Component
{
public:
    explicit CompressorTransferCurve (CompressorAudioProcessor& processor);

    void paint (juce::Graphics& graphics) override;

private:
    CompressorAudioProcessor& audioProcessor;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (CompressorTransferCurve)
};
