/*
  ==============================================================================

    SynthVoice.cpp
    Created: 6 May 2022 6:45:58pm
    Author:  Jonas

  ==============================================================================
*/

#include "SynthVoice.h"

bool SynthVoice::canPlaySound(juce::SynthesiserSound*)
{
    return true;
}

void SynthVoice::startNote(int midiNoteNumber, float velocity, juce::SynthesiserSound* sound,
	int currentPitchWheelPosition)
{
    if (processorSequence == nullptr) return;

    processorSequence->startNote(getSampleRate(), juce::MidiMessage::getMidiNoteInHertz(midiNoteNumber));
     adsr.noteOn();
}

void SynthVoice::stopNote(float velocity, bool allowTailOff)
{
    adsr.noteOff();

    if (!allowTailOff || ! adsr.isActive())
    {
	    clearCurrentNote();
    }
}

void SynthVoice::pitchWheelMoved(int newPitchWheelValue)
{
}

void SynthVoice::controllerMoved(int controllerNumber, int newControllerValue)
{
}

void SynthVoice::renderNextBlock(juce::AudioBuffer<float>& outputBuffer, int startSample, int numSamples)
{
    if (!isVoiceActive()) return;
    if (processorSequence == nullptr) return;


    //auto* left = outputBuffer.getWritePointer (0);
    auto channels = outputBuffer.getArrayOfWritePointers();

    auto end = startSample + numSamples;



    for (int i = startSample; i < end; ++i)
    {
        auto stereoSample = processorSequence->getNextStereoSample();

	    channels[0][i] += stereoSample.left;
        channels[1][i] += stereoSample.right;
    }

    adsr.applyEnvelopeToBuffer(outputBuffer, startSample,numSamples);

	if (!adsr.isActive())
	{
		clearCurrentNote();
	}
}

void SynthVoice::prepareToPlay(double sampleRate, int samplesPerBlock, int outputChannels)
{
	juce::ADSR::Parameters adsrParams;
	adsrParams.attack = 0.f;
	adsrParams.decay = 0.f;
	adsrParams.sustain = 1.f;
	adsrParams.release = 0.f;

    adsr.setSampleRate(sampleRate);
	adsr.setParameters(adsrParams);

    if (processorSequence != nullptr) processorSequence->prepareToPlay(sampleRate);
}

void SynthVoice::setProcessorSequence(NodeProcessorSequence* sequence)
{
	processorSequence = std::unique_ptr<NodeProcessorSequence>(sequence);
}

void SynthVoice::update(juce::ADSR::Parameters parameters, double bps, double freeSeconds, double freeSamples,
                        double positionSeconds, double positionSamples)
{
	adsr.setParameters(parameters);
	if (processorSequence != nullptr) processorSequence->sync(bps, freeSeconds, freeSamples, positionSeconds, positionSamples);
}


