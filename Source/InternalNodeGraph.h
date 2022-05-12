/*
  ==============================================================================

    InternalNodeGraph.h
    Created: 15 Apr 2022 5:50:17pm
    Author:  Jonas

  ==============================================================================
*/

#pragma once
#include <JuceHeader.h>
#include "ByteCodeProcessor.h"
#include "Defines.h"
#include "ParameterManager.h"

class ByteBeatNodeGraphAudioProcessor;

enum NodeType
{
    Void,
	Expression,
    Output,
    Parameter
};

class InternalNodeGraph : public juce::ChangeBroadcaster,
							private juce::AsyncUpdater
{
public:
    /** Creates an empty graph. */
	explicit InternalNodeGraph(ByteBeatNodeGraphAudioProcessor* p, ParameterManager& paramManager);


    /** Destructor.
        Any processor objects that have been added to the graph will also be deleted.
    */
     ~InternalNodeGraph();

    /** Each node in the graph has a UID of this type. */
    struct NodeID
    {
        NodeID() {}
        explicit NodeID (juce::uint32 i) : uid (i) {}

        juce::uint32 uid = 0;

        bool operator== (const NodeID& other) const noexcept    { return uid == other.uid; }
        bool operator!= (const NodeID& other) const noexcept    { return uid != other.uid; }
        bool operator<  (const NodeID& other) const noexcept    { return uid <  other.uid; }
    };


    //==============================================================================
    /**
        Represents an input or output channel of a node.
    */
    struct NodeAndChannel
    {
        NodeID nodeID;
        int channelIndex;

        bool operator== (const NodeAndChannel& other) const noexcept    { return nodeID == other.nodeID && channelIndex == other.channelIndex; }
        bool operator!= (const NodeAndChannel& other) const noexcept    { return ! operator== (other); }
    };

    class Node   : public juce::ReferenceCountedObject
    {
    public:

        //==============================================================================
        /** The ID number assigned to this node.
            This is assigned by the graph that owns it, and can't be changed.
        */
        const NodeID nodeID;



        virtual void processNextValue() {}


        juce::NamedValueSet properties;

        juce::Atomic<float> outValue;

        using Ptr = juce::ReferenceCountedObjectPtr<Node>;

           auto getNumInputs() {return numInputs;}
		auto getNumOutputs() {return numOutputs;}

        bool feedsInto(NodeID id)
        {
	        for (auto c : outputs)
	        {
		        if (c.otherNode->nodeID == id) return true;

		        if (c.otherNode->feedsInto(id)) return true;
	        }

	        return false;
        }


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

        Node (NodeID n, int numIns, int numOuts) noexcept : nodeID(n), numInputs(numIns), numOutputs(numOuts)
        {

        }



    private:


        void setParentGraph (InternalNodeGraph*) const;

        int numInputs, numOutputs;


        juce::CriticalSection lock;

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Node)
    };



    class ExpressionNode : public Node
    {
    public:

        ExpressionNode(NodeID n) : Node(n, expr_node_num_ins, 1)
        {
            properties.set("type", NodeType::Expression);
        }

        void processNextValue() override
        {
	         for (auto c : inputs)
            {
                auto cValue =  c.otherNode->outValue.get();
	            if (c.otherChannel <= expr_node_num_ins)
	            {
		            inputValues[c.otherChannel] += cValue;
	            }

            }

             outValue.set(processor->process(inputValues, t));
        }

        std::unique_ptr<ByteCodeProcessor> processor;

    private:


        std::vector<float> inputValues;
        float t;



    };


    class OutputNode : public Node
    {
    public:
        OutputNode(NodeID n) : Node(n, 1, 0)
        {
	         properties.set("type", NodeType::Output);
        }

	    float getNextSample()
        {
            float value = 0;
            for (auto c : inputs)
            {
                if (c.thisChannel == 0)
                {
	                value += c.otherNode->outValue.get();
                }
            }

	    	return static_cast<juce::uint8>(value) / 128.f - 1.f ;
        }


    };

    class ParameterNode : public Node
    {
    public:
        ParameterNode(NodeID n, ParameterManager& paramManager) : Node(n, 0, 1), parameterManager(paramManager)
        {
            properties.set("type", NodeType::Parameter);
            properties.set("parameterID", paramManager.connectToID(properties["parameterID"]) );
        }

        ~ParameterNode() override
        {
            parameterManager.removeConnection(properties["parameterID"]);
        }


	    //juce::AudioProcessorValueTreeState::Parameter* parameter;

        //juce::AudioParameterFloat& parameter;
        //juce::String parameterID;

    private:
        ParameterManager& parameterManager;

    };


    struct Connection
    {
	    Connection();
        Connection (NodeAndChannel source, NodeAndChannel destination) noexcept;

        Connection (const Connection&) = default;
        Connection& operator= (const Connection&) = default;

        bool operator== (const Connection&) const noexcept;
        bool operator!= (const Connection&) const noexcept;
        bool operator<  (const Connection&) const noexcept;

        //==============================================================================
        /** The channel and node which is the input source for this connection. */
        NodeAndChannel source { {}, 0 };

        /** The channel and node which is the input source for this connection. */
        NodeAndChannel destination { {}, 0 };
    };


     void clear();

     const juce::ReferenceCountedArray<Node>& getNodes() const noexcept    { return nodes; }

     int getNumNodes() const noexcept                                { return nodes.size(); }

      Node::Ptr getNode (int index) const noexcept                    { return nodes[index]; }

    Node* getNodeForId (NodeID) const;

    Node::Ptr addNode (NodeType nodeType, NodeID nodeId = {});

    Node::Ptr removeNode (NodeID);

    Node::Ptr removeNode (Node*);

    std::vector<Connection> getConnections() const;

    bool isConnected (const Connection&) const noexcept;

    bool isConnected (NodeID srcID, NodeID destID) const noexcept;

    bool isAnInputTo (Node& source, Node& destination) const noexcept;

    bool canConnect (const Connection&) const;

    bool addConnection (const Connection&);

     bool removeConnection (const Connection&);

    bool disconnectNode (NodeID);

    bool isConnectionLegal (const Connection&) const;

    bool removeIllegalConnections();

    float getNextSample();

    juce::ValueTree toValueTree() const;

    void restoreFromTree(const juce::ValueTree& graphTree);

private:
    ByteBeatNodeGraphAudioProcessor* audioProcessor;
    ParameterManager& parameterManager;
    juce::ReferenceCountedArray<Node> nodes;
    NodeID lastNodeID = {};


     struct GraphRenderSequence;
     std::unique_ptr<GraphRenderSequence> renderSequence;

     std::atomic<bool> isPrepared { false };


    void topologyChanged();
    void unprepare();
    void handleAsyncUpdate() override;


        void clearRenderingSequence();
    void buildRenderingSequence();
    bool anyNodesNeedPreparing() const noexcept;


    bool isConnected (Node* src, int sourceChannel, Node* dest, int destChannel) const noexcept;
    bool isAnInputTo (Node& src, Node& dst, int recursionCheck) const noexcept;
    bool canConnect (Node* src, int sourceChannel, Node* dest, int destChannel) const noexcept;
    bool isLegal (Node* src, int sourceChannel, Node* dest, int destChannel) const noexcept; //IMPLEMENT
    bool loopCheck(Node* src, Node* dest) const noexcept;//IMPLEMENT
    static void getNodeConnections (Node&, std::vector<Connection>&);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (InternalNodeGraph)

};
