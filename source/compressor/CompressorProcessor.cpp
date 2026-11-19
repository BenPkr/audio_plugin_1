#include "CompressorProcessor.h"
#include "CompressorEditor.h"

CompressorAudioProcessor::CompressorAudioProcessor()
    : AudioProcessor (BusesProperties()
                        .withInput ("Input", juce::AudioChannelSet::stereo(), true)
                        .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "CompressorParameters", createCompressorParameterLayout())
{
    thresholdParam = apvts.getRawParameterValue (CompressorParameterIDs::threshold);
    ratioParam = apvts.getRawParameterValue (CompressorParameterIDs::ratio);
    kneeParam = apvts.getRawParameterValue (CompressorParameterIDs::knee);
    attackParam = apvts.getRawParameterValue (CompressorParameterIDs::attack);
    releaseParam = apvts.getRawParameterValue (CompressorParameterIDs::release);
    makeupGainParam = apvts.getRawParameterValue (CompressorParameterIDs::makeupGain);
}

void CompressorAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    juce::ignoreUnused (samplesPerBlock);
    compressor.prepare (sampleRate);
}

void CompressorAudioProcessor::releaseResources()
{
    compressor.reset();
}

bool CompressorAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto input = layouts.getMainInputChannelSet();
    const auto output = layouts.getMainOutputChannelSet();
    return input == output && (input == juce::AudioChannelSet::mono()
                            || input == juce::AudioChannelSet::stereo());
}

void CompressorAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer,
                                             juce::MidiBuffer& midiMessages)
{
    juce::ignoreUnused (midiMessages);
    juce::ScopedNoDenormals noDenormals;

    compressor.processBlock (
        buffer,
        thresholdParam->load(),
        ratioParam->load(),
        kneeParam->load(),
        attackParam->load(),
        releaseParam->load(),
        makeupGainParam->load());
}

juce::AudioProcessorEditor* CompressorAudioProcessor::createEditor()
{
    return new CompressorAudioProcessorEditor (*this);
}

bool CompressorAudioProcessor::hasEditor() const
{
    return true;
}

const juce::String CompressorAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool CompressorAudioProcessor::acceptsMidi() const
{
    return false;
}

bool CompressorAudioProcessor::producesMidi() const
{
    return false;
}

bool CompressorAudioProcessor::isMidiEffect() const
{
    return false;
}

double CompressorAudioProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int CompressorAudioProcessor::getNumPrograms()
{
    return 1;
}

int CompressorAudioProcessor::getCurrentProgram()
{
    return 0;
}

void CompressorAudioProcessor::setCurrentProgram (int index)
{
    juce::ignoreUnused (index);
}

const juce::String CompressorAudioProcessor::getProgramName (int index)
{
    juce::ignoreUnused (index);
    return {};
}

void CompressorAudioProcessor::changeProgramName (int index, const juce::String& newName)
{
    juce::ignoreUnused (index, newName);
}

void CompressorAudioProcessor::getStateInformation (juce::MemoryBlock& destination)
{
    auto state = apvts.copyState();
    std::unique_ptr<juce::XmlElement> xml (state.createXml());
    copyXmlToBinary (*xml, destination);
}

void CompressorAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xml (getXmlFromBinary (data, sizeInBytes));
    if (xml != nullptr && xml->hasTagName (apvts.state.getType()))
        apvts.replaceState (juce::ValueTree::fromXml (*xml));
}

float CompressorAudioProcessor::getCurrentInputLevelDb() const noexcept
{
    return compressor.getCurrentInputLevelDb();
}

float CompressorAudioProcessor::getCurrentGainReductionDb() const noexcept
{
    return compressor.getCurrentGainReductionDb();
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new CompressorAudioProcessor();
}
