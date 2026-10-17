#include "FrequencyResponsePlot.h"
#include "../Parameters.h"
#include "../PluginProcessor.h"
#include <algorithm>
#include <cmath>
#include <complex>

FrequencyResponsePlot::FrequencyResponsePlot (AudioPluginAudioProcessor& processor,
                                              juce::AudioProcessorValueTreeState& apvts)
    : audioProcessor (processor),
      valueTreeState (apvts)
{
    delayParam        = valueTreeState.getRawParameterValue (ParameterIDs::delay);
    blendParam        = valueTreeState.getRawParameterValue (ParameterIDs::blend);
    feedforwardParam  = valueTreeState.getRawParameterValue (ParameterIDs::feedforward);
    feedbackParam     = valueTreeState.getRawParameterValue (ParameterIDs::feedback);
    autoGainParam     = valueTreeState.getRawParameterValue (ParameterIDs::autoGain);
    dampingParam      = valueTreeState.getRawParameterValue (ParameterIDs::damping);

    modEnableParam    = valueTreeState.getRawParameterValue (ParameterIDs::modEnable);
    modTypeParam      = valueTreeState.getRawParameterValue (ParameterIDs::modType);
    modDepthParam     = valueTreeState.getRawParameterValue (ParameterIDs::modDepth);
    modFrequencyParam = valueTreeState.getRawParameterValue (ParameterIDs::modFrequency);
    phaserStagesParam = valueTreeState.getRawParameterValue (ParameterIDs::phaserStages);
    phaserRateParam   = valueTreeState.getRawParameterValue (ParameterIDs::phaserRate);
    phaserDepthParam  = valueTreeState.getRawParameterValue (ParameterIDs::phaserDepth);
    phaserMixParam    = valueTreeState.getRawParameterValue (ParameterIDs::phaserMix);

    startTimer (33); // ~30 FPS refresh rate
}

FrequencyResponsePlot::~FrequencyResponsePlot()
{
    stopTimer();
}

void FrequencyResponsePlot::timerCallback()
{
    // Advance LFO state for GUI visualization frame (~33ms steps)
    constexpr float dt = 0.033f;
    if (processingMode == 3)
    {
        const float rate = phaserRateParam ? juce::jlimit (0.05f, 8.0f, phaserRateParam->load()) : 0.5f;
        phaserPhase += 2.0f * juce::MathConstants<float>::pi * rate * dt;
        if (phaserPhase >= 2.0f * juce::MathConstants<float>::pi)
            phaserPhase = std::fmod (phaserPhase, 2.0f * juce::MathConstants<float>::pi);
    }

    const float modFreq  = modFrequencyParam ? juce::jlimit (0.05f, 20.0f, modFrequencyParam->load()) : 1.0f;
    const float modType  = modTypeParam      ? modTypeParam->load()      : 0.0f;

    if (modType < 0.5f) // Sine
    {
        guiPhase += 2.0f * static_cast<float> (M_PI) * modFreq * dt;
        if (guiPhase >= 2.0f * static_cast<float> (M_PI))
            guiPhase -= 2.0f * static_cast<float> (M_PI);
    }
    else // Lowpass Noise
    {
        const float white = (random.nextFloat() * 2.0f) - 1.0f;
        const float alpha = juce::jlimit (0.01f, 0.5f, 2.0f * static_cast<float> (M_PI) * modFreq * dt);
        guiNoiseState += alpha * (white - guiNoiseState);
    }

    if (processingMode == 1 || processingMode == 2)
    {
        const int sampleCount = audioProcessor.copyAnalysisSamples (
            analysisInput.data(), analysisOutput.data(), nullptr, fftSize);
        analysisReady = sampleCount == fftSize && audioProcessor.getSampleRate() > 0.0;

        if (analysisReady)
        {
            double inputEnergy = 0.0;
            int peakBin = 1;
            float peakMagnitude = 0.0f;
            const int minPeakBin = juce::jmax (1, static_cast<int> (std::ceil (
                40.0 * static_cast<double> (fftSize) / audioProcessor.getSampleRate())));

            for (int i = 0; i < fftSize; ++i)
            {
                const float input = analysisInput[static_cast<size_t> (i)];
                inputEnergy += static_cast<double> (input) * input;
                const float window = 0.5f - 0.5f * std::cos (
                    2.0f * juce::MathConstants<float>::pi * static_cast<float> (i)
                    / static_cast<float> (fftSize - 1));
                fftData[static_cast<size_t> (i)] = input * window;
                fftData[static_cast<size_t> (fftSize + i)] = 0.0f;
            }

            inputRms = static_cast<float> (std::sqrt (inputEnergy / static_cast<double> (fftSize)));
            fft.performFrequencyOnlyForwardTransform (fftData.data());
            for (int bin = 0; bin < spectrumSize; ++bin)
            {
                const float magnitude = fftData[static_cast<size_t> (bin)] * 4.0f / static_cast<float> (fftSize);
                inputSpectrum[static_cast<size_t> (bin)] = magnitude;
                if (bin >= minPeakBin && magnitude > peakMagnitude)
                {
                    peakMagnitude = magnitude;
                    peakBin = bin;
                }
            }
            inputPeakAmplitude = peakMagnitude;
            inputPeakFrequencyHz = static_cast<float> (peakBin) * static_cast<float> (audioProcessor.getSampleRate())
                                 / static_cast<float> (fftSize);

            for (int i = 0; i < fftSize; ++i)
            {
                const float output = analysisOutput[static_cast<size_t> (i)];
                const float window = 0.5f - 0.5f * std::cos (
                    2.0f * juce::MathConstants<float>::pi * static_cast<float> (i)
                    / static_cast<float> (fftSize - 1));
                fftData[static_cast<size_t> (i)] = output * window;
                fftData[static_cast<size_t> (fftSize + i)] = 0.0f;
            }

            fft.performFrequencyOnlyForwardTransform (fftData.data());
            for (int bin = 0; bin < spectrumSize; ++bin)
                outputSpectrum[static_cast<size_t> (bin)] =
                    fftData[static_cast<size_t> (bin)] * 4.0f / static_cast<float> (fftSize);
        }
    }

    repaint();
}

void FrequencyResponsePlot::paint (juce::Graphics& g)
{
    g.fillAll (juce::Colours::black.withAlpha (0.85f));
    g.setColour (juce::Colours::darkgrey);
    g.drawRect (getLocalBounds(), 1);

    auto bounds = getLocalBounds().toFloat().reduced (4.0f);
    if (bounds.isEmpty())
        return;

    // Header Title
    g.setColour (juce::Colours::lightgrey);
    g.setFont (juce::FontOptions (11.0f));
    g.drawText (processingMode == 0 ? "Frequency Response (Magnitude)"
                 : processingMode == 3 ? "Phaser Response + Sweep Range"
                 : "Live Input / Output Spectrum",
                bounds.removeFromTop (16.0f), juce::Justification::left, false);

    if (processingMode != 0)
    {
        g.setColour (juce::Colours::lightgrey);
        g.drawText (processingMode == 1 ? "Live Spectrum - Ring Modulator"
                     : processingMode == 2 ? "Live Spectrum - Tremolo"
                     : "Moving all-pass notches",
                    bounds.removeFromTop (14.0f), juce::Justification::left, false);

        if (processingMode == 3)
        {
            const int stageCount = juce::jlimit (
                2, 8, phaserStagesParam ? static_cast<int> (phaserStagesParam->load()) : 4);
            const float sweepDepth = phaserDepthParam
                                   ? juce::jlimit (0.0f, 1.0f, phaserDepthParam->load() * 0.01f)
                                   : 0.75f;
            const float wetMix = phaserMixParam
                               ? juce::jlimit (0.0f, 1.0f, phaserMixParam->load() * 0.01f)
                               : 0.5f;
            const double sampleRate = audioProcessor.getSampleRate() > 0.0
                                    ? audioProcessor.getSampleRate() : 44100.0;
            const double maximumFrequency = std::min (6000.0, sampleRate * 0.4);
            const double minimumFrequency = std::min (150.0, maximumFrequency * 0.25);
            const double stageLogRange = std::log (maximumFrequency / minimumFrequency);
            const float currentLfo = 0.5f * (std::sin (phaserPhase) + 1.0f);
            const double currentOctaveOffset = (static_cast<double> (currentLfo) - 0.5)
                                             * 2.0 * static_cast<double> (sweepDepth);

            auto graphArea = bounds.reduced (0.0f, 2.0f);
            const auto frequencyLabels = graphArea.removeFromBottom (12.0f);
            const auto responseArea = graphArea;
            constexpr float minDb = -48.0f;
            constexpr float maxDb = 3.0f;
            constexpr int sweepSamples = 24;

            auto responseAt = [=] (double frequency, double octaveOffset)
            {
                const double omega = 2.0 * juce::MathConstants<double>::pi
                                   * frequency / sampleRate;
                const std::complex<double> z1 = std::polar (1.0, -omega);
                const std::complex<double> z2 = z1 * z1;
                std::complex<double> allPassResponse { 1.0, 0.0 };
                const double frequencyScale = std::exp2 (octaveOffset);

                for (int stage = 0; stage < stageCount; ++stage)
                {
                    const double stagePosition = static_cast<double> (stage)
                                               / static_cast<double> (stageCount - 1);
                    const double centerFrequency = std::min (
                        minimumFrequency * std::exp (stagePosition * stageLogRange)
                            * frequencyScale,
                        sampleRate * 0.45);
                    const double centerOmega = 2.0 * juce::MathConstants<double>::pi
                                             * centerFrequency / sampleRate;
                    const double alpha = std::sin (centerOmega) / (2.0 * 0.7071067811865476);
                    const double a1 = -2.0 * std::cos (centerOmega) / (1.0 + alpha);
                    const double a2 = (1.0 - alpha) / (1.0 + alpha);
                    allPassResponse *= (a2 + a1 * z1 + z2) / (1.0 + a1 * z1 + a2 * z2);
                }

                return std::abs ((1.0 - static_cast<double> (wetMix))
                               + static_cast<double> (wetMix) * allPassResponse);
            };

            const float maximumDisplayFrequency = juce::jmax (
                20.0f, static_cast<float> (std::min (20000.0, sampleRate * 0.5)));
            const float logFrequencyRange = std::log (maximumDisplayFrequency / 20.0f);
            const int numPixels = static_cast<int> (responseArea.getWidth());
            juce::Path currentResponsePath;
            juce::Path sweepUpperPath;
            juce::Path sweepLowerPath;

            for (int pixel = 0; pixel < numPixels; ++pixel)
            {
                const float proportion = static_cast<float> (pixel)
                                       / static_cast<float> (juce::jmax (1, numPixels - 1));
                const double frequency = 20.0 * std::exp (
                    static_cast<double> (proportion * logFrequencyRange));
                double lowestMagnitude = 1.0;
                double highestMagnitude = 0.0;
                for (int sweep = 0; sweep < sweepSamples; ++sweep)
                {
                    const double lfo = 0.5 * (std::sin (
                        2.0 * juce::MathConstants<double>::pi * static_cast<double> (sweep)
                        / static_cast<double> (sweepSamples - 1)) + 1.0);
                    const double octaveOffset = (lfo - 0.5) * 2.0 * static_cast<double> (sweepDepth);
                    const double magnitude = responseAt (frequency, octaveOffset);
                    lowestMagnitude = std::min (lowestMagnitude, magnitude);
                    highestMagnitude = std::max (highestMagnitude, magnitude);
                }

                const double currentMagnitude = responseAt (frequency, currentOctaveOffset);
                const auto toY = [&responseArea] (double magnitude)
                {
                    constexpr double low = -48.0;
                    constexpr double high = 3.0;
                    const double db = 20.0 * std::log10 (std::max (magnitude, 1.0e-5));
                    const double normalized = juce::jlimit (0.0, 1.0, (db - low) / (high - low));
                    return responseArea.getBottom() - static_cast<float> (normalized)
                         * responseArea.getHeight();
                };
                const float currentY = toY (currentMagnitude);
                const float upperY = toY (highestMagnitude);
                const float lowerY = toY (lowestMagnitude);
                const float x = responseArea.getX() + static_cast<float> (pixel);
                if (pixel == 0)
                {
                    currentResponsePath.startNewSubPath (x, currentY);
                    sweepUpperPath.startNewSubPath (x, upperY);
                    sweepLowerPath.startNewSubPath (x, lowerY);
                }
                else
                {
                    currentResponsePath.lineTo (x, currentY);
                    sweepUpperPath.lineTo (x, upperY);
                    sweepLowerPath.lineTo (x, lowerY);
                }
            }

            g.setColour (juce::Colours::grey.withAlpha (0.3f));
            const float zeroDbY = responseArea.getBottom()
                                - ((0.0f - minDb) / (maxDb - minDb)) * responseArea.getHeight();
            g.drawHorizontalLine (static_cast<int> (zeroDbY), responseArea.getX(), responseArea.getRight());
            g.setColour (juce::Colours::magenta.withAlpha (0.22f));
            g.strokePath (sweepUpperPath, juce::PathStrokeType (1.0f));
            g.strokePath (sweepLowerPath, juce::PathStrokeType (1.0f));
            g.setColour (juce::Colours::cyan);
            g.strokePath (currentResponsePath, juce::PathStrokeType (1.8f));

            constexpr std::array<float, 5> frequencies { 20.0f, 100.0f, 500.0f, 2000.0f, 8000.0f };
            for (const float frequency : frequencies)
            {
                if (frequency > maximumDisplayFrequency)
                    continue;
                const float x = responseArea.getX()
                    + std::log (frequency / 20.0f) / logFrequencyRange * responseArea.getWidth();
                g.setColour (juce::Colours::grey.withAlpha (0.35f));
                g.drawVerticalLine (static_cast<int> (x), responseArea.getY(), responseArea.getBottom());
                g.setColour (juce::Colours::lightgrey.withAlpha (0.8f));
                const juce::String label = frequency >= 1000.0f
                                         ? juce::String (frequency / 1000.0f, 0) + "k"
                                         : juce::String (static_cast<int> (frequency));
                g.drawText (label, juce::Rectangle<int> (static_cast<int> (x - 16.0f),
                                                          static_cast<int> (frequencyLabels.getY()),
                                                          32, 12),
                            juce::Justification::centred, false);
            }

            g.setFont (juce::FontOptions (9.0f));
            g.setColour (juce::Colours::cyan);
            g.drawText ("Current", bounds.toNearestInt().removeFromLeft (52),
                        juce::Justification::left, false);
            g.setColour (juce::Colours::magenta);
            g.drawText ("Sweep", juce::Rectangle<int> (
                            static_cast<int> (bounds.getX() + 54.0f),
                            static_cast<int> (bounds.getY()), 42, 12),
                        juce::Justification::left, false);
            return;
        }

        if (!analysisReady)
        {
            g.setColour (juce::Colours::grey);
            g.drawText ("Waiting for audio...", bounds, juce::Justification::centred, true);
            return;
        }

        auto spectrumArea = bounds.reduced (0.0f, 2.0f);
        const auto annotationBounds = spectrumArea.removeFromTop (12.0f);
        const auto frequencyLabelBounds = spectrumArea.removeFromBottom (12.0f);
        const auto spectrumBounds = spectrumArea;
        constexpr float minDb = -90.0f;
        constexpr float maxDb = 0.0f;
        const double sampleRate = audioProcessor.getSampleRate();
        const float maxFrequency = juce::jmax (
            20.0f, static_cast<float> (std::min (20000.0, sampleRate * 0.5)));
        const float logFrequencyRange = std::log (maxFrequency / 20.0f);
        juce::Path inputPath;
        juce::Path outputPath;
        const int numPixels = static_cast<int> (spectrumBounds.getWidth());

        for (int pixel = 0; pixel < numPixels; ++pixel)
        {
            const float proportion = static_cast<float> (pixel) / static_cast<float> (juce::jmax (1, numPixels - 1));
            const float frequency = 20.0f * std::exp (proportion * logFrequencyRange);
            const int bin = juce::jlimit (1, spectrumSize - 1, static_cast<int> (
                std::round (frequency * static_cast<float> (fftSize) / static_cast<float> (sampleRate))));
            const float inputDb = 20.0f * std::log10 (juce::jmax (1.0e-6f, inputSpectrum[static_cast<size_t> (bin)]));
            const float outputDb = 20.0f * std::log10 (juce::jmax (1.0e-6f, outputSpectrum[static_cast<size_t> (bin)]));
            const float inputY = spectrumBounds.getBottom()
                               - juce::jlimit (0.0f, 1.0f, (inputDb - minDb) / (maxDb - minDb))
                               * spectrumBounds.getHeight();
            const float outputY = spectrumBounds.getBottom()
                                - juce::jlimit (0.0f, 1.0f, (outputDb - minDb) / (maxDb - minDb))
                                * spectrumBounds.getHeight();
            const float x = spectrumBounds.getX() + static_cast<float> (pixel);

            if (pixel == 0)
            {
                inputPath.startNewSubPath (x, inputY);
                outputPath.startNewSubPath (x, outputY);
            }
            else
            {
                inputPath.lineTo (x, inputY);
                outputPath.lineTo (x, outputY);
            }
        }

        g.setFont (juce::FontOptions (9.0f));
        for (const float db : { -20.0f, -40.0f, -60.0f, -80.0f })
        {
            const float y = spectrumBounds.getBottom() - (db - minDb) / (maxDb - minDb) * spectrumBounds.getHeight();
            g.setColour (juce::Colours::grey.withAlpha (0.25f));
            g.drawHorizontalLine (static_cast<int> (y), spectrumBounds.getX(), spectrumBounds.getRight());
            g.setColour (juce::Colours::lightgrey.withAlpha (0.7f));
            g.drawText (juce::String (static_cast<int> (db)), static_cast<int> (spectrumBounds.getX()),
                        static_cast<int> (y - 9.0f), 28, 10, juce::Justification::left, false);
        }

        g.setColour (juce::Colours::orange);
        g.strokePath (inputPath, juce::PathStrokeType (1.2f));
        g.setColour (juce::Colours::cyan);
        g.strokePath (outputPath, juce::PathStrokeType (1.5f));

        const bool hasTonalPeak = inputPeakAmplitude > 0.001f && inputPeakAmplitude > inputRms * 1.2f
                               && inputPeakFrequencyHz >= 40.0f;
        juce::String analysisText = hasTonalPeak
            ? "Dominant input peak " + juce::String (inputPeakFrequencyHz, 1) + " Hz"
            : "No confident tonal peak";
        if (hasTonalPeak && processingMode == 1 && modFrequencyParam != nullptr)
        {
            const float carrier = modFrequencyParam->load();
            analysisText += " | carrier / peak "
                          + juce::String (carrier / inputPeakFrequencyHz, 2) + ":1";
        }
        g.setColour (juce::Colours::white);
        g.drawText (analysisText, annotationBounds.toNearestInt(),
                    juce::Justification::right, false);

        for (const float frequency : { 100.0f, 1000.0f, 10000.0f })
        {
            if (frequency > maxFrequency)
                continue;
            const float x = spectrumBounds.getX()
                          + std::log (frequency / 20.0f) / logFrequencyRange * spectrumBounds.getWidth();
            g.setColour (juce::Colours::grey.withAlpha (0.35f));
            g.drawVerticalLine (static_cast<int> (x), spectrumBounds.getY(), spectrumBounds.getBottom());
            g.setColour (juce::Colours::lightgrey.withAlpha (0.8f));
            const juce::String label = frequency >= 1000.0f
                                     ? juce::String (frequency / 1000.0f, 0) + "k"
                                     : juce::String (static_cast<int> (frequency));
            g.drawText (label, juce::Rectangle<int> (static_cast<int> (x - 16.0f),
                                                      static_cast<int> (frequencyLabelBounds.getY()),
                                                      32, 12),
                        juce::Justification::centred, false);
        }

        g.setFont (juce::FontOptions (9.0f));
        g.setColour (juce::Colours::orange);
        g.drawText ("Input", annotationBounds.toNearestInt().removeFromLeft (45),
                    juce::Justification::left, false);
        g.setColour (juce::Colours::cyan);
        g.drawText ("Output", annotationBounds.toNearestInt().removeFromLeft (90).withX (
                        static_cast<int> (annotationBounds.getX() + 48.0f)),
                    juce::Justification::left, false);
        return;
    }

    const float delayMs    = delayParam       ? delayParam->load()       : 10.0f;
    const float BL         = blendParam       ? blendParam->load()       : 1.0f;
    const float FF         = feedforwardParam ? feedforwardParam->load() : 0.5f;
    const float FB         = feedbackParam    ? feedbackParam->load()    : 0.0f;
    const bool  autoGainOn = autoGainParam    ? (autoGainParam->load() > 0.5f) : true;
    const bool  dampingOn  = dampingParam     ? (dampingParam->load() > 0.5f)  : true;

    const bool  modOn      = modEnableParam   ? (modEnableParam->load() > 0.5f) : false;
    const float modType    = modTypeParam     ? modTypeParam->load()            : 0.0f;
    const float modDepth   = modDepthParam    ? modDepthParam->load()           : 0.0f;

    // Calculate instantaneous modulated delay offset
    float modOffsetMs = 0.0f;
    if (modOn)
    {
        if (modType < 0.5f)
            modOffsetMs = 0.5f * (std::sin (guiPhase) + 1.0f) * modDepth;
        else
            modOffsetMs = 0.5f * (juce::jlimit (-1.0f, 1.0f, guiNoiseState) + 1.0f) * modDepth;
    }

    const float totalDelayMs = juce::jlimit (0.1f, 100.0f, delayMs + modOffsetMs);

    // L2 Normalization factor
    const float absFb = std::abs (FB);
    const float normScale = autoGainOn ? std::sqrt (1.0f - (absFb * absFb)) : 1.0f;

    // Sample rate & delay in samples
    constexpr double fs = 44100.0;
    const double delaySamples = (totalDelayMs / 1000.0) * fs;

    juce::Path responsePath;
    const int numPixels = static_cast<int> (bounds.getWidth());
    constexpr float minDb = -24.0f;
    constexpr float maxDb =  24.0f;

    for (int i = 0; i < numPixels; ++i)
    {
        const float normX = static_cast<float> (i) / static_cast<float> (numPixels - 1);
        const double freq = 20.0 * std::pow (1000.0, normX);
        const double omega = 2.0 * M_PI * freq / fs;

        const std::complex<double> z_inv_M = std::polar (1.0, -omega * delaySamples);

        constexpr double dampingCoeff = 0.25;
        std::complex<double> H_feedback = 1.0;

        if (dampingOn)
        {
            const std::complex<double> z_inv = std::polar (1.0, -omega);
            H_feedback = (1.0 - dampingCoeff) / (1.0 - (dampingCoeff * z_inv));
        }

        const std::complex<double> numerator   = (static_cast<double> (BL * normScale)) + (static_cast<double> (FF) * z_inv_M);
        const std::complex<double> denominator = 1.0 - (static_cast<double> (FB) * H_feedback * z_inv_M);

        const std::complex<double> H = numerator / denominator;
        const double mag = std::abs (H);

        const float db = static_cast<float> (20.0 * std::log10 (std::max (mag, 1e-5)));

        const float normY = juce::jlimit (0.0f, 1.0f, (db - minDb) / (maxDb - minDb));
        const float y = bounds.getBottom() - (normY * bounds.getHeight());
        const float x = bounds.getX() + static_cast<float> (i);

        if (i == 0)
            responsePath.startNewSubPath (x, y);
        else
            responsePath.lineTo (x, y);
    }

    // Grid lines (0 dB center)
    g.setColour (juce::Colours::grey.withAlpha (0.25f));
    const float zeroDbY = bounds.getBottom() - (((0.0f - minDb) / (maxDb - minDb)) * bounds.getHeight());
    g.drawHorizontalLine (static_cast<int> (zeroDbY), bounds.getX(), bounds.getRight());

    // Draw Frequency Curve
    g.setColour (juce::Colours::cyan);
    g.strokePath (responsePath, juce::PathStrokeType (1.8f));
}

void FrequencyResponsePlot::resized() {}