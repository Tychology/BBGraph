/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "GraphRenderSequence.h"
#include "CustomRange.h"


//==============================================================================
ByteBeatNodeGraphAudioProcessor::ByteBeatNodeGraphAudioProcessor()
#ifndef JucePlugin_PreferredChannelConfigurations
     : AudioProcessor (BusesProperties()
                     #if ! JucePlugin_IsMidiEffect
                      #if ! JucePlugin_IsSynth
                       .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                      #endif
                       .withOutput ("Output", juce::AudioChannelSet::stereo(), true)
                     #endif
                       ),
#elif
    :
#endif
	apvts(*this, nullptr, "apvts", createParameters()),
	parameterManager(apvts),
	graph(*this, parameterManager)
{
       // graph.addChangeListener(this);

    synth.addSound(new SynthSound());
    synth.addVoice(new SynthVoice());
    synth.addVoice(new SynthVoice());
    synth.addVoice(new SynthVoice());
    synth.addVoice(new SynthVoice());
    synth.addVoice(new SynthVoice());
    synth.addVoice(new SynthVoice());
    synth.addVoice(new SynthVoice());
    synth.addVoice(new SynthVoice());
    synth.setNoteStealingEnabled(true);
}

ByteBeatNodeGraphAudioProcessor::~ByteBeatNodeGraphAudioProcessor()
{
}


//==============================================================================
void ByteBeatNodeGraphAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    freeSeconds = 0;
    freeSamples = 0;

    synth.setCurrentPlaybackSampleRate(sampleRate);
    synth.setNoteStealingEnabled(false);

   for (int i = 0; i < synth.getNumVoices(); ++i)
    {
        if (auto voice = dynamic_cast<SynthVoice*>(synth.getVoice(i)))
        {
	        voice->prepareToPlay(sampleRate, samplesPerBlock, getTotalNumOutputChannels());
        }
    }
}

void ByteBeatNodeGraphAudioProcessor::releaseResources()
{
    // When playback stops, you can use this as an opportunity to free up any
    // spare memory, etc.
}

#ifndef JucePlugin_PreferredChannelConfigurations
bool ByteBeatNodeGraphAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
  #if JucePlugin_IsMidiEffect
    juce::ignoreUnused (layouts);
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

void ByteBeatNodeGraphAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    auto totalNumInputChannels  = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();

    // In case we have more outputs than inputs, this code clears any output
    // channels that didn't contain input data, (because these aren't
    // guaranteed to be empty - they may contain garbage).
    // This is here to avoid people getting screaming feedback
    // when they first compile a plugin, but obviously you don't need to keep
    // this code if your algorithm always overwrites all the output channels.
    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear (i, 0, buffer.getNumSamples());



    auto playHead = getPlayHead();
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


    //Beats per seconds
    auto bps = beatsPerMinute.get() / 60;


    juce::ADSR::Parameters adsrParameters
	{
		apvts.getRawParameterValue("Attack")->load(),
        apvts.getRawParameterValue("Decay")->load(),
        apvts.getRawParameterValue("Sustain")->load(),
        apvts.getRawParameterValue("Release")->load()
    };


    for (int i = 0; i < synth.getNumVoices(); ++i)
    {
	    if (auto voice = dynamic_cast<SynthVoice*>(synth.getVoice(i)))
	    {
	    	voice->update(adsrParameters, positionInfo.isPlaying, bps, freeSeconds, freeSamples, positionSeconds, positionSamples);
        }
    }


    freeSeconds += buffer.getNumSamples() / getSampleRate();
    freeSamples += buffer.getNumSamples();






    synth.renderNextBlock(buffer, midiMessages, 0, buffer.getNumSamples());


    //auto* channelZero = buffer.getWritePointer (0);

    ////Currently we juce output the same audio on all
    // for (int channel = 1; channel < totalNumOutputChannels; ++channel)
    //{
    //    auto* channelData = buffer.getWritePointer (channel);

    //    buffer.addFrom(channel, 0, channelZero, buffer.getNumSamples());

    //}



    buffer.applyGain(juce::Decibels::decibelsToGain(apvts.getRawParameterValue("Volume")->load()));
}


