/*
  ==============================================================================

	This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
Simple_EqualizerAudioProcessor::Simple_EqualizerAudioProcessor()
#ifndef JucePlugin_PreferredChannelConfigurations
	: AudioProcessor(BusesProperties()
#if ! JucePlugin_IsMidiEffect
#if ! JucePlugin_IsSynth
		.withInput("Input", juce::AudioChannelSet::stereo(), true)
#endif
		.withOutput("Output", juce::AudioChannelSet::stereo(), true)
#endif
	)
#endif
{
}

Simple_EqualizerAudioProcessor::~Simple_EqualizerAudioProcessor()
{
}

//==============================================================================
const juce::String Simple_EqualizerAudioProcessor::getName() const
{
	return JucePlugin_Name;
}

bool Simple_EqualizerAudioProcessor::acceptsMidi() const
{
#if JucePlugin_WantsMidiInput
	return true;
#else
	return false;
#endif
}

bool Simple_EqualizerAudioProcessor::producesMidi() const
{
#if JucePlugin_ProducesMidiOutput
	return true;
#else
	return false;
#endif
}

bool Simple_EqualizerAudioProcessor::isMidiEffect() const
{
#if JucePlugin_IsMidiEffect
	return true;
#else
	return false;
#endif
}

double Simple_EqualizerAudioProcessor::getTailLengthSeconds() const
{
	return 0.0;
}

int Simple_EqualizerAudioProcessor::getNumPrograms()
{
	return 1;   // NB: some hosts don't cope very well if you tell them there are 0 programs,
	// so this should be at least 1, even if you're not really implementing programs.
}

int Simple_EqualizerAudioProcessor::getCurrentProgram()
{
	return 0;
}

void Simple_EqualizerAudioProcessor::setCurrentProgram(int index)
{
}

const juce::String Simple_EqualizerAudioProcessor::getProgramName(int index)
{
	return {};
}

void Simple_EqualizerAudioProcessor::changeProgramName(int index, const juce::String& newName)
{
}

//==============================================================================
void Simple_EqualizerAudioProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
	// Use this method as the place to do any pre-playback
	// initialisation that you need..

	juce::dsp::ProcessSpec spec;

	spec.maximumBlockSize = samplesPerBlock;
	// As this is a mono channel
	spec.numChannels = 1;
	spec.sampleRate = sampleRate;

	leftChannel.prepare(spec);
	rightChannel.prepare(spec);

}

void Simple_EqualizerAudioProcessor::releaseResources()
{
	// When playback stops, you can use this as an opportunity to free up any
	// spare memory, etc.
}

#ifndef JucePlugin_PreferredChannelConfigurations
bool Simple_EqualizerAudioProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const
{
#if JucePlugin_IsMidiEffect
	juce::ignoreUnused(layouts);
	return true;
#else
	// This is the place where you check if the layout is supported.
	// In this template code we only support mono or stereo.
	// Some plugin hosts, such as certain GarageBand versions, will only
	// load plugins that support stereo bus layouts.
	if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
		&& layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
		return false;

	// This checks if the input layout matches the output layout
#if ! JucePlugin_IsSynth
	if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
		return false;
#endif

	return true;
#endif
}
#endif

void Simple_EqualizerAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
	juce::ScopedNoDenormals noDenormals;
	auto totalNumInputChannels = getTotalNumInputChannels();
	auto totalNumOutputChannels = getTotalNumOutputChannels();

	// In case we have more outputs than inputs, this code clears any output
	// channels that didn't contain input data, (because these aren't
	// guaranteed to be empty - they may contain garbage).
	// This is here to avoid people getting screaming feedback
	// when they first compile a plugin, but obviously you don't need to keep
	// this code if your algorithm always overwrites all the output channels.
	for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
		buffer.clear(i, 0, buffer.getNumSamples());

	// Initialize AudioBlock with incomming buffer
	juce::dsp::AudioBlock<float> block(buffer);

	// Seperate left and right channel indices
	auto leftBlock = block.getSingleChannelBlock(0);
	auto rightBlock = block.getSingleChannelBlock(1);

	juce::dsp::ProcessContextReplacing<float> leftContext(leftBlock);
	juce::dsp::ProcessContextReplacing<float> rightContext(rightBlock);

	leftChain.process(leftContext);
	rightChain.process(rightContext);
}

//==============================================================================
bool Simple_EqualizerAudioProcessor::hasEditor() const
{
	return true; // (change this to false if you choose to not supply an editor)
}

juce::AudioProcessorEditor* Simple_EqualizerAudioProcessor::createEditor()
{
//	return new Simple_EqualizerAudioProcessorEditor(*this);
	return new juce::GenericAudioProcessorEditor(*this);
}

//==============================================================================
void Simple_EqualizerAudioProcessor::getStateInformation(juce::MemoryBlock& destData)
{
	// You should use this method to store your parameters in the memory block.
	// You could do that either as raw data, or use the XML or ValueTree classes
	// as intermediaries to make it easy to save and load complex data.
}

void Simple_EqualizerAudioProcessor::setStateInformation(const void* data, int sizeInBytes)
{
	// You should use this method to restore your parameters from this memory block,
	// whose contents will have been created by the getStateInformation() call.
}

juce::AudioProcessorValueTreeState::ParameterLayout Simple_EqualizerAudioProcessor::createParameterLayout()
{
	// Creates and prepares Process Value Tree State information to be held
	// See Audio Processor Parameter documentation 
	juce::AudioProcessorValueTreeState::ParameterLayout layout;
	auto humanLowestFreq = 20.f;
	auto humanHighesFreq = 20000.f;
	layout.add(std::make_unique<juce::AudioParameterFloat>(
		"LowCut Freq", "LowCut Freq", juce::NormalisableRange<float>(humanLowestFreq, humanHighesFreq, 1.f, 1.f), humanLowestFreq));
	layout.add(std::make_unique<juce::AudioParameterFloat>(
		"HighCut Freq", "HighCut Freq", juce::NormalisableRange<float>(humanLowestFreq, humanHighesFreq, 1.f, 1.f), humanHighesFreq));
	layout.add(std::make_unique<juce::AudioParameterFloat>(
		"Peak Freq", "Peak Freq", juce::NormalisableRange<float>(humanLowestFreq, humanHighesFreq, 1.f, 1.f), 750.f));
	layout.add(std::make_unique<juce::AudioParameterFloat>(
		"Peak Gain", "Peak Gain", juce::NormalisableRange<float>(-24.f, 24.f, .5f, 1.f), 0.0f));
	layout.add(std::make_unique<juce::AudioParameterFloat>(
		"Peak Quailty", "Peak Quailty", juce::NormalisableRange<float>(0.1f, 10.f, .05f, 1.f), 1.f));

	juce::StringArray stringArray;

	// Create a string array with 12, 24, 36 and 48db options
	for (int i = 0; i < 4; ++i) {
		juce::String str;
		str << (12 + i * 12);
		str << " db/Oct";
		stringArray.add(str);
	}
	layout.add(std::make_unique<juce::AudioParameterChoice>("LowCutSlope","LowCutSlope", stringArray, 0));
	layout.add(std::make_unique<juce::AudioParameterChoice>("HighCutSlope","HighCutSlope", stringArray, 0));
	return layout;
}

//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
	return new Simple_EqualizerAudioProcessor();
}
