#include "InternalNodeGraph.h"

#include "PluginProcessor.h"
#include "GraphRenderSequence.h"
#include "Defines.h"

#pragma region Nodes

bool InternalNodeGraph::Node::feedsInto(NodeID id)
{
	for (const auto c : outputs)
	{
		if (c.otherNode->nodeID == id) return true;

		if (c.otherNode->feedsInto(id)) return true;
	}
	
	return false;
}

InternalNodeGraph::ExpressionNode::ExpressionNode(NodeID n) : Node(n, expr_node_num_ins, 1)
{
	properties.set("type", NodeType::Expression);
	processor = std::make_unique<ByteCodeProcessor>();
}

void InternalNodeGraph::ExpressionNode::update()
{
	const auto valid = processor->update(properties.getWithDefault("expression", "").toString());
	properties.set("validExpression", valid);
}

InternalNodeGraph::OutputNode::OutputNode(NodeID n) : Node(n, 2, 0)
{
	properties.set("type", NodeType::Output);
}

bool InternalNodeGraph::OutputNode::isStereo()
{
	return std::any_of(inputs.begin(), inputs.end(), [](Connection c) {return c.thisChannel == 0; }) &&
		std::any_of(inputs.begin(), inputs.end(), [](Connection c) {return c.thisChannel == 1; });
}

InternalNodeGraph::ParameterNode::ParameterNode(NodeID n, ParameterManager& paramManager) : Node(n, 0, 1), parameterManager(paramManager)
{
	properties.set("type", NodeType::Parameter);
	properties.set("parameterID", paramManager.connectToID(properties["parameterID"]));
}

InternalNodeGraph::ParameterNode::~ParameterNode()
{
	parameterManager.removeConnection(properties["parameterID"]);
}

#pragma endregion

#pragma region Connection

InternalNodeGraph::Connection::Connection(NodeAndChannel src, NodeAndChannel dst) noexcept
	: source(src), destination(dst)
{
}

bool InternalNodeGraph::Connection::operator== (const Connection& other) const noexcept
{
	return source == other.source && destination == other.destination;
}

