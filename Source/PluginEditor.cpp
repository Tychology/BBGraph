/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"



//==============================================================================
ByteBeatNodeGraphAudioProcessorEditor::ByteBeatNodeGraphAudioProcessorEditor (ByteBeatNodeGraphAudioProcessor& p)
    : AudioProcessorEditor (&p), graph(p.getGraph()), audioProcessor(p)
{
    addAndMakeVisible(viewport);
    viewport.setViewedComponent(new GraphEditorPanel(p.getApvts(), p.getGraph()), true);
    viewport.getViewedComponent()->setSize(1000, 1000);

    setSize (600, 400);
}

ByteBeatNodeGraphAudioProcessorEditor::~ByteBeatNodeGraphAudioProcessorEditor()
{
}

//==============================================================================
void ByteBeatNodeGraphAudioProcessorEditor::paint (juce::Graphics& g)
{

}

void ByteBeatNodeGraphAudioProcessorEditor::resized()
{
    viewport.setBounds(getLocalBounds());

}
