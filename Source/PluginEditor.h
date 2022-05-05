/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "GraphEditorPanel.h"
#include "InternalNodeGraph.h"
#include "PluginProcessor.h"

//==============================================================================
/**
*/
class ByteBeatNodeGraphAudioProcessorEditor  : public juce::AudioProcessorEditor
{
public:
    ByteBeatNodeGraphAudioProcessorEditor (ByteBeatNodeGraphAudioProcessor&);
    ~ByteBeatNodeGraphAudioProcessorEditor() override;

    //==============================================================================
    void paint (juce::Graphics&) override;
    void resized() override;

private:
    // This reference is provided as a quick way for your editor to
    // access the processor object that created it.
    juce::Viewport viewport;
    InternalNodeGraph& graph;
    ByteBeatNodeGraphAudioProcessor& audioProcessor;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ByteBeatNodeGraphAudioProcessorEditor)
};
