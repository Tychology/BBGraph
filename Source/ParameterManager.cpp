#include "ParameterManager.h"

ParameterManager::ParameterManager(juce::AudioProcessorValueTreeState& apvts) : apvts(apvts)
{
	// Parameter ids start at 1 so [0] is always set
	isParameterConnected.set(0);
}

juce::String ParameterManager::newConnection()
{
	for (int i = 1; i < isParameterConnected.size(); ++i)
	{
		if (!isParameterConnected[i])
		{
			isParameterConnected.set(i);
			return juce::String(i);
		}
	}
	// Error: No more free parameters to connect
	throw std::runtime_error("No available parameter slot");
}

juce::String ParameterManager::connectToID(juce::String parameterID)
{
	const auto i = parameterID.getIntValue();
	if (i > 0 && !isParameterConnected[i])
	{
		isParameterConnected.set(i);
		return parameterID;
	}

	return newConnection();
}

void ParameterManager::removeConnection(juce::AudioParameterFloat& parameter)
{
	isParameterConnected.reset(parameter.paramID.getIntValue());
}

void ParameterManager::removeConnection(juce::String parameterID)
{
	isParameterConnected.reset(parameterID.getIntValue());
}

bool ParameterManager::existFreeParams() const
{
	return !isParameterConnected.all();
}
