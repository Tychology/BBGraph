/*
  ==============================================================================

    NodeProcessor.h
    Created: 6 May 2022 6:50:58pm
    Author:  Jonas

  ==============================================================================
*/

#pragma once
#include <JuceHeader.h>

#include "ByteCodeProcessor.h"

class NodeProcessor : public juce::ReferenceCountedObject
{
public:

    NodeProcessor()
    {

    }


    float getOutValue() {return outValue;}

    virtual void processNextValue();

    std::vector<std::vector<NodeProcessor*>> inputs;

protected:
    float outValue{0.f};

private:


};


class ExpressionNodeProcessor : public NodeProcessor
{
public:
    ExpressionNodeProcessor(ByteCodeProcessor& p) : processor(p)
    {

    }

     void processNextValue() override
        {
         const int numInputs = inputs.size();

	        for (int i = 0; i < numInputs; ++i)
	        {
                auto& input = inputs[i];

                float value = 0.f;

                for (auto inputConnection : input)
                {
                    value += inputConnection->getOutValue();
                }

                inputValues[i] = value;
            }

             outValue = processor.process(inputValues, 0);

        }

private:


    std::vector<float> inputValues;

    ByteCodeProcessor& processor;
};


class OutputNodeProcessor : public NodeProcessor
{
public:
    float getNextSample()
    {
	    float value = 0;

        auto& input = inputs[0];

        for (auto inputConnection : input)
        {
                value += inputConnection->getOutValue();
        }

	    return static_cast<juce::uint8>(value) / 128.f - 1.f ;
    }



};


class ParameterNodeProcessor : public NodeProcessor
{
public:

    ParameterNodeProcessor(juce::AudioParameterFloat& param) : parameter(param)
    {

    }

    void processNextValue() override
    {
        outValue = parameter.get();
    }

private:
    juce::AudioParameterFloat& parameter;
};



class NodeProcessorSequence
{
public:

    NodeProcessorSequence()
    {

    }

    float getNextSample()
    {
	    float sampleValue = 0.f;

        for (auto p : processors)
        {

            if (auto outputNode = dynamic_cast<OutputNodeProcessor*>(p))
            {
	            sampleValue += outputNode->getNextSample();
            }
            else
            {
	            p->processNextValue();
            }
        }

        return sampleValue;
    }

    juce::ReferenceCountedArray<NodeProcessor> processors;

private:

};