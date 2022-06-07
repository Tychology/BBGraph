/*
  ==============================================================================

    InternalNodeGraph.cpp
    Created: 15 Apr 2022 5:50:17pm
    Author:  Jonas

  ==============================================================================
*/

#include "InternalNodeGraph.h"
#include "PluginProcessor.h"
#include "GraphRenderSequence.h"







InternalNodeGraph::InternalNodeGraph(ByteBeatNodeGraphAudioProcessor& p, ParameterManager& paramManager) : audioProcessor(p), parameterManager(paramManager)
{

}

InternalNodeGraph::~InternalNodeGraph()
= default;


void InternalNodeGraph::Node::setParentGraph(InternalNodeGraph* const graph) const
{
}

InternalNodeGraph::Connection::Connection (NodeAndChannel src, NodeAndChannel dst) noexcept
    : source (src), destination (dst)
{
}

bool InternalNodeGraph::Connection::operator== (const Connection& other) const noexcept
{
    return source == other.source && destination == other.destination;
}

bool InternalNodeGraph::Connection::operator!= (const Connection& c) const noexcept
{
    return ! operator== (c);
}

bool InternalNodeGraph::Connection::operator< (const Connection& other) const noexcept
{
    if (source.nodeID != other.source.nodeID)
        return source.nodeID < other.source.nodeID;

    if (destination.nodeID != other.destination.nodeID)
        return destination.nodeID < other.destination.nodeID;

    if (source.channelIndex != other.source.channelIndex)
        return source.channelIndex < other.source.channelIndex;

    return destination.channelIndex < other.destination.channelIndex;
}





void InternalNodeGraph::clear()
{
     const juce::ScopedLock sl (audioProcessor.getCallbackLock());

    if (nodes.isEmpty())
        return;

    nodes.clear();
    topologyChanged();
}

InternalNodeGraph::Node* InternalNodeGraph::getNodeForId(NodeID nodeID) const
{
	 for (auto* n : nodes)
        if (n->nodeID == nodeID)
            return n;

    return {};
}

InternalNodeGraph::Node::Ptr InternalNodeGraph::addNode(NodeType nodeType, NodeID nodeID)
{

	if (nodeID == NodeID())
        nodeID.uid = ++(lastNodeID.uid);

	for (auto* n : nodes)
    {
        if (n->nodeID == nodeID)
        {
            jassertfalse; // Cannot add two copies of duplicate node IDs!
            return {};
        }
    }

    if (lastNodeID < nodeID)
        lastNodeID = nodeID;

    Node::Ptr n;

	switch (nodeType)
	{
	case NodeType::Expression:
		n = new ExpressionNode(nodeID);
		break;

	case NodeType::Output:
		n = new OutputNode(nodeID);
		break;

	case NodeType::Parameter:
        if (parameterManager.existFreeParams()) //should give feedback to the user if there are no free parameters
        	n = new ParameterNode(nodeID, parameterManager);
        else return n;
		break;

	case NodeType::Void: return n;
	//default: break;
	}


    {
        const juce::ScopedLock sl ();
        nodes.add (n.get());
    }

    //n->setParentGraph (this);
    topologyChanged();
    return n;


}

InternalNodeGraph::Node::Ptr InternalNodeGraph::removeNode(NodeID nodeID)
{
	const juce::ScopedLock sl (audioProcessor.getCallbackLock());

    for (int i = nodes.size(); --i >= 0;)
    {
        if (nodes.getUnchecked (i)->nodeID == nodeID)
        {
            disconnectNode (nodeID);
            auto node = nodes.removeAndReturn (i);
            topologyChanged();
            return node;
        }
    }

    return {};
}

InternalNodeGraph::Node::Ptr InternalNodeGraph::removeNode(Node* node)
{
	 if (node != nullptr)
        return removeNode (node->nodeID);

    jassertfalse;
    return {};
}


void InternalNodeGraph::getNodeConnections(Node& node, std::vector<Connection>& connections)
{
     for (auto& i : node.inputs)
        connections.push_back ({ { i.otherNode->nodeID, i.otherChannel }, { node.nodeID, i.thisChannel } });

    for (auto& o : node.outputs)
        connections.push_back ({ { node.nodeID, o.thisChannel }, { o.otherNode->nodeID, o.otherChannel } });
}


std::vector<InternalNodeGraph::Connection> InternalNodeGraph::getConnections() const
{
	std::vector<Connection> connections;

    for (auto& n : nodes)
        getNodeConnections (*n, connections);

    std::sort (connections.begin(), connections.end());
    auto last = std::unique (connections.begin(), connections.end());
    connections.erase (last, connections.end());

    return connections;
}

bool InternalNodeGraph::isConnected (Node* src, int sourceChannel, Node* dest, int destChannel) const noexcept
{
    for (auto& o : src->outputs)
        if (o.otherNode == dest && o.thisChannel == sourceChannel && o.otherChannel == destChannel)
            return true;

    return false;
}

