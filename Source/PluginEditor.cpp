/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"



//==============================================================================
ByteBeatNodeGraphAudioProcessorEditor::ByteBeatNodeGraphAudioProcessorEditor (ByteBeatNodeGraphAudioProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p), graph(p.getGraph())
{
    addAndMakeVisible(viewport);
    viewport.setViewedComponent(new GraphEditorPanel(graph), true);
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