bool InternalNodeGraph::Connection::operator!= (const Connection& c) const noexcept
{
	return !operator== (c);
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

#pragma endregion

#pragma region Graph

InternalNodeGraph::InternalNodeGraph(ByteBeatNodeGraphAudioProcessor& p, ParameterManager& paramManager) : audioProcessor(p), parameterManager(paramManager)
{}

// This has to be here because GraphRenderSequence is not a complete type in the header.
InternalNodeGraph::~InternalNodeGraph()
{}

void InternalNodeGraph::clear()
{
	const juce::ScopedLock sl(audioProcessor.getCallbackLock());

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

InternalNodeGraph::Node::Ptr InternalNodeGraph::addNode(NodeType nodeType, NodeID nodeID, bool quiet)
{
	if (nodeID == NodeID())
		nodeID.uid = ++(lastNodeID.uid);

	for (const auto* n : nodes)
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
		if (parameterManager.existFreeParams())
		{
			// Should give feedback to the user if there are no free parameters
			n = new ParameterNode(nodeID, parameterManager);
		}
		else
		{
			juce::AlertWindow::showMessageBoxAsync(juce::MessageBoxIconType::WarningIcon, "Error", "No available parameter slot", "OK");
			return n;
		}
		break;

	case NodeType::Void: return n;
		//default: break;
	}

	{
		const juce::ScopedLock sl();
		nodes.add(n.get());
	}

	if (!quiet) topologyChanged();
	return n;
}

InternalNodeGraph::Node::Ptr InternalNodeGraph::removeNode(NodeID nodeID)
{
	const juce::ScopedLock sl(audioProcessor.getCallbackLock());

	for (int i = nodes.size(); --i >= 0;)
	{
		if (nodes.getUnchecked(i)->nodeID == nodeID)
		{
			disconnectNode(nodeID);
			auto node = nodes.removeAndReturn(i);
			topologyChanged();
			return node;
		}
	}

	return {};
}

InternalNodeGraph::Node::Ptr InternalNodeGraph::removeNode(Node* node)
{
	if (node != nullptr)
		return removeNode(node->nodeID);

	jassertfalse;
	return {};
}

void InternalNodeGraph::getNodeConnections(Node& node, std::vector<Connection>& connections)
{
	for (const auto& i : node.inputs)
		connections.push_back({ { i.otherNode->nodeID, i.otherChannel }, { node.nodeID, i.thisChannel } });

	for (const auto& o : node.outputs)
		connections.push_back({ { node.nodeID, o.thisChannel }, { o.otherNode->nodeID, o.otherChannel } });
}


std::vector<InternalNodeGraph::Connection> InternalNodeGraph::getConnections() const
{
	std::vector<Connection> connections;

	for (auto& n : nodes)
		getNodeConnections(*n, connections);

	std::sort(connections.begin(), connections.end());
	const auto last = std::unique(connections.begin(), connections.end());
	connections.erase(last, connections.end());

	return connections;
}

bool InternalNodeGraph::isConnected(Node* src, int sourceChannel, Node* dest, int destChannel) const noexcept
{
	for (const auto& o : src->outputs)
		if (o.otherNode == dest && o.thisChannel == sourceChannel && o.otherChannel == destChannel)
			return true;

	return false;
}

bool InternalNodeGraph::isConnected(const Connection& c) const noexcept
{
	if (auto* source = getNodeForId(c.source.nodeID))
		if (auto* dest = getNodeForId(c.destination.nodeID))
			return isConnected(source, c.source.channelIndex,
				dest, c.destination.channelIndex);

	return false;
}

bool InternalNodeGraph::isConnected(NodeID srcID, NodeID destID) const noexcept
{
	if (auto* source = getNodeForId(srcID))
		if (const auto* dest = getNodeForId(destID))
			for (const auto& out : source->outputs)
				if (out.otherNode == dest)
					return true;

	return false;
}


bool InternalNodeGraph::isAnInputTo(Node& source, Node& destination) const noexcept
{
	jassert(nodes.contains(&source));
	jassert(nodes.contains(&destination));

	return isAnInputTo(source, destination, nodes.size());
}

bool InternalNodeGraph::isAnInputTo(Node& src, Node& dst, int recursionCheck) const noexcept
{
	for (auto&& i : dst.inputs)
		if (i.otherNode == &src)
			return true;

	if (recursionCheck > 0)
		for (auto&& i : dst.inputs)
			if (isAnInputTo(src, *i.otherNode, recursionCheck - 1))
				return true;

	return false;
}

bool InternalNodeGraph::canConnect(Node* src, int sourceChannel, Node* dest, int destChannel) const noexcept
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

	return !isConnected(src, sourceChannel, dest, destChannel);
}

bool InternalNodeGraph::canConnect(const Connection& c) const
{
	if (auto* source = getNodeForId(c.source.nodeID))
		if (auto* dest = getNodeForId(c.destination.nodeID))
			return canConnect(source, c.source.channelIndex,
				dest, c.destination.channelIndex);

	return false;
}

bool InternalNodeGraph::addConnection(const Connection& c, bool quiet)
{
	if (auto* source = getNodeForId(c.source.nodeID))
	{
		if (auto* dest = getNodeForId(c.destination.nodeID))
		{
			const auto sourceChan = c.source.channelIndex;
			const auto destChan = c.destination.channelIndex;

			if (canConnect(source, sourceChan, dest, destChan))
			{
				source->outputs.add({ dest, destChan, sourceChan });
				dest->inputs.add({ source, sourceChan, destChan });
				jassert(isConnected(c));
				if (!quiet) topologyChanged();
				return true;
			}
		}
	}

	return false;
}

bool InternalNodeGraph::removeConnection(const Connection& c)
{
	if (auto* source = getNodeForId(c.source.nodeID))
	{
		if (auto* dest = getNodeForId(c.destination.nodeID))
		{
			const auto sourceChan = c.source.channelIndex;
			const auto destChan = c.destination.channelIndex;

			if (isConnected(source, sourceChan, dest, destChan))
			{
				source->outputs.removeAllInstancesOf({ dest, destChan, sourceChan });
				dest->inputs.removeAllInstancesOf({ source, sourceChan, destChan });
				topologyChanged();
				return true;
			}
		}
	}

	return false;
}