bool InternalNodeGraph::isConnected (const Connection& c) const noexcept
{
    if (auto* source = getNodeForId (c.source.nodeID))
        if (auto* dest = getNodeForId (c.destination.nodeID))
            return isConnected (source, c.source.channelIndex,
                                dest, c.destination.channelIndex);

    return false;
}

bool InternalNodeGraph::isConnected (NodeID srcID, NodeID destID) const noexcept
{
    if (auto* source = getNodeForId (srcID))
        if (auto* dest = getNodeForId (destID))
            for (auto& out : source->outputs)
                if (out.otherNode == dest)
                    return true;

    return false;
}


bool InternalNodeGraph::isAnInputTo(Node& source, Node& destination) const noexcept
{
	jassert (nodes.contains (&source));
    jassert (nodes.contains (&destination));

    return isAnInputTo (source, destination, nodes.size());
}

bool InternalNodeGraph::isAnInputTo (Node& src, Node& dst, int recursionCheck) const noexcept
{
    for (auto&& i : dst.inputs)
        if (i.otherNode == &src)
            return true;

    if (recursionCheck > 0)
        for (auto&& i : dst.inputs)
            if (isAnInputTo (src, *i.otherNode, recursionCheck - 1))
                return true;

    return false;
}


bool InternalNodeGraph::canConnect (Node* src, int sourceChannel, Node* dest, int destChannel) const noexcept
{

    if (sourceChannel < 0
         || destChannel < 0
         || src == dest)
        return false;

    if (src == nullptr
         || sourceChannel >= src->getNumOutputs())
        return false;

    if (dest == nullptr
         || destChannel >= dest->getNumInputs())
        return false;

    if (loopCheck(src, dest)) return false;

    return ! isConnected (src, sourceChannel, dest, destChannel);
}

bool InternalNodeGraph::canConnect(const Connection& c) const
{
	if (auto* source = getNodeForId (c.source.nodeID))
        if (auto* dest = getNodeForId (c.destination.nodeID))
            return canConnect (source, c.source.channelIndex,
                               dest, c.destination.channelIndex);

    return false;
}

bool InternalNodeGraph::addConnection(const Connection& c)
{
	 if (auto* source = getNodeForId (c.source.nodeID))
    {
        if (auto* dest = getNodeForId (c.destination.nodeID))
        {
            auto sourceChan = c.source.channelIndex;
            auto destChan = c.destination.channelIndex;

            if (canConnect (source, sourceChan, dest, destChan))
            {
                source->outputs.add ({ dest, destChan, sourceChan });
                dest->inputs.add ({ source, sourceChan, destChan });
                jassert (isConnected (c));
                topologyChanged();
                return true;
            }
        }
    }

    return false;
}

bool InternalNodeGraph::removeConnection(const Connection& c)
{
	if (auto* source = getNodeForId (c.source.nodeID))
    {
        if (auto* dest = getNodeForId (c.destination.nodeID))
        {
            auto sourceChan = c.source.channelIndex;
            auto destChan = c.destination.channelIndex;

            if (isConnected (source, sourceChan, dest, destChan))
            {
                source->outputs.removeAllInstancesOf ({ dest, destChan, sourceChan });
                dest->inputs.removeAllInstancesOf ({ source, sourceChan, destChan });
                topologyChanged();
                return true;
            }
        }
    }

    return false;
}

bool InternalNodeGraph::disconnectNode(NodeID nodeID)
{
	if (auto* node = getNodeForId (nodeID))
    {
        std::vector<Connection> connections;
        getNodeConnections (*node, connections);

        if (! connections.empty())
        {
            for (auto c : connections)
                removeConnection (c);

            return true;
        }
    }

    return false;
}


//IMPLEMENT
bool InternalNodeGraph::isLegal (Node* src, int sourceChannel, Node* dest, int destChannel) const noexcept
{
	return !loopCheck(src, dest);
}

bool InternalNodeGraph::isConnectionLegal(const Connection& c) const
{
	if (auto* source = getNodeForId (c.source.nodeID))
        if (auto* dest = getNodeForId (c.destination.nodeID))
            return isLegal (source, c.source.channelIndex, dest, c.destination.channelIndex);

    return false;
}

bool InternalNodeGraph::removeIllegalConnections()
{
	bool anyRemoved = false;

    for (auto* node : nodes)
    {
        std::vector<Connection> connections;
        getNodeConnections (*node, connections);

        for (auto c : connections)
            if (! isConnectionLegal (c))
                anyRemoved = removeConnection (c) || anyRemoved;
    }

    return anyRemoved;
}



