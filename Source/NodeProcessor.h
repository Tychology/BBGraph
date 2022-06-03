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

    double outValue{0.f};

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

                double value = 0.f;

                for (auto inputConnection : input)
                {
                    value += inputConnection->outValue;
                }

                inputValues[i] = value;
            }

             outValue = processor.process(inputValues, counterValues);

        }


private:


    double inputValues[expr_node_num_ins] {0.f};
    CounterValues& counterValues;

    ByteCodeProcessor& processor;
};


class OutputNodeProcessor : public NodeProcessor
{
public:
    OutputNodeProcessor() : NodeProcessor(true) {}


    void processNextValue()
    {
	    double value = 0;

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
        smoothedValue.reset(10000);
    }

    void processNextValue() override
    {
        //auto value = parameter.get();

    	//smoothedValue.setTargetValue(value);
        
        //outValue = smoothedValue.getNextValue();
        //DBG(outValue);
    	outValue = parameter.get();
    }

private:
    juce::SmoothedValue<double, juce::ValueSmoothingTypes::Linear> smoothedValue;
    juce::AudioParameterFloat& parameter;
};



class NodeProcessorSequence
{
public:

    //Constructs empty NodeProcessorSequence
    //NodeProcessorSequence() = default;

    void startNote(double sampleRate, double noteFrequency)
    {
        counterValues.n = 0;
        dn =  noteFrequency * 256.f / sampleRate;
    }

    void sync(juce::AudioPlayHead::CurrentPositionInfo& positionInfo, double sampleRate)
    {
	    counterValues.t = positionInfo.timeInSamples;
        counterValues.h = positionInfo.timeInSeconds * 256.f;
        counterValues.bpm = positionInfo.timeInSeconds * 256.f * positionInfo.bpm / 60;

        dh = 256.f / sampleRate;
        dbpm = positionInfo.bpm / 60.f * 256.f / sampleRate;
    }

    void setBPM(double _bpm, double sampleRate)
    {
        bpm = _bpm;
        dbpm = _bpm / 60.f * 256.f / sampleRate;
    }

    void setTimes(double timeInSeconds, juce::int64 timeInSamples, double sampleRate)
    {

        counterValues.t = timeInSamples;
        counterValues.h = timeInSeconds * 256.f;
        counterValues.bpm = timeInSeconds * 256.f * bpm / 60;

    	dh = 256.f / sampleRate;
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



	double t = 0.f;
    double dh = 256.f / 48000;
    double dn = 0.f;
    double bpm = 0;
    double dbpm = 0.f;


};