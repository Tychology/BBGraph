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


enum OutputType{none, mono, left, right};

struct StereoSample {float left; float right;};

class NodeProcessor //: public juce::ReferenceCountedObject
{
public:

    //float getOutValue() {return outValue;}

    virtual void processNextValue() {}

    std::vector<std::vector<NodeProcessor*>> inputs;

    double outValue{0.f};

    //const bool isOutputNode;
    const OutputType outputType;

    virtual ~NodeProcessor() = default;
protected:
    NodeProcessor(OutputType ot ) : outputType(ot) {}




private:


};


class ExpressionNodeProcessor : public NodeProcessor
{
public:
    ExpressionNodeProcessor(ByteCodeProcessor& p, GlobalValues& gv) : NodeProcessor(none), globalValues(gv), processor(p)
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

             outValue = processor.process(inputValues, globalValues);

        }


private:


    double inputValues[expr_node_num_ins] {0.f};
    //CounterValues& counterValues;
    GlobalValues& globalValues;

    ByteCodeProcessor& processor;
};


class OutputNodeProcessor : public NodeProcessor
{
public:
    OutputNodeProcessor(OutputType outputType) : NodeProcessor(outputType)
    {}


    void processNextValue() override
    {
	    double value = 0;

        auto& input = inputs[0];

        for (auto inputConnection : input)
        {
                value += inputConnection->outValue;
        }

        outValue = static_cast<juce::uint8>(value) / 128.f - 1.f;
    }

    StereoSample getNextStereoSample()
    {
	    double left;
        double right;

        auto& input = inputs[0];

        for (auto inputConnection : inputs[0])
        {
                left += inputConnection->outValue;
        }

        for (auto inputConnection : inputs[1])
        {
                right += inputConnection->outValue;
        }

        if (outputType == mono)
        {
            auto value = static_cast<juce::uint8>(left + right) / 128.f - 1.f;
	        return {value, value};
        }
        else
        {
            return {
	        static_cast<juce::uint8>(left) / 128.f - 1.f,
            static_cast<juce::uint8>(right) / 128.f - 1.f
            };
        }
    
	    
    }

};


class ParameterNodeProcessor : public NodeProcessor
{
public:

    ParameterNodeProcessor(juce::AudioParameterFloat& param) : NodeProcessor(none), parameter(param)
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
        globalValues.rs = 0;
        globalValues.rt = 0;
        globalValues.n = 0;
        globalValues.nf = noteFrequency;
        globalValues.t = 0;
        deltaN =  noteFrequency * 256 / sampleRate;
    }

    /*void sync(juce::AudioPlayHead::CurrentPositionInfo& positionInfo, double sampleRate)
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
    }*/

    void prepareToPlay(double sampleRate)
    {
        globalValues.fs = 0;
        globalValues.ft = 0;
        globalValues.ps = 0;
        globalValues.pt = 0;
        globalValues.rs = 0;
        globalValues.rt = 0;
        globalValues.n = 0;
        globalValues.t = 0;

        globalValues.tr = sampleRate;
        deltaT = 8000 / sampleRate;
    	deltaS = 1 / sampleRate;
    }

    void sync(bool _isPlaying, double bps, double freeSeconds, double freeSamples, double positionSeconds, double positionSamples)
    {
        isPlaying = _isPlaying;
	    globalValues.bps = bps;

        globalValues.fs = freeSeconds;
        globalValues.ft = freeSamples;
        globalValues.ps = positionSeconds;
        globalValues.pt = positionSamples;
    }


   StereoSample getNextStereoSample()
    {
	    StereoSample stereoSample{};

        for (auto p : processors)
        {
            p->processNextValue();

            switch (p->outputType)
            {
            case none: break;
            case mono:
                stereoSample.left += p->outValue;
                stereoSample.right += p->outValue;
                break;
            case left:
                stereoSample.left += p->outValue;
                break;
            case right:
                stereoSample.right += p->outValue;
                break;
            }
        }


        /*++counterValues.t;
        counterValues.h += dh;
        counterValues.n += dn;
        counterValues.bpm += dbpm;*/

        globalValues.fs += deltaS;
        globalValues.ft++;

	    if (isPlaying)
	    {
		    globalValues.ps += deltaS;
			globalValues.pt++;
	    }

        globalValues.rs += deltaS;
        globalValues.rt++;
        globalValues.n += deltaN;
        globalValues.t += deltaT;

        return stereoSample;
    }

    juce::OwnedArray<NodeProcessor> processors;
	CounterValues counterValues;
    GlobalValues globalValues;
    //juce::ReferenceCountedArray<NodeProcessor> processors;

private:



	/*double t = 0.f;
    double dh = 256.f / 48000;
    double dn = 0.f;
    double bpm = 0;
    double dbpm = 0.f;*/

    bool isPlaying;


    double deltaS;
    double deltaT;
    double deltaN;


};