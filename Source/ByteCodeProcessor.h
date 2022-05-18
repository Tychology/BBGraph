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

    float process(float* inpuValues, CounterValues& counterValues ) {return 0;};
};