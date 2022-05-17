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
}

void SynthVoice::stopNote(float velocity, bool allowTailOff)
{
}

void SynthVoice::pitchWheelMoved(int newPitchWheelValue)
{
}

void SynthVoice::controllerMoved(int controllerNumber, int newControllerValue)
{
}

void SynthVoice::renderNextBlock(juce::AudioBuffer<float>& outputBuffer, int startSample, int numSamples)
{
    if (processorSequence == nullptr) return;

    auto* channelZero = outputBuffer.getWritePointer (0);

    auto end = startSample + numSamples;


    for (int i = startSample; i < end; ++i)
    {
	    channelZero[i] += processorSequence->getNextSample();
    }

}
