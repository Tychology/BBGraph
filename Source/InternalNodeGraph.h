#pragma once

#include <JuceHeader.h>

#include "ByteCodeProcessor.h"
#include "ParameterManager.h"

struct GraphRenderSequence;

class ByteBeatNodeGraphAudioProcessor;

enum NodeType
{
	Void,
	Expression,
	Output,
	Parameter
};

class InternalNodeGraph : public juce::ChangeBroadcaster, juce::AsyncUpdater
{
public:
	explicit InternalNodeGraph(ByteBeatNodeGraphAudioProcessor& p, ParameterManager& paramManager);

	~InternalNodeGraph();

	// Each node in the graph has a UID of this type.
	struct NodeID
	{
		NodeID() {}
		explicit NodeID(juce::uint32 i) : uid(i) {}

		juce::uint32 uid = 0;

		bool operator== (const NodeID& other) const noexcept { return uid == other.uid; }
		bool operator!= (const NodeID& other) const noexcept { return uid != other.uid; }
		bool operator<  (const NodeID& other) const noexcept { return uid < other.uid; }
	};

	// Represents an input or output channel of a node.
	struct NodeAndChannel
	{
		NodeID nodeID;
		int channelIndex;

		bool operator== (const NodeAndChannel& other) const noexcept { return nodeID == other.nodeID && channelIndex == other.channelIndex; }
		bool operator!= (const NodeAndChannel& other) const noexcept { return !operator== (other); }
	};

	class Node : public juce::ReferenceCountedObject
	{
	public:
		// The ID number assigned to this node.
		// This is assigned by the graph that owns it, and can't be changed.
		const NodeID nodeID;

		juce::NamedValueSet properties;

		using Ptr = juce::ReferenceCountedObjectPtr<Node>;

		auto getNumInputs() { return numInputs; }
		auto getNumOutputs() { return numOutputs; }

		bool feedsInto(NodeID id);

		virtual void update() {}

	protected:
		friend class InternalNodeGraph;
		friend struct GraphRenderSequence;

		struct Connection
		{
			Node* otherNode;
			int otherChannel, thisChannel;

			bool operator==(const Connection& other) const noexcept
			{
				return otherNode == other.otherNode
					&& thisChannel == other.thisChannel
					&& otherChannel == other.otherChannel;
			}
		};

		juce::Array<Connection> inputs, outputs;

		Node(NodeID n, int numIns, int numOuts) noexcept : nodeID(n), numInputs(numIns), numOutputs(numOuts)
		{
		}

	private:
		int numInputs, numOutputs;
		juce::CriticalSection lock;

		JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Node)
	};

	class ExpressionNode : public Node
	{
	public:

		ExpressionNode(NodeID n);

		void update() override;

		std::unique_ptr<ByteCodeProcessor> processor;

	private:
		std::vector<float> inputValues;
		float t;
	};

	class OutputNode : public Node
	{
	public:
		OutputNode(NodeID n);

		bool isStereo();
	};

	class ParameterNode : public Node
	{
	public:
		ParameterNode(NodeID n, ParameterManager& paramManager);
		~ParameterNode() override;

	private:
		ParameterManager& parameterManager;

	};

	struct Connection
	{
		Connection();
		Connection(NodeAndChannel source, NodeAndChannel destination) noexcept;

		Connection(const Connection&) = default;
		Connection& operator= (const Connection&) = default;

		bool operator== (const Connection&) const noexcept;
		bool operator!= (const Connection&) const noexcept;
		bool operator<  (const Connection&) const noexcept;
		
		// The channel and node which is the input source for this connection.
		NodeAndChannel source{ {}, 0 };

		// The channel and node which is the input source for this connection.
		NodeAndChannel destination{ {}, 0 };
	};
	
	void clear();

	const juce::ReferenceCountedArray<Node>& getNodes() const noexcept { return nodes; }

	int getNumNodes() const noexcept { return nodes.size(); }

	Node::Ptr getNode(int index) const noexcept { return nodes[index]; }

	Node* getNodeForId(NodeID) const;

	Node::Ptr addNode(NodeType nodeType, NodeID nodeId = {}, bool quiet = false);

	Node::Ptr removeNode(NodeID);

	Node::Ptr removeNode(Node*);

	std::vector<Connection> getConnections() const;

	bool isConnected(const Connection&) const noexcept;

	bool isConnected(NodeID srcID, NodeID destID) const noexcept;

	bool isAnInputTo(Node& source, Node& destination) const noexcept;

	bool canConnect(const Connection&) const;

	bool addConnection(const Connection&, bool quiet = false);

	bool removeConnection(const Connection&);

	bool disconnectNode(NodeID);

	bool isConnectionLegal(const Connection&) const;

	bool removeIllegalConnections();
	
	juce::ValueTree toValueTree() const;

	void restoreFromTree(const juce::ValueTree& graphTree);

private:
	ByteBeatNodeGraphAudioProcessor& audioProcessor;
	ParameterManager& parameterManager;
	juce::ReferenceCountedArray<Node> nodes;
	NodeID lastNodeID = {};
	
	std::unique_ptr<GraphRenderSequence> renderSequence;
	
	void topologyChanged();
	void handleAsyncUpdate() override;
	void buildRenderingSequence();

	bool isConnected(Node* src, int sourceChannel, Node* dest, int destChannel) const noexcept;
	bool isAnInputTo(Node& src, Node& dst, int recursionCheck) const noexcept;
	bool canConnect(Node* src, int sourceChannel, Node* dest, int destChannel) const noexcept;
	bool loopCheck(Node* src, Node* dest) const noexcept;
	static void getNodeConnections(Node&, std::vector<Connection>&);

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(InternalNodeGraph)
};