juce::ValueTree InternalNodeGraph::toValueTree() const
{
	juce::ValueTree graphTree("graph");
	juce::ValueTree nodesTree("nodes");
	juce::ValueTree connectionsTree("connections");


	for (auto* node : nodes)
	{

		juce::ValueTree n ("node");

        for (auto property : node->properties)
        {
	        n.setProperty(property.name, property.value, nullptr);
        }

		n.setProperty("uid", (int)node->nodeID.uid, nullptr);
        /*
		n.setProperty("x", node->properties ["x"], nullptr);
		n.setProperty("y", node->properties ["y"], nullptr);

		if (auto* expNode = dynamic_cast<InternalNodeGraph::ExpressionNode*>(node))
		{
			n.setProperty("type", NodeType::Expression, nullptr);
			n.setProperty("expression", node->properties.getWithDefault("expression", "" ), nullptr);
		}
		else if (auto* outNode = dynamic_cast<InternalNodeGraph::OutputNode*>(node))
		{
			n.setProperty("type", NodeType::Output, nullptr);
		}
		else if (auto* paramNode = dynamic_cast<InternalNodeGraph::ParameterNode*>(node))
		{
			n.setProperty("type", NodeType::Parameter, nullptr);
			n.setProperty("parameterID", node->properties["parameterID"], nullptr);
            n.setProperty("log", node->properties["log"], nullptr);
            n.setProperty("start", node->properties["start"], nullptr);
            n.setProperty("end", node->properties["end"], nullptr);
		}
		else
		{
			jassertfalse;
			continue;
		}*/

		nodesTree.addChild(n, -1, nullptr);

	}


	for (auto& connection : getConnections())
	{
		juce::ValueTree c("connection");

		c.setProperty("srcID", juce::var((int) connection.source.nodeID.uid), nullptr);
		c.setProperty("srcChannel", juce::var(connection.source.channelIndex), nullptr);
		c.setProperty("destID", juce::var((int) connection.destination.nodeID.uid), nullptr);
		c.setProperty("destChannel", juce::var(connection.destination.channelIndex), nullptr);

		connectionsTree.addChild(c, -1, nullptr);
	}



	graphTree.addChild(nodesTree, 0, nullptr);
	graphTree.addChild(connectionsTree, 1, nullptr);

	return graphTree;
}


void InternalNodeGraph::restoreFromTree(const juce::ValueTree& graphTree)
{
	clear();

	juce::ValueTree nodesTree = graphTree.getChildWithName("nodes");
	juce::ValueTree connectionsTree = graphTree.getChildWithName("connections");


	if (nodesTree.isValid())
	{
		auto numNodes = nodesTree.getNumChildren();

		for (int i = 0; i < numNodes; ++i)
		{
			juce::ValueTree n = nodesTree.getChild(i);

			auto type = static_cast<NodeType>(static_cast<int>(n.getProperty("type")));
			if (type == NodeType::Void) break;

			auto nodeID = NodeID(static_cast<int>(n.getProperty("uid")));

			auto node = addNode(type, nodeID);

			for (int i = 0; i < n.getNumProperties(); ++i)
			{
                auto propertyName = n.getPropertyName(i);

				node->properties.set(propertyName, n.getProperty(propertyName));
			}

            node->update();
            /*
			node->properties.set("x", n.getProperty("x"));
			node->properties.set("y", n.getProperty("y"));


			switch (type)
			{
			case Void: break;
			case Expression:
				node->properties.set("expression", n.getProperty("expression"));
				break;
			case Output: break;
			case Parameter:
				node->properties.set("parameterID", n.getProperty("parameterID"));
                node->properties.set("log", n.getProperty("log"));
                node->properties.set("start", n.getProperty("start"));
                node->properties.set("end", n.getProperty("end"));
				break;
			}
			*/
		}
	}


	if (connectionsTree.isValid())
	{
		auto numConnections = connectionsTree.getNumChildren();

		for (int i = 0; i < numConnections; ++i)
		{
			auto c = connectionsTree.getChild(i);

			addConnection({
				{NodeID((int)c.getProperty("srcID")), c.getProperty("srcChannel")},
				{NodeID((int)c.getProperty("destID")), c.getProperty("destChannel")}
			});
		}
	}


	removeIllegalConnections();
	topologyChanged();
}



  bool InternalNodeGraph::loopCheck(Node* src, Node* dest) const noexcept
{
	return  dest->feedsInto(src->nodeID);
}

void InternalNodeGraph::topologyChanged()
{
     sendChangeMessage();

    //if (isPrepared)
    {
    	if (juce::MessageManager::getInstance()->isThisTheMessageThread())
			handleAsyncUpdate();
		else
			triggerAsyncUpdate();
    }

}

void InternalNodeGraph::handleAsyncUpdate()
{
     buildRenderingSequence();
}

//void InternalNodeGraph::clearRenderingSequence()
//{
//    auto oldSequence = std::make_unique<GraphRenderSequence>(*this);
//
//	const juce::ScopedLock sl (audioProcessor.getCallbackLock());
//	std::swap (renderSequence, oldSequence);
//
//
//}

void InternalNodeGraph::buildRenderingSequence()
{
    auto newSequence = std::make_unique<GraphRenderSequence>(*this);

    isPrepared = true;

    std::swap(renderSequence, newSequence);


    audioProcessor.setNodeProcessorSequence(*renderSequence);

}


