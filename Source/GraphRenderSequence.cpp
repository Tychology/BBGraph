/*
  ==============================================================================

    GraphRenderSequence.cpp
    Created: 16 May 2022 4:17:21pm
    Author:  Jonas

  ==============================================================================
*/

#include "GraphRenderSequence.h"
#include "InternalNodeGraph.h"
#include "NodeProcessor.h"

GraphRenderSequence::GraphRenderSequence(InternalNodeGraph& g): graph(g), orderedNodes(createOrderedNodeList(graph)), numNodes(orderedNodes.size())
{
	for (int i = 0; i < numNodes; ++i)
	{
		nodeIDtoIndex.insert({orderedNodes[i]->nodeID.uid, i});
	}



}

NodeProcessorSequence* GraphRenderSequence::createNodeProcessorSequence(juce::AudioProcessorValueTreeState& apvts)
{
	auto sequence = new NodeProcessorSequence();

	for (int i = 0; i < orderedNodes.size(); ++i)
	{


		auto node = orderedNodes[i];
		if (auto exprNode = dynamic_cast<InternalNodeGraph::ExpressionNode*>(node))
		{
			auto processor = new ExpressionNodeProcessor(*exprNode->processor, sequence->counterValues);
			processor->inputs.resize(expr_node_num_ins);
			for (auto c : node->inputs)
			{

				processor->inputs[c.thisChannel].push_back(sequence->processors[nodeIDtoIndex[c.otherNode->nodeID.uid]]);
			}

			sequence->processors.add(processor);
		}
		else if (auto outputNode = dynamic_cast<InternalNodeGraph::OutputNode*>(node))
		{
			OutputType outputType;

			if (outputNode->isStereo())
			{
				auto processorL = new OutputNodeProcessor(left);
				auto processorR = new OutputNodeProcessor(right);
				processorL->inputs.resize(1);
				processorR->inputs.resize(1);

				for (auto c : node->inputs)
				{
					if (c.thisChannel == 0)
					{
						processorL->inputs[0].push_back(sequence->processors[nodeIDtoIndex[c.otherNode->nodeID.uid]]);
					}
					else
					{
						processorR->inputs[0].push_back(sequence->processors[nodeIDtoIndex[c.otherNode->nodeID.uid]]);
					}
				}
				sequence->processors.add(processorL);
				sequence->processors.add(processorR);
			}
			{
				auto processor = new OutputNodeProcessor(mono);
				processor->inputs.resize(1);


				for (auto c : node->inputs)
				{
					processor->inputs[0].push_back(sequence->processors[nodeIDtoIndex[c.otherNode->nodeID.uid]]);
				}
				sequence->processors.add(processor);
			}

			
		}
		else if (auto paramNode = dynamic_cast<InternalNodeGraph::ParameterNode*>(node))
		{
			sequence->processors.add(new ParameterNodeProcessor( *dynamic_cast<juce::AudioParameterFloat*>(apvts.getParameter(paramNode->properties["parameterID"].toString()))));
		}
		else jassertfalse;

	}


	return sequence;
}

void GraphRenderSequence::getAllParentsOfNode(const InternalNodeGraph::Node* child,
	std::unordered_set<InternalNodeGraph::Node*>& parents,
	const std::unordered_map<InternalNodeGraph::Node*, std::unordered_set<InternalNodeGraph::Node*>>& otherParents)
{
	for (auto&& i : child->inputs)
	{
		auto* parentNode = i.otherNode;

		if (parentNode == child)
			continue;

		if (parents.insert (parentNode).second)
		{
			auto parentParents = otherParents.find (parentNode);

			if (parentParents != otherParents.end())
			{
				parents.insert (parentParents->second.begin(), parentParents->second.end());
				continue;
			}

			getAllParentsOfNode (i.otherNode, parents, otherParents);
		}
	}
}

juce::Array<InternalNodeGraph::Node*> GraphRenderSequence::createOrderedNodeList(const InternalNodeGraph& graph)
{
	juce::Array<InternalNodeGraph::Node*> result;

	std::unordered_map<InternalNodeGraph::Node*, std::unordered_set<InternalNodeGraph::Node*>> nodeParents;

	for (auto* node : graph.getNodes())
	{
		int insertionIndex = 0;

		for (; insertionIndex < result.size(); ++insertionIndex)
		{
			auto& parents = nodeParents[result.getUnchecked (insertionIndex)];

			if (parents.find (node) != parents.end())
				break;
		}

		result.insert (insertionIndex, node);
		getAllParentsOfNode (node, nodeParents[node], nodeParents);
	}

	return result;
}