bool InternalNodeGraph::disconnectNode(NodeID nodeID)
{
	if (auto* node = getNodeForId(nodeID))
	{
		std::vector<Connection> connections;
		getNodeConnections(*node, connections);

		if (!connections.empty())
		{
			for (auto c : connections)
				removeConnection(c);

			return true;
		}
	}

	return false;
}

bool InternalNodeGraph::isConnectionLegal(const Connection& c) const
{
	if (auto* source = getNodeForId(c.source.nodeID))
		if (auto* dest = getNodeForId(c.destination.nodeID))
			return !loopCheck(source, dest);

	return false;
}

bool InternalNodeGraph::removeIllegalConnections()
{
	bool anyRemoved = false;

	for (auto* node : nodes)
	{
		std::vector<Connection> connections;
		getNodeConnections(*node, connections);

		for (auto c : connections)
			if (!isConnectionLegal(c))
				anyRemoved = removeConnection(c) || anyRemoved;
	}

	return anyRemoved;
}

juce::ValueTree InternalNodeGraph::toValueTree() const
{
	juce::ValueTree graphTree("graph");
	juce::ValueTree nodesTree("nodes");
	juce::ValueTree connectionsTree("connections");
	
	for (const auto* node : nodes)
	{
		juce::ValueTree n("node");

		for (auto property : node->properties)
		{
			n.setProperty(property.name, property.value, nullptr);
		}

		n.setProperty("uid", (int)node->nodeID.uid, nullptr);

		nodesTree.addChild(n, -1, nullptr);
	}
	
	for (const auto& connection : getConnections())
	{
		juce::ValueTree c("connection");

		c.setProperty("srcID", juce::var(static_cast<int>(connection.source.nodeID.uid)), nullptr);
		c.setProperty("srcChannel", juce::var(connection.source.channelIndex), nullptr);
		c.setProperty("destID", juce::var(static_cast<int>(connection.destination.nodeID.uid)), nullptr);
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

	const juce::ValueTree nodesTree = graphTree.getChildWithName("nodes");
	const juce::ValueTree connectionsTree = graphTree.getChildWithName("connections");
	
	if (nodesTree.isValid())
	{
		const auto numNodes = nodesTree.getNumChildren();

		for (int i = 0; i < numNodes; ++i)
		{
			juce::ValueTree n = nodesTree.getChild(i);

			const NodeType type = static_cast<NodeType>(static_cast<int>(n.getProperty("type")));
			if (type == NodeType::Void) break;

			const auto nodeID = NodeID(static_cast<int>(n.getProperty("uid")));

			const auto node = addNode(type, nodeID, true);

			for (int i = 0; i < n.getNumProperties(); ++i)
			{
				auto propertyName = n.getPropertyName(i);

				node->properties.set(propertyName, n.getProperty(propertyName));
			}

			node->update();
		}
	}

	if (connectionsTree.isValid())
	{
		const auto numConnections = connectionsTree.getNumChildren();

		for (int i = 0; i < numConnections; ++i)
		{
			auto c = connectionsTree.getChild(i);

			addConnection({
				{NodeID((int)c.getProperty("srcID")), c.getProperty("srcChannel")},
				{NodeID((int)c.getProperty("destID")), c.getProperty("destChannel")}
				}, true);
		}
	}

	removeIllegalConnections();
	topologyChanged();
}

bool InternalNodeGraph::loopCheck(Node* src, Node* dest) const noexcept
{
	return dest->feedsInto(src->nodeID);
}

void InternalNodeGraph::topologyChanged()
{
	sendChangeMessage();

	if (juce::MessageManager::getInstance()->isThisTheMessageThread())
		handleAsyncUpdate();
	else
		triggerAsyncUpdate();
}

void InternalNodeGraph::handleAsyncUpdate()
{
	buildRenderingSequence();
}

void InternalNodeGraph::buildRenderingSequence()
{
	auto newSequence = std::make_unique<GraphRenderSequence>(*this);

	std::swap(renderSequence, newSequence);

	audioProcessor.setNodeProcessorSequence(*renderSequence);
}

#pragma endregion
