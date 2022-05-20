/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"



//==============================================================================
ByteBeatNodeGraphAudioProcessorEditor::ByteBeatNodeGraphAudioProcessorEditor (ByteBeatNodeGraphAudioProcessor& p)
    : AudioProcessorEditor (&p), graph(p.graph), audioProcessor(p), topBar(p)
{
    addAndMakeVisible(viewport);
    viewport.setViewedComponent(new GraphEditorPanel(p.apvts, graph), true);
    viewport.getViewedComponent()->setSize(4000, 4000);

    addAndMakeVisible(topBar);

    setResizable(true, true);


    //setResizeLimits(600, 400, viewport.getViewedComponent()->getWidth(), viewport.getViewedComponent()->getHeight());
    getConstrainer()->setSizeLimits(600, 400, viewport.getViewedComponent()->getWidth(), viewport.getViewedComponent()->getHeight());

    int width = p.apvts.state.getProperty("windowWidth");
    int height = p.apvts.state.getProperty("windowHeight");

    //juce::Desktop::getInstance().setGlobalScaleFactor(1.5);

    setBoundsConstrained(juce::Rectangle<int>(getX(), getY(), width, height));
    //setSize (600, 400);
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
    auto bounds = getLocalBounds();
    topBar.setBounds(bounds.removeFromTop(50));

    viewport.setBounds(bounds);
    audioProcessor.apvts.state.setProperty("windowWidth", getWidth(), nullptr);
    audioProcessor.apvts.state.setProperty("windowHeight", getHeight(), nullptr);

}
