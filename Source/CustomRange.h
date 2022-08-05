#pragma once

#include <JuceHeader.h>

//https://forum.juce.com/t/why-no-logarithmic-slider/43840
template<typename type>
static juce::NormalisableRange<type> logRange (type min, type max)
{
    return { min, max,
	    [=](type min, type max, type v) { return std::exp2 (v * std::log2 (max / min)) * min; },
	    [=](type min, type max, type v) { return std::log2 (v / min) / std::log2 (max / min); },
	    [](type min, type max, type v) { return  v < min ? min : v > max ? max : v; }
    };
}
