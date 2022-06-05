/*
  ==============================================================================

    SynthVoice.h
    Created: 6 May 2022 6:45:58pm
    Author:  Jonas

  ==============================================================================
*/

#pragma once
#include <JuceHeader.h>
//#include "InternalNodeGraph.h"
#include "NodeProcessor.h"


class SynthVoice : public juce::SynthesiserVoice
{
public:

	bool canPlaySound(juce::SynthesiserSound*) override;
	void startNote(int midiNoteNumber, float velocity, juce::SynthesiserSound* sound,
		int currentPitchWheelPosition) override;
	void stopNote(float velocity, bool allowTailOff) override;
	void pitchWheelMoved(int newPitchWheelValue) override;
	void controllerMoved(int controllerNumber, int newControllerValue) override;
	void renderNextBlock(juce::AudioBuffer<float>& outputBuffer, int startSample, int numSamples) override;

	void SynthVoice::prepareToPlay(double sampleRate, int samplesPerBlock, int outputChannels)
	{
		adsr.setSampleRate(sampleRate);
		//byte.prepareToPlay(sampleRate);
		
		juce::ADSR::Parameters adsrParams;

		adsrParams.attack = 0.f;
		adsrParams.decay = 0.f;
		adsrParams.sustain = 1.f;
		adsrParams.release = 0.f;
		
		adsr.setParameters(adsrParams);
	}
	

	void sync(juce::AudioPlayHead::CurrentPositionInfo& positionInfo)
	{
		processorSequence->sync(positionInfo, getSampleRate());

	}

	void setBPM(double bpm)
	{
		if (processorSequence == nullptr) return;
		processorSequence->setBPM(bpm, getSampleRate());
	}


	void setTimes(double timeInSeconds, juce::int64 timeInSamples)
	{
		if (processorSequence == nullptr) return;
		processorSequence->setTimes( timeInSeconds,  timeInSamples, getSampleRate());
	}

	void setProcessorSequence(NodeProcessorSequence* sequence)
	{
		processorSequence = std::unique_ptr<NodeProcessorSequence>(sequence);
	}


	void updateADSR(juce::ADSR::Parameters parameters)
	{
		adsr.setParameters(parameters);
	}


private:
	juce::ADSR adsr;
	std::unique_ptr<NodeProcessorSequence> processorSequence;
	//InternalNodeGraph

};


class SynthSound : public juce::SynthesiserSound
{
public:
    bool appliesToNote(int midiNoteNumber) override {return true;}
    bool appliesToChannel(int midiChannel) override {return true;}
};