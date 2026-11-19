#include "ExpanderProcessor.h"
#include "ExpanderEditor.h"

ExpanderAudioProcessor::ExpanderAudioProcessor()
    : AudioProcessor (BusesProperties()
                        .withInput ("Input", juce::AudioChannelSet::stereo(), true)
                        .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "ExpanderParameters", createExpanderParameterLayout())
{
    thresholdParam = apvts.getRawParameterValue (ExpanderParameterIDs::threshold);
    ratioParam = apvts.getRawParameterValue (ExpanderParameterIDs::ratio);
    attackParam = apvts.getRawParameterValue (ExpanderParameterIDs::attack);
    releaseParam = apvts.getRawParameterValue (ExpanderParameterIDs::release);
}

void ExpanderAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    juce::ignoreUnused (samplesPerBlock);
    expander.prepare (sampleRate);
}

void ExpanderAudioProcessor::releaseResources()
{
    expander.reset();
}

bool ExpanderAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto input = layouts.getMainInputChannelSet();
    const auto output = layouts.getMainOutputChannelSet();
    return input == output && (input == juce::AudioChannelSet::mono()
                            || input == juce::AudioChannelSet::stereo());
}

void ExpanderAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer,
                                           juce::MidiBuffer& midiMessages)
{
    juce::ignoreUnused (midiMessages);
    juce::ScopedNoDenormals noDenormals;
    expander.processBlock (buffer, thresholdParam->load(), ratioParam->load(),
                           attackParam->load(), releaseParam->load());
}

juce::AudioProcessorEditor* ExpanderAudioProcessor::createEditor()
{
    return new ExpanderAudioProcessorEditor (*this);
}

bool ExpanderAudioProcessor::hasEditor() const
{
    return true;
}

const juce::String ExpanderAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool ExpanderAudioProcessor::acceptsMidi() const
{
    return false;
}

bool ExpanderAudioProcessor::producesMidi() const
{
    return false;
}

bool ExpanderAudioProcessor::isMidiEffect() const
{
    return false;
}

double ExpanderAudioProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int ExpanderAudioProcessor::getNumPrograms()
{
    return 1;
}

int ExpanderAudioProcessor::getCurrentProgram()
{
    return 0;
}

void ExpanderAudioProcessor::setCurrentProgram (int index)
{
    juce::ignoreUnused (index);
}

const juce::String ExpanderAudioProcessor::getProgramName (int index)
{
    juce::ignoreUnused (index);
    return {};
}

void ExpanderAudioProcessor::changeProgramName (int index, const juce::String& newName)
{
    juce::ignoreUnused (index, newName);
}

void ExpanderAudioProcessor::getStateInformation (juce::MemoryBlock& destination)
{
    auto state = apvts.copyState();
    std::unique_ptr<juce::XmlElement> xml (state.createXml());
    copyXmlToBinary (*xml, destination);
}

void ExpanderAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xml (getXmlFromBinary (data, sizeInBytes));
    if (xml != nullptr && xml->hasTagName (apvts.state.getType()))
        apvts.replaceState (juce::ValueTree::fromXml (*xml));
}

float ExpanderAudioProcessor::getCurrentInputLevelDb() const noexcept
{
    return expander.getCurrentInputLevelDb();
}

float ExpanderAudioProcessor::getCurrentGainReductionDb() const noexcept
{
    return expander.getCurrentGainReductionDb();
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new ExpanderAudioProcessor();
}
