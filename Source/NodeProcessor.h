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
#include "Defines.h"

class NodeProcessor //: public juce::ReferenceCountedObject
{
public:

    //float getOutValue() {return outValue;}

    virtual void processNextValue() {}

    std::vector<std::vector<NodeProcessor*>> inputs;

    float outValue{0.f};

    const bool isOutputNode;

    ~NodeProcessor() = default;
protected:
    NodeProcessor(bool isOutputNode ) : isOutputNode(isOutputNode) {}




private:


};


class ExpressionNodeProcessor : public NodeProcessor
{
public:
    ExpressionNodeProcessor(ByteCodeProcessor& p, CounterValues& cv) : NodeProcessor(false), processor(p), counterValues(cv)
    {
    }

     void processNextValue() override
        {
         //const int numInputs = inputs.size();

	        for (int i = 0; i < expr_node_num_ins; ++i)
	        {
                auto& input = inputs[i];

                float value = 0.f;

                for (auto inputConnection : input)
                {
                    value += inputConnection->outValue;
                }

                inputValues[i] = value;
            }

             outValue = processor.process(inputValues, counterValues);

        }


private:


    float inputValues[expr_node_num_ins] {0.f};
    CounterValues& counterValues;

    ByteCodeProcessor& processor;
};


class OutputNodeProcessor : public NodeProcessor
{
public:
    OutputNodeProcessor() : NodeProcessor(true) {}


    void processnNextValue()
    {
	    float value = 0;

        auto& input = inputs[0];

        for (auto inputConnection : input)
        {
                value += inputConnection->outValue;
        }

        outValue = static_cast<juce::uint8>(value) / 128.f - 1.f;
    }



};


class ParameterNodeProcessor : public NodeProcessor
{
public:

    ParameterNodeProcessor(juce::AudioParameterFloat& param) : NodeProcessor(false), parameter(param)
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

    //Constructs empty NodeProcessorSequence
    //NodeProcessorSequence() = default;

    void setCounters(float sampleRate, float noteFrequency, float beatsPerMinute)
    {
        dh = 256.f / sampleRate;
        dn =  noteFrequency * 256.f / sampleRate;
        dbpm = beatsPerMinute / 60.f * 256.f / sampleRate;
    }


    float getNextSample()
    {
	    float sampleValue = 0.f;

        for (auto p : processors)
        {
            p->processNextValue();

            if (p->isOutputNode) sampleValue += p->outValue;
        }


        ++counterValues.t;
        counterValues.h += dh;
        counterValues.n += dn;
        counterValues.bpm += dbpm;

        return sampleValue;
    }

    juce::OwnedArray<NodeProcessor> processors;
     CounterValues counterValues;
    //juce::ReferenceCountedArray<NodeProcessor> processors;

private:



    float t = 0.f;
    float dh = 0.f;
    float dn = 0.f;
    float dbpm = 0.f;


};