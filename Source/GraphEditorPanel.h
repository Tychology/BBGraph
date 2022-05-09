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

#include "LookAndFeelDemo.h"


struct LookAndFeel : juce::LookAndFeel_V4
{
	void drawRotarySlider(juce::Graphics&, int x, int y, int width, int height, float sliderPosProportional, float rotaryStartAngle, float rotaryEndAngle, juce::Slider&) override;
};



class GraphEditorPanel   : public juce::Component,
                           public juce::ChangeListener
{
    struct PinComponent;
    struct NodeComponent;
    struct ConnectorComponent;


    struct ExpressionNodeComponent;
    struct OutputNodeComponent;
    struct ParameterNodeComponent;

public:
	GraphEditorPanel (juce::AudioProcessorValueTreeState& apvts, InternalNodeGraph& g);

    ~GraphEditorPanel() override;

    void createNewNode(NodeType nodeType, juce::Point<int> position);

    void setNodePosition (InternalNodeGraph::NodeID, juce::Point<double>);
	juce::Point<double> getNodePosition (InternalNodeGraph::NodeID) const;


    void paint (juce::Graphics& g) override
    {
    g.fillAll (getLookAndFeel().findColour (juce::ResizableWindow::backgroundColourId));
}

    void mouseDown (const juce::MouseEvent& e) override
    {
    	unfocusAllComponents();
    if (e.mods.isPopupMenu())
        showPopupMenu (e.position.toInt());
}

    void resized() override
    {
    updateComponents();
	}

	void changeListenerCallback (juce::ChangeBroadcaster*) override
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

    juce::AudioProcessorValueTreeState& apvts;


    NodeComponent* getComponentForNode (InternalNodeGraph::NodeID) const;
    ConnectorComponent* getComponentForConnection (const InternalNodeGraph::Connection&) const;
    PinComponent* findPinAt (juce::Point<float>) const;

	juce::OwnedArray<NodeComponent> nodes;
	juce::OwnedArray<ConnectorComponent> connectors;
    std::unique_ptr<ConnectorComponent> draggingConnector;
    std::unique_ptr<juce::PopupMenu> menu;

};
