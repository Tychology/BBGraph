/*
  ==============================================================================

    InternalNodeGraph.cpp
    Created: 15 Apr 2022 5:50:17pm
    Author:  Jonas

  ==============================================================================
*/

#include "InternalNodeGraph.h"

InternalNodeGraph::InternalNodeGraph()
{
}

InternalNodeGraph::~InternalNodeGraph()
{
}


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
    // const juce::ScopedLock sl (getCallbackLock());

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
		n = new ParameterNode(nodeID);
		break;
	default: break;
	}


    {
        //const juce::ScopedLock sl ();
        nodes.add (n.get());
    }

    //n->setParentGraph (this);
    topologyChanged();
    return n;


}

InternalNodeGraph::Node::Ptr InternalNodeGraph::removeNode(NodeID nodeID)
{
	//const ScopedLock sl (getCallbackLock());

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
	return false;
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

//IMPLEMENT
  bool InternalNodeGraph::loopCheck(Node* src, Node* dest) const noexcept
{
    return false;
}

void InternalNodeGraph::topologyChanged()
{
     sendChangeMessage();
}

void InternalNodeGraph::handleAsyncUpdate()
{
}


