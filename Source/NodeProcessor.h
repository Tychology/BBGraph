#pragma once

#include <JuceHeader.h>

#include "ByteCodeProcessor.h"
#include "Defines.h"


enum OutputType { none, mono, left, right };

struct StereoSample { float left; float right; };

class NodeProcessor
{
public:
	virtual void processNextValue() = 0;

	std::vector<std::vector<NodeProcessor*>> inputs;

	double outValue = 0;

	const OutputType outputType;

	virtual ~NodeProcessor() = default;

protected:
	NodeProcessor(OutputType ot) : outputType(ot) {}
};


class ExpressionNodeProcessor : public NodeProcessor
{
public:
	ExpressionNodeProcessor(ByteCodeProcessor& p, GlobalValues& gv)
		: NodeProcessor(none), globalValues(gv), processor(p)
	{
	}

	void processNextValue() override;

private:

	double inputValues[expr_node_num_ins]{ 0 };
	GlobalValues& globalValues;

	ByteCodeProcessor& processor;
};


class OutputNodeProcessor : public NodeProcessor
{
public:
	OutputNodeProcessor(OutputType outputType) : NodeProcessor(outputType)
	{
	}

	void processNextValue() override;
};


class ParameterNodeProcessor : public NodeProcessor
{
public:

	ParameterNodeProcessor(juce::AudioParameterFloat& param) : NodeProcessor(none), parameter(param)
	{
	}

	void processNextValue() override;

private:
	juce::AudioParameterFloat& parameter;
};

class NodeProcessorSequence
{
public:
	void startNote(double sampleRate, double noteFrequency);

	void prepareToPlay(double sampleRate);

	void sync(bool _isPlaying, double bps, double freeSeconds, double freeSamples, double positionSeconds, double positionSamples);

	StereoSample getNextStereoSample();

	juce::OwnedArray<NodeProcessor> processors;
	GlobalValues globalValues;

private:
	bool isPlaying;

	double deltaS;
	double deltaT;
	double deltaN;
};
