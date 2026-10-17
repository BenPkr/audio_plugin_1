#include "PluginProcessor.h"
#include "PluginEditor.h"
#include <algorithm>
#include <cmath>

//==============================================================================
AudioPluginAudioProcessor::AudioPluginAudioProcessor()
     : AudioProcessor (BusesProperties()
                     #if ! JucePlugin_IsMidiEffect
                      #if ! JucePlugin_IsSynth
                       .withInput  ("Input",  juce::AudioChannelSet::mono(), true)
                      #endif
                       .withOutput ("Output", juce::AudioChannelSet::mono(), true)
                     #endif
                       ),
       apvts (*this, nullptr, "Parameters", createParameterLayout())
{
    modeParam         = apvts.getRawParameterValue (ParameterIDs::mode);

    // Cache base parameter pointers
    delayParam       = apvts.getRawParameterValue (ParameterIDs::delay);
    blendParam       = apvts.getRawParameterValue (ParameterIDs::blend);
    feedforwardParam = apvts.getRawParameterValue (ParameterIDs::feedforward);
    feedbackParam    = apvts.getRawParameterValue (ParameterIDs::feedback);

    // Cache Tame / Moorer toggle parameter pointers
    autoGainParam    = apvts.getRawParameterValue (ParameterIDs::autoGain);
    dampingParam     = apvts.getRawParameterValue (ParameterIDs::damping);

    // Cache modulation pointers
    modEnableParam    = apvts.getRawParameterValue (ParameterIDs::modEnable);
    modTypeParam      = apvts.getRawParameterValue (ParameterIDs::modType);
    modDepthParam     = apvts.getRawParameterValue (ParameterIDs::modDepth);
    modFrequencyParam = apvts.getRawParameterValue (ParameterIDs::modFrequency);
    tremWaveformParam = apvts.getRawParameterValue (ParameterIDs::tremWaveform);
}

AudioPluginAudioProcessor::~AudioPluginAudioProcessor()
{
}

//==============================================================================
const juce::String AudioPluginAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool AudioPluginAudioProcessor::acceptsMidi() const
{
   #if JucePlugin_WantsMidiInput
    return true;
   #else
    return false;
   #endif
}

bool AudioPluginAudioProcessor::producesMidi() const
{
   #if JucePlugin_ProducesMidiOutput
    return true;
   #else
    return false;
   #endif
}

bool AudioPluginAudioProcessor::isMidiEffect() const
{
   #if JucePlugin_IsMidiEffect
    return true;
   #else
    return false;
   #endif
}

double AudioPluginAudioProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int AudioPluginAudioProcessor::getNumPrograms()
{
    return 1;   // NB: some hosts don't cope very well if you tell them there are 0 programs,
                // so this should be at least 1, even if you're not really implementing programs.
}

int AudioPluginAudioProcessor::getCurrentProgram()
{
    return 0;
}

void AudioPluginAudioProcessor::setCurrentProgram (int index)
{
    juce::ignoreUnused (index);
}

const juce::String AudioPluginAudioProcessor::getProgramName (int index)
{
    juce::ignoreUnused (index);
    return {};
}

void AudioPluginAudioProcessor::changeProgramName (int index, const juce::String& newName)
{
    juce::ignoreUnused (index, newName);
}

//==============================================================================
void AudioPluginAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    // Prepare DSP filter buffer allocation on main thread
    combFilter.prepare (sampleRate, samplesPerBlock, getTotalNumInputChannels());
    ringModulator.prepare (sampleRate);
    tremolo.prepare (sampleRate);
    transitionBuffer.setSize (getTotalNumInputChannels(), samplesPerBlock, false, false, true);

    currentMode = modeParam ? juce::jlimit (0, 2, static_cast<int> (modeParam->load())) : 0;
    transitionFromMode = currentMode;
    transitionSamplesRemaining = 0;
    transitionLengthSamples = juce::jmax (1, static_cast<int> (std::round (sampleRate * 0.01)));
    resetMode (currentMode);
}

void AudioPluginAudioProcessor::releaseResources()
{
    // When playback stops, free up memory
    combFilter.reset();
    ringModulator.reset();
    tremolo.reset();
    transitionSamplesRemaining = 0;
}

bool AudioPluginAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
  #if JucePlugin_IsMidiEffect
    juce::ignoreUnused (layouts);
    return true;
  #else
    const auto mainInput  = layouts.getMainInputChannelSet();
    const auto mainOutput = layouts.getMainOutputChannelSet();

    return mainInput == juce::AudioChannelSet::mono()
        && mainOutput == juce::AudioChannelSet::mono();
  #endif
}

void AudioPluginAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer,
                                              juce::MidiBuffer& midiMessages)
{
    juce::ignoreUnused (midiMessages);

    juce::ScopedNoDenormals noDenormals;

    const int requestedMode = modeParam ? juce::jlimit (0, 2, static_cast<int> (modeParam->load())) : 0;
    if (requestedMode != currentMode)
    {
        transitionFromMode = currentMode;
        currentMode = requestedMode;
        resetMode (currentMode);
        transitionSamplesRemaining = transitionLengthSamples;
    }

    if (transitionSamplesRemaining > 0
        && buffer.getNumChannels() == transitionBuffer.getNumChannels()
        && buffer.getNumSamples() <= transitionBuffer.getNumSamples())
    {
        transitionBuffer.makeCopyOf (buffer, true);
        processMode (buffer, transitionFromMode);
        processMode (transitionBuffer, currentMode);

        for (int sample = 0; sample < buffer.getNumSamples(); ++sample)
        {
            const float blend = 1.0f - static_cast<float> (transitionSamplesRemaining)
                                      / static_cast<float> (transitionLengthSamples);
            for (int channel = 0; channel < buffer.getNumChannels(); ++channel)
            {
                const float oldOutput = buffer.getReadPointer (channel)[sample];
                const float newOutput = transitionBuffer.getReadPointer (channel)[sample];
                buffer.getWritePointer (channel)[sample] = oldOutput + blend * (newOutput - oldOutput);
            }

            if (transitionSamplesRemaining > 0)
                --transitionSamplesRemaining;
        }
    }
    else
    {
        if (transitionSamplesRemaining > 0)
            transitionSamplesRemaining = 0;
        processMode (buffer, currentMode);
    }

    if (buffer.getNumChannels() > 0)
    {
        const auto writeStart = scopeWriteIndex.load (std::memory_order_relaxed);
        const auto* output = buffer.getReadPointer (0);
        for (int sample = 0; sample < buffer.getNumSamples(); ++sample)
            scopeSamples[static_cast<size_t> ((writeStart + static_cast<std::uint64_t> (sample)) % scopeBufferSize)]
                .store (output[sample], std::memory_order_relaxed);
        scopeWriteIndex.store (writeStart + static_cast<std::uint64_t> (buffer.getNumSamples()), std::memory_order_release);
    }
}

void AudioPluginAudioProcessor::processMode (juce::AudioBuffer<float>& buffer, int mode)
{
    switch (mode)
    {
        case 0:
            combFilter.setDelayMs           (delayParam       ? delayParam->load()       : 10.0f);
            combFilter.setBlend             (blendParam       ? blendParam->load()       : 1.0f);
            combFilter.setFeedforward       (feedforwardParam ? feedforwardParam->load() : 0.5f);
            combFilter.setFeedback          (feedbackParam    ? feedbackParam->load()    : 0.0f);
            combFilter.setAutoGainEnabled   (autoGainParam    ? autoGainParam->load() > 0.5f : true);
            combFilter.setDampingEnabled    (dampingParam     ? dampingParam->load() > 0.5f : true);
            combFilter.setModEnabled        (modEnableParam   ? modEnableParam->load() > 0.5f : false);
            combFilter.setModType           (modTypeParam     ? (modTypeParam->load() > 0.5f ? 1 : 0) : 0);
            combFilter.setModDepthMs        (modDepthParam    ? modDepthParam->load() : 0.0f);
            combFilter.setModFrequencyHz    (modFrequencyParam ? modFrequencyParam->load() : 1.0f);
            combFilter.processBlock (buffer);
            break;

        case 1:
            ringModulator.setCarrierFrequency (modFrequencyParam ? modFrequencyParam->load() : 440.0f);
            ringModulator.setMix (blendParam ? 0.5f * (blendParam->load() + 1.0f) : 1.0f);
            ringModulator.processBlock (buffer);
            break;

        case 2:
            tremolo.setFrequencyHz (modFrequencyParam ? modFrequencyParam->load() : 5.0f);
            tremolo.setDepth (modDepthParam ? modDepthParam->load() * 0.01f : 0.8f);
            tremolo.setWaveform (static_cast<Tremolo::Waveform> (
                juce::jlimit (0, 2, tremWaveformParam ? static_cast<int> (tremWaveformParam->load()) : 0)));
            tremolo.processBlock (buffer);
            break;
    }
}

void AudioPluginAudioProcessor::resetMode (int mode)
{
    switch (mode)
    {
        case 0: combFilter.reset(); break;
        case 1: ringModulator.reset(); break;
        case 2: tremolo.reset(); break;
    }
}

int AudioPluginAudioProcessor::copyScopeSamples (float* destination, int maxSamples) const
{
    if (destination == nullptr || maxSamples <= 0)
        return 0;

    const auto end = scopeWriteIndex.load (std::memory_order_acquire);
    const auto count = static_cast<int> (std::min<std::uint64_t> (
        static_cast<std::uint64_t> (maxSamples), std::min<std::uint64_t> (end, scopeBufferSize)));
    const auto start = end - static_cast<std::uint64_t> (count);
    for (int sample = 0; sample < count; ++sample)
        destination[sample] = scopeSamples[static_cast<size_t> (
            (start + static_cast<std::uint64_t> (sample)) % scopeBufferSize)].load (std::memory_order_relaxed);

    return count;
}

//==============================================================================
bool AudioPluginAudioProcessor::hasEditor() const
{
    return true; // (change this to false if you choose to not supply an editor)
}

juce::AudioProcessorEditor* AudioPluginAudioProcessor::createEditor()
{
    return new AudioPluginAudioProcessorEditor (*this);
}

//==============================================================================
void AudioPluginAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    // Save parameter state
    auto state = apvts.copyState();
    std::unique_ptr<juce::XmlElement> xml (state.createXml());
    copyXmlToBinary (*xml, destData);
}

void AudioPluginAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    // Restore parameter state
    std::unique_ptr<juce::XmlElement> xmlState (getXmlFromBinary (data, sizeInBytes));

    if (xmlState.get() != nullptr)
        if (xmlState->hasTagName (apvts.state.getType()))
            apvts.replaceState (juce::ValueTree::fromXml (*xmlState));
}

//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new AudioPluginAudioProcessor();
}