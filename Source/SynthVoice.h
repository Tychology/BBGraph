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


	void prepareToPlay(double sampleRate, int samplesPerBlock, int outputChannels);


	//void sync(juce::AudioPlayHead::CurrentPositionInfo& positionInfo)
	//{
	//	processorSequence->sync(positionInfo, getSampleRate());

	//}

	//void setBPM(double bpm)
	//{
	//	if (processorSequence == nullptr) return;
	//	processorSequence->setBPM(bpm, getSampleRate());
	//}


	//void setTimes(double timeInSeconds, juce::int64 timeInSamples)
	//{
	//	if (processorSequence == nullptr) return;
	//	processorSequence->setTimes( timeInSeconds,  timeInSamples, getSampleRate());
	//}

	void setProcessorSequence(NodeProcessorSequence* sequence);


	void update (juce::ADSR::Parameters parameters, double bps, double freeSeconds, double freeSamples, double positionSeconds, double positionSamples);

private:
	juce::ADSR adsr;
	std::unique_ptr<NodeProcessorSequence> processorSequence;
	

};


class SynthSound : public juce::SynthesiserSound
{
public:
    bool appliesToNote(int midiNoteNumber) override {return true;}
    bool appliesToChannel(int midiChannel) override {return true;}
};