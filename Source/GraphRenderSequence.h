#pragma once

#include <JuceHeader.h>

#include "InternalNodeGraph.h"

class NodeProcessorSequence;

struct  GraphRenderSequence
{
public:
	GraphRenderSequence(InternalNodeGraph& g);

	NodeProcessorSequence* createNodeProcessorSequence(juce::AudioProcessorValueTreeState& apvts);

private:
	static void getAllParentsOfNode(
		const InternalNodeGraph::Node* child,
		std::unordered_set<InternalNodeGraph::Node*>& parents,
		const std::unordered_map<InternalNodeGraph::Node*, std::unordered_set<InternalNodeGraph::Node*>>& otherParents);

	static juce::Array<InternalNodeGraph::Node*> createOrderedNodeList(const InternalNodeGraph& graph);

	std::unordered_map<juce::uint32, int> nodeIDtoIndex;

	InternalNodeGraph& graph;
	const juce::Array<InternalNodeGraph::Node*> orderedNodes;
	const int numNodes;
};
