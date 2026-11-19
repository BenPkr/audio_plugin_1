#include "CompressorTransferCurve.h"
#include "../CompressorParameters.h"
#include "../dsp/CompressorMath.h"
#include <algorithm>

namespace
{
    constexpr float minimumDb = -72.0f;
    constexpr float maximumDb = 6.0f;

    float mapInputToOutput (float inputDb,
                            float thresholdDb,
                            float ratio,
                            float kneeWidthDb,
                            float makeupGainDb)
    {
        return getCompressorTargetLevelDb (inputDb, thresholdDb, ratio, kneeWidthDb)
             + makeupGainDb;
    }
}

CompressorTransferCurve::CompressorTransferCurve (CompressorAudioProcessor& processor)
    : audioProcessor (processor)
{
}

void CompressorTransferCurve::paint (juce::Graphics& graphics)
{
    auto area = getLocalBounds().toFloat().reduced (44.0f, 28.0f);
    if (area.isEmpty())
        return;

    graphics.setColour (juce::Colour (0xff25252d));
    graphics.fillRoundedRectangle (getLocalBounds().toFloat(), 8.0f);

    const auto* thresholdParameter = audioProcessor.apvts.getRawParameterValue (
        CompressorParameterIDs::threshold);
    const auto* ratioParameter = audioProcessor.apvts.getRawParameterValue (
        CompressorParameterIDs::ratio);
    const auto* kneeParameter = audioProcessor.apvts.getRawParameterValue (
        CompressorParameterIDs::knee);
    const auto* makeupParameter = audioProcessor.apvts.getRawParameterValue (
        CompressorParameterIDs::makeupGain);
    if (thresholdParameter == nullptr || ratioParameter == nullptr
        || kneeParameter == nullptr || makeupParameter == nullptr)
        return;

    const float thresholdDb = thresholdParameter->load();
    const float ratio = std::max (1.0f, ratioParameter->load());
    const float kneeWidthDb = std::max (0.0f, kneeParameter->load());
    const float makeupGainDb = makeupParameter->load();
    const float inputLevelDb = audioProcessor.getCurrentInputLevelDb();

    const auto xForDb = [&area] (float db)
    {
        return area.getX() + (db - minimumDb) / (maximumDb - minimumDb) * area.getWidth();
    };
    const auto yForDb = [&area] (float db)
    {
        return area.getBottom() - (db - minimumDb) / (maximumDb - minimumDb) * area.getHeight();
    };

    graphics.setColour (juce::Colours::white.withAlpha (0.55f));
    graphics.setFont (11.0f);
    for (int db = -60; db <= 0; db += 12)
    {
        const float x = xForDb (static_cast<float> (db));
        const float y = yForDb (static_cast<float> (db));
        graphics.drawVerticalLine (static_cast<int> (x), area.getY(), area.getBottom());
        graphics.drawHorizontalLine (static_cast<int> (y), area.getX(), area.getRight());
        graphics.drawText (juce::String (db), static_cast<int> (x - 15.0f),
                           static_cast<int> (area.getBottom() + 4.0f), 30, 16,
                           juce::Justification::centred);
        graphics.drawText (juce::String (db), 2, static_cast<int> (y - 8.0f), 36, 16,
                           juce::Justification::centredRight);
    }

    graphics.setColour (juce::Colours::white.withAlpha (0.35f));
    graphics.drawText ("INPUT (dB)", static_cast<int> (area.getX()),
                       getHeight() - 18, static_cast<int> (area.getWidth()), 16,
                       juce::Justification::centred);
    graphics.drawText ("OUTPUT (dB)", 2, 2, 76, 16, juce::Justification::centredLeft);

    juce::Path unityLine;
    unityLine.startNewSubPath (xForDb (minimumDb), yForDb (minimumDb + makeupGainDb));
    unityLine.lineTo (xForDb (maximumDb), yForDb (maximumDb + makeupGainDb));
    graphics.setColour (juce::Colours::white.withAlpha (0.22f));
    graphics.strokePath (unityLine, juce::PathStrokeType (1.0f));

    juce::Path response;
    for (int point = 0; point <= 160; ++point)
    {
        const float inputDb = minimumDb + (maximumDb - minimumDb)
                                        * static_cast<float> (point) / 160.0f;
        const float outputDb = mapInputToOutput (
            inputDb, thresholdDb, ratio, kneeWidthDb, makeupGainDb);
        const auto position = juce::Point<float> (
            xForDb (inputDb), yForDb (juce::jlimit (minimumDb, maximumDb, outputDb)));
        if (point == 0)
            response.startNewSubPath (position);
        else
            response.lineTo (position);
    }

    graphics.setColour (juce::Colour (0xff65d6ad));
    graphics.strokePath (response, juce::PathStrokeType (2.2f));

    const float markerX = xForDb (juce::jlimit (minimumDb, maximumDb, inputLevelDb));
    const float markerOutputDb = mapInputToOutput (
        inputLevelDb, thresholdDb, ratio, kneeWidthDb, makeupGainDb);
    const float markerY = yForDb (juce::jlimit (minimumDb, maximumDb, markerOutputDb));
    graphics.setColour (juce::Colour (0xffffc857));
    graphics.drawVerticalLine (static_cast<int> (markerX), area.getY(), area.getBottom());
    graphics.fillEllipse (markerX - 4.5f, markerY - 4.5f, 9.0f, 9.0f);

    graphics.setColour (juce::Colours::white);
    graphics.drawText ("Avg input: " + juce::String (juce::roundToInt (inputLevelDb))
                       + " dB   |   Avg GR: "
                       + juce::String (juce::roundToInt (audioProcessor.getCurrentGainReductionDb()))
                       + " dB",
                       getLocalBounds().removeFromTop (22), juce::Justification::centred, true);
}
