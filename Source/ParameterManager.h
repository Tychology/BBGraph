#pragma once

#include <bitset>
#include <JuceHeader.h>

#include "Defines.h"

class ParameterManager
{
public:
	ParameterManager(juce::AudioProcessorValueTreeState& apvts);

	juce::String newConnection();

	juce::String connectToID(juce::String parameterID);

	void removeConnection(juce::AudioParameterFloat& parameter);

	void removeConnection(juce::String parameterID);

	bool existFreeParams() const;

private:
	juce::AudioProcessorValueTreeState& apvts;
	std::bitset<total_num_params + 1> isParameterConnected;
};