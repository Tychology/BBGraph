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
