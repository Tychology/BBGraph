#include "PluginProcessor.h"

#include "CustomRange.h"
#include "GraphRenderSequence.h"
#include "PluginEditor.h"
#include "SynthVoice.h"

ByteBeatNodeGraphAudioProcessor::ByteBeatNodeGraphAudioProcessor()
	: AudioProcessor(BusesProperties()
		.withOutput("Output", juce::AudioChannelSet::stereo(), true)),

	apvts(*this, nullptr, "apvts", createParameters()),
	parameterManager(apvts),
	graph(*this, parameterManager)
{
	synth.addSound(new SynthSound());
	for (int i = 0; i < total_num_voices; ++i)
	{
		synth.addVoice(new SynthVoice());
	}

	synth.setNoteStealingEnabled(true);
}

void ByteBeatNodeGraphAudioProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
	freeSeconds = 0;
	freeSamples = 0;

	synth.setCurrentPlaybackSampleRate(sampleRate);
	synth.setNoteStealingEnabled(false);

	for (int i = 0; i < synth.getNumVoices(); ++i)
	{
		if (const auto voice = dynamic_cast<SynthVoice*>(synth.getVoice(i)))
		{
			voice->prepareToPlay(sampleRate, samplesPerBlock, getTotalNumOutputChannels());
		}
	}
}

void ByteBeatNodeGraphAudioProcessor::releaseResources()
{
	// No resources to release
}

bool ByteBeatNodeGraphAudioProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const
{
	// We only support stereo output
	return layouts.getMainOutputChannelSet() == juce::AudioChannelSet::stereo();
}

void ByteBeatNodeGraphAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
	juce::ScopedNoDenormals noDenormals;
	const auto totalNumInputChannels = getTotalNumInputChannels();
	const auto totalNumOutputChannels = getTotalNumOutputChannels();

	// In case we have more outputs than inputs, this code clears any output
	// channels that didn't contain input data, (because these aren't
	// guaranteed to be empty - they may contain garbage).
	// This is here to avoid people getting screaming feedback
	// when they first compile a plugin, but obviously you don't need to keep
	// this code if your algorithm always overwrites all the output channels.
	for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
		buffer.clear(i, 0, buffer.getNumSamples());


	const auto playHead = getPlayHead();
	double positionSeconds = 0;
	double positionSamples = 0;
	juce::AudioPlayHead::CurrentPositionInfo positionInfo;
	
	if (playHead != nullptr && playHead->getCurrentPosition(positionInfo))
	{
		positionSeconds = positionInfo.timeInSeconds;
		positionSamples = positionInfo.timeInSamples;

		if (syncToHost.get())
		{
			beatsPerMinute.set(positionInfo.bpm);
			sendChangeMessage();
		}
	}
	else
	{
		if (syncToHost.get())
		{
			beatsPerMinute.set(0);
			sendChangeMessage();
		}
	}
	
	// Beats per seconds
	const auto bps = beatsPerMinute.get() / 60;
	
	juce::ADSR::Parameters adsrParameters
	{
		apvts.getRawParameterValue("Attack")->load(),
		apvts.getRawParameterValue("Decay")->load(),
		apvts.getRawParameterValue("Sustain")->load(),
		apvts.getRawParameterValue("Release")->load()
	};
	
	for (int i = 0; i < synth.getNumVoices(); ++i)
	{
		if (const auto voice = dynamic_cast<SynthVoice*>(synth.getVoice(i)))
		{
			voice->update(adsrParameters, positionInfo.isPlaying, bps, freeSeconds, freeSamples, positionSeconds, positionSamples);
		}
	}
	
	freeSeconds += buffer.getNumSamples() / getSampleRate();
	freeSamples += buffer.getNumSamples();
	
	synth.renderNextBlock(buffer, midiMessages, 0, buffer.getNumSamples());

	buffer.applyGain(juce::Decibels::decibelsToGain(apvts.getRawParameterValue("Volume")->load()));
}

bool ByteBeatNodeGraphAudioProcessor::hasEditor() const
{
	return true;
}

