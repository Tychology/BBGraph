/*
  ==============================================================================

    ByteCodeProcessor.h
    Created: 25 Apr 2022 6:31:56pm
    Author:  Jonas

  ==============================================================================
*/

#pragma once
#include <JuceHeader.h>

struct CounterValues
{
	float t = 0.f;
    float h = 0.f;
    float n = 0.f;
    float bpm = 0.f;
};

class ByteCodeProcessor
{

public:

    float process(float* inputValues, CounterValues& counterValues )
    {

        //DBG(inputValues[0]);
	    //return (std::sin(  counterValues.n / 256 * juce::MathConstants<float>::twoPi) + 1) * inpuValues[0];
        return (static_cast<int>(counterValues.n * inputValues[0]) & 255);
        //return inputValues[0];
    }
};