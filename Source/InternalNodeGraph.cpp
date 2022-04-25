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
}

InternalNodeGraph::Node* InternalNodeGraph::getNodeForId(NodeID) const
{
	return nullptr;
}

InternalNodeGraph::Node::Ptr InternalNodeGraph::addNode(NodeType nodeType, NodeID nodeId)
{
	return {};
}

InternalNodeGraph::Node::Ptr InternalNodeGraph::removeNode(NodeID)
{
	return {};
}

InternalNodeGraph::Node::Ptr InternalNodeGraph::removeNode(Node*)
{
	return {};
}

std::vector<InternalNodeGraph::Connection> InternalNodeGraph::getConnections() const
{
	return {};
}

bool InternalNodeGraph::isConnected(const Connection&) const noexcept
{
	return false;
}

bool InternalNodeGraph::isConnected(NodeID possibleSourceNodeID, NodeID possibleDestNodeID) const noexcept
{
	return false;
}

bool InternalNodeGraph::isAnInputTo(Node& source, Node& destination) const noexcept
{
	return false;
}

bool InternalNodeGraph::canConnect(const Connection&) const
{
	return false;
}

bool InternalNodeGraph::addConnection(const Connection&)
{
	return false;
}

bool InternalNodeGraph::removeConnection(const Connection&)
{
	return false;
}

bool InternalNodeGraph::disconnectNode(NodeID)
{
	return false;
}

bool InternalNodeGraph::isConnectionLegal(const Connection&) const
{
	return false;
}

bool InternalNodeGraph::removeIllegalConnections()
{
	return false;
}

void InternalNodeGraph::handleAsyncUpdate()
{
}
