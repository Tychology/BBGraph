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
	InternalNodeGraph();

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
    protected:
        friend class InternalNodeGraph;

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

        Node (NodeID n) noexcept : nodeID(n)
        {

        }


    private:


        void setParentGraph (InternalNodeGraph*) const;


        juce::CriticalSection lock;

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Node)
    };



    class ExpressionNode : public Node
    {
    public:

        ExpressionNode(NodeID n) : Node(n)
        {
        }

        void processNextValue() override
        {
	         for (auto c : inputs)
            {
                auto cValue =  c.otherNode->outValue.get();
	            if (c.otherChannel <= 4)
	            {
		            inputValues[c.otherChannel] += cValue;
	            }

            }

             outValue.set(processor->process(inputValues, t));
        }

    private:


        float inputValues[4];
        float t;
        std::unique_ptr<ByteCodeProcessor> processor;


    };


    class OutputNode : public Node
    {
    public:
        OutputNode(NodeID n) : Node(n) {}

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
        ParameterNode(NodeID n) : Node(n)
        {

        }

	    void processNextValue()
	    {
            if (parameter != nullptr)
				outValue.set(parameter->get());
            else outValue.set(0);
	    }


    private:
	    juce::AudioProcessorValueTreeState::Parameter* parameter;
        //juce::AudioParameterFloat&
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


    private:
    juce::ReferenceCountedArray<Node> nodes;
    NodeID lastNodeID = {};


    void topologyChanged();
    void unprepare();
    void handleAsyncUpdate() override;

    bool anyNodesNeedPreparing() const noexcept;

    bool isConnected (Node* src, int sourceChannel, Node* dest, int destChannel) const noexcept;
    bool isAnInputTo (Node& src, Node& dst, int recursionCheck) const noexcept;
    bool canConnect (Node* src, int sourceChannel, Node* dest, int destChannel) const noexcept; //IMPLEMENT
    bool isLegal (Node* src, int sourceChannel, Node* dest, int destChannel) const noexcept; //IMPLEMENT
    static void getNodeConnections (Node&, std::vector<Connection>&);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (InternalNodeGraph)

};
