/*
  ==============================================================================

    ParameterManager.h
    Created: 9 May 2022 3:52:44pm
    Author:  Jonas

  ==============================================================================
*/

#pragma once
#include <bitset>
#include <JuceHeader.h>
#include "Defines.h"


class ParameterManager //: public juce::ReferenceCountedObject
{
public:
    ParameterManager (juce::AudioProcessorValueTreeState& apvts) : apvts(apvts)
    {
        isParameterConnected.set(0); //parameter ids start at 1 so [0] is always set
    }
    /*
    juce::AudioParameterFloat& newConnection()
    {

	    for (int i = 1; i < isParameterConnected.size(); ++i)
	    {
            if (!isParameterConnected[i])
            {
                if (auto* param = dynamic_cast<juce::AudioParameterFloat*>(apvts.getParameter(juce::String(i))))
                {
					isParameterConnected.set(i);
                	return *param;
                }
            }
	    }
        jassertfalse; //No more free parameters to connect
    }*/

    juce::String newConnection()
    {
	    for (int i = 1; i < isParameterConnected.size(); ++i)
	    {
            if (!isParameterConnected[i])
            {
					isParameterConnected.set(i);
                	return juce::String(i);
            }
	    }
        jassertfalse; //No more free parameters to connect
    }

    juce::String connectToID (juce::String parameterID)
    {
        auto i = parameterID.getIntValue();
        if(i > 0 && !isParameterConnected[i])
        {
	        isParameterConnected.set(i);
            return parameterID;
        }

    	return newConnection();
    }

    void removeConection(juce::AudioParameterFloat& parameter)
    {
        isParameterConnected.reset(parameter.getParameterID().getIntValue());
    }

    void removeConnection(juce::String parameterID)
    {
	    isParameterConnected.reset(parameterID.getIntValue());
    }


    bool existFreeParams()
    {
	    return !isParameterConnected.all();
    }

private:
    juce::AudioProcessorValueTreeState& apvts;
    //std::vector<bool> isParameterConnected;
    std::bitset<total_num_prams + 1> isParameterConnected;
};