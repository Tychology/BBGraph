#include "NodeProcessor.h"

void ExpressionNodeProcessor::processNextValue()
{
	for (int i = 0; i < expr_node_num_ins; ++i)
	{
		auto& input = inputs[i];

		double value = 0;

		for (const auto inputConnection : input)
		{
			value += inputConnection->outValue;
		}

		inputValues[i] = value;
	}

	outValue = processor.process(inputValues, globalValues);
}

void OutputNodeProcessor::processNextValue()
{
	double value = 0;

	const auto& input = inputs[0];

	for (const auto inputConnection : input)
	{
		value += inputConnection->outValue;
	}

	outValue = static_cast<juce::uint8>(value) / 128.0 - 1.0;
}

void ParameterNodeProcessor::processNextValue()
{
	outValue = parameter.get();
}

void NodeProcessorSequence::startNote(double sampleRate, double noteFrequency)
{
	globalValues.rs = 0;
	globalValues.r = 0;
	globalValues.n = 0;
	globalValues.nf = noteFrequency;
	globalValues.t = 0;
	deltaN = noteFrequency * 256 / sampleRate;
}

void NodeProcessorSequence::prepareToPlay(double sampleRate)
{
	globalValues.fs = 0;
	globalValues.f = 0;
	globalValues.ps = 0;
	globalValues.p = 0;
	globalValues.rs = 0;
	globalValues.r = 0;
	globalValues.n = 0;
	globalValues.t = 0;

	globalValues.sr = sampleRate;
	deltaT = 8000 / sampleRate;
	deltaS = 1 / sampleRate;
}

void NodeProcessorSequence::sync(bool _isPlaying, double bps, double freeSeconds, double freeSamples,
	double positionSeconds, double positionSamples)
{
	isPlaying = _isPlaying;
	globalValues.bps = bps;

	globalValues.fs = freeSeconds;
	globalValues.f = freeSamples;
	globalValues.ps = positionSeconds;
	globalValues.p = positionSamples;
}

StereoSample NodeProcessorSequence::getNextStereoSample()
{
	StereoSample stereoSample{};

	for (const auto p : processors)
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

	globalValues.fs += deltaS;
	globalValues.f++;

	if (isPlaying)
	{
		globalValues.ps += deltaS;
		globalValues.p++;
	}

	globalValues.rs += deltaS;
	globalValues.r++;
	globalValues.n += deltaN;
	globalValues.t += deltaT;

	return stereoSample;
}
