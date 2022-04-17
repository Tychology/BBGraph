/*
  ==============================================================================

    GraphEditorPanel.h
    Created: 15 Apr 2022 5:35:09pm
    Author:  Jonas

  ==============================================================================
*/

#pragma once
#include <JuceHeader.h>

#include "InternalNodeGraph.h"

class GraphEditorPanel   : public juce::Component,
                           public juce::ChangeListener
{
	GraphEditorPanel (InternalNodeGraph& g) : graph(g)
	{
		graph.addChangeListener (this);
		setOpaque (true);
	}

    ~GraphEditorPanel()
	{
		graph.removeChangeListener (this);
	    draggingConnector = nullptr;
	    nodes.clear();
	    connectors.clear();
	}

    void createNewNode();

    void setNodePosition (InternalNodeGraph::NodeID, juce::Point<double>);
	juce::Point<double> getNodePosition (InternalNodeGraph::NodeID) const;


    void paint (juce::Graphics& g)
{
    g.fillAll (getLookAndFeel().findColour (juce::ResizableWindow::backgroundColourId));
}

    void mouseDown (const juce::MouseEvent& e)
{

    if (e.mods.isPopupMenu())
        showPopupMenu (e.position.toInt());
}

    void resized()
{
    updateComponents();
}

void changeListenerCallback (juce::ChangeBroadcaster*)
{
    updateComponents();
}

    void updateComponents();

    void showPopupMenu (juce::Point<int> position);

    void beginConnectorDrag(InternalNodeGraph::NodeAndChannel source,
                             InternalNodeGraph::NodeAndChannel dest,
                             const juce::MouseEvent&);
	void dragConnector(const juce::MouseEvent&);
	void endDraggingConnector(const juce::MouseEvent&);


    InternalNodeGraph& graph;

private:
    struct NodeComponent;
    struct ConnectorComponent;
    struct PinComponent;

    NodeComponent* getComponentForNode (InternalNodeGraph::NodeID) const;
    ConnectorComponent* getComponentForConnection (const InternalNodeGraph::Connection&) const;
    PinComponent* findPinAt (juce::Point<float>) const;

	juce::OwnedArray<NodeComponent> nodes;
	juce::OwnedArray<ConnectorComponent> connectors;
    std::unique_ptr<ConnectorComponent> draggingConnector;
    std::unique_ptr<juce::PopupMenu> menu;

};