juce::AudioProcessorEditor* ByteBeatNodeGraphAudioProcessor::createEditor()
{
	return new ByteBeatNodeGraphAudioProcessorEditor(*this);
}

void ByteBeatNodeGraphAudioProcessor::getStateInformation(juce::MemoryBlock& destData)
{
	juce::MemoryOutputStream mos(destData, true);
	juce::ValueTree pluginState("PluginState");

	pluginState.addChild(apvts.state, -1, nullptr);
	pluginState.addChild(graph.toValueTree(), -1, nullptr);

	pluginState.writeToStream(mos);
}

void ByteBeatNodeGraphAudioProcessor::setStateInformation(const void* data, int sizeInBytes)
{
	const auto tree = juce::ValueTree::readFromData(data, sizeInBytes);
	if (tree.isValid())
	{
		apvts.replaceState(tree.getChildWithName("apvts"));
		graph.restoreFromTree(tree.getChildWithName("graph"));
	}
}

void ByteBeatNodeGraphAudioProcessor::setNodeProcessorSequence(GraphRenderSequence& sequence)
{
	const juce::ScopedLock sl(getCallbackLock());

	for (int i = 0; i < synth.getNumVoices(); ++i)
	{
		const auto nodeProcessorSequence = sequence.createNodeProcessorSequence(apvts);

		if (const auto voice = dynamic_cast<SynthVoice*>(synth.getVoice(i)))
		{
			voice->setProcessorSequence(nodeProcessorSequence);
		}
	}
}

juce::AudioProcessorValueTreeState::ParameterLayout ByteBeatNodeGraphAudioProcessor::createParameters() const
{
	std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

	// Implement a range that is switchable between linear and logarithmic
	juce::NormalisableRange<float> defaultRange{ 0, 255 };

	for (int i = 1; i <= total_num_params; ++i)
	{
		params.push_back(std::make_unique<juce::AudioParameterFloat>(juce::String(i), juce::String("Parameter ") += i, defaultRange, 0));
	}

	params.push_back(std::make_unique<juce::AudioParameterFloat>("Attack", "Attack", logRange<float>(0.01, 10), 0.01));
	params.push_back(std::make_unique<juce::AudioParameterFloat>("Decay", "Decay", logRange<float>(0.01, 10), 0.01));
	params.push_back(std::make_unique<juce::AudioParameterFloat>("Sustain", "Sustain", juce::NormalisableRange<float>(0, 1), 1));
	params.push_back(std::make_unique<juce::AudioParameterFloat>("Release", "Release", logRange<float>(0.01, 10), 0.01));
	params.push_back(std::make_unique<juce::AudioParameterFloat>("Volume", "Volume", juce::NormalisableRange<float>(-72, 0), -12));
	
	return { params.begin(), params.end() };
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
	return new ByteBeatNodeGraphAudioProcessor();
}

#pragma region Plugin Information

const juce::String ByteBeatNodeGraphAudioProcessor::getName() const
{
	return JucePlugin_Name;
}

bool ByteBeatNodeGraphAudioProcessor::acceptsMidi() const
{
	return true;
}

bool ByteBeatNodeGraphAudioProcessor::producesMidi() const
{
	return false;
}

bool ByteBeatNodeGraphAudioProcessor::isMidiEffect() const
{
	return false;
}

double ByteBeatNodeGraphAudioProcessor::getTailLengthSeconds() const
{
	return 0.0;
}

int ByteBeatNodeGraphAudioProcessor::getNumPrograms()
{
	return 1;
}

int ByteBeatNodeGraphAudioProcessor::getCurrentProgram()
{
	return 0;
}

void ByteBeatNodeGraphAudioProcessor::setCurrentProgram(int index)
{
}

const juce::String ByteBeatNodeGraphAudioProcessor::getProgramName(int index)
{
	return {};
}

void ByteBeatNodeGraphAudioProcessor::changeProgramName(int index, const juce::String& newName)
{
}

#pragma endregion