//==============================================================================
bool ByteBeatNodeGraphAudioProcessor::hasEditor() const
{
    return true; // (change this to false if you choose to not supply an editor)
}

juce::AudioProcessorEditor* ByteBeatNodeGraphAudioProcessor::createEditor()
{
    return new ByteBeatNodeGraphAudioProcessorEditor (*this);
}

//==============================================================================
void ByteBeatNodeGraphAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    // You should use this method to store your parameters in the memory block.
    // You could do that either as raw data, or use the XML or ValueTree classes
    // as intermediaries to make it easy to save and load complex data.

	juce::MemoryOutputStream mos(destData, true);
    juce::ValueTree pluginState ("PluginState");

    pluginState.addChild(apvts.state, -1, nullptr);
    pluginState.addChild(graph.toValueTree(), -1, nullptr);

    pluginState.writeToStream(mos);
    //apvts.state.writeToStream(mos);
}

void ByteBeatNodeGraphAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    // You should use this method to restore your parameters from this memory block,
    // whose contents will have been created by the getStateInformation() call.

	auto tree = juce::ValueTree::readFromData(data, sizeInBytes);
    DBG(tree.toXmlString());
    if( tree.isValid() )
    {
        apvts.replaceState(tree.getChildWithName("apvts"));

        graph.restoreFromTree(tree.getChildWithName("graph"));
    }
}

void ByteBeatNodeGraphAudioProcessor::setNodeProcessorSequence(GraphRenderSequence& sequence)
{
    const juce::ScopedLock sl (getCallbackLock());

	for (int i = 0; i < synth.getNumVoices(); ++i)
	{
        auto nodeProcessorSequence = sequence.createNodeProcessorSequence(apvts);

		if (auto voice = dynamic_cast<SynthVoice*>(synth.getVoice(i)))
		{
            voice->setProcessorSequence(nodeProcessorSequence);
		}
	}

}

juce::AudioProcessorValueTreeState::ParameterLayout ByteBeatNodeGraphAudioProcessor::createParameters()
{
     std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    //Implement a range that is switchble between linear and logarithmic
     juce::NormalisableRange<float> defaultRange{0, 255};

     for (int i = 1; i <= total_num_prams; ++i)
     {
         params.push_back(std::make_unique<juce::AudioParameterFloat>(juce::String(i), juce::String("Parameter ") += i, defaultRange, 0));
     }

	params.push_back(std::make_unique<juce::AudioParameterFloat>("Attack", "Attack", logRange<float>(0.01, 10), 0.01));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("Decay", "Decay", logRange<float>(0.01, 10), 0.01));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("Sustain", "Sustain", juce::NormalisableRange<float>(0, 1), 1));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("Release", "Release", logRange<float>(0.01, 10), 0.01));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("Volume", "Volume", juce::NormalisableRange<float>(-72, 0), -12));

    

     return {params.begin(), params.end()};
}










//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new ByteBeatNodeGraphAudioProcessor();
}



//==============================================================================
const juce::String ByteBeatNodeGraphAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool ByteBeatNodeGraphAudioProcessor::acceptsMidi() const
{
   #if JucePlugin_WantsMidiInput
    return true;
   #else
    return false;
   #endif
}

bool ByteBeatNodeGraphAudioProcessor::producesMidi() const
{
   #if JucePlugin_ProducesMidiOutput
    return true;
   #else
    return false;
   #endif
}

bool ByteBeatNodeGraphAudioProcessor::isMidiEffect() const
{
   #if JucePlugin_IsMidiEffect
    return true;
   #else
    return false;
   #endif
}

double ByteBeatNodeGraphAudioProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int ByteBeatNodeGraphAudioProcessor::getNumPrograms()
{
    return 1;   // NB: some hosts don't cope very well if you tell them there are 0 programs,
                // so this should be at least 1, even if you're not really implementing programs.
}

int ByteBeatNodeGraphAudioProcessor::getCurrentProgram()
{
    return 0;
}

void ByteBeatNodeGraphAudioProcessor::setCurrentProgram (int index)
{
}

const juce::String ByteBeatNodeGraphAudioProcessor::getProgramName (int index)
{
    return {};
}

void ByteBeatNodeGraphAudioProcessor::changeProgramName (int index, const juce::String& newName)
{
}
