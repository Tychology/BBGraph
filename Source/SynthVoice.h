#pragma once

#include <JuceHeader.h>

#include "NodeProcessor.h"

class SynthVoice : public juce::SynthesiserVoice
{
public:

	bool canPlaySound(juce::SynthesiserSound*) override;
	void startNote(int midiNoteNumber, float velocity, juce::SynthesiserSound* sound, int currentPitchWheelPosition) override;
	void stopNote(float velocity, bool allowTailOff) override;
	void pitchWheelMoved(int newPitchWheelValue) override;
	void controllerMoved(int controllerNumber, int newControllerValue) override;

	void prepareToPlay(double sampleRate, int samplesPerBlock, int outputChannels);
	void renderNextBlock(juce::AudioBuffer<float>& outputBuffer, int startSample, int numSamples) override;

	void setProcessorSequence(NodeProcessorSequence* sequence);

	void update(juce::ADSR::Parameters parameters, bool isPlaying, double bps, double freeSeconds, double freeSamples, double positionSeconds, double positionSamples);

private:
	juce::ADSR adsr;
	std::unique_ptr<NodeProcessorSequence> processorSequence;
	juce::AudioBuffer<float> buffer;
};


class SynthSound : public juce::SynthesiserSound
{
public:
	bool appliesToNote(int midiNoteNumber) override { return true; }
	bool appliesToChannel(int midiChannel) override { return true; }
};
