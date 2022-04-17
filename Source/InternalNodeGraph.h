/*
  ==============================================================================

    InternalNodeGraph.h
    Created: 15 Apr 2022 5:50:17pm
    Author:  Jonas

  ==============================================================================
*/

#pragma once
#include <JuceHeader.h>

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



        virtual float getNextSample();


        juce::NamedValueSet properties;

        using Ptr = juce::ReferenceCountedObjectPtr<Node>;
    private:

        struct Connection
        {
            Node* otherNode;
            int otherChannel, thisChannel;

            bool operator== (const Connection&) const noexcept;
        };

        juce::Array<Connection> inputs, outputs;

        Node (NodeID n) noexcept;

        juce::CriticalSection lock;

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Node)
    };








    struct Connection
    {
	    Connection() = default;
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

    Node::Ptr addNode (type, NodeID nodeId = {});

    Node::Ptr removeNode (NodeID);

    Node::Ptr removeNode (Node*);

    std::vector<Connection> getConnections() const;

    bool isConnected (const Connection&) const noexcept;

    bool isConnected (NodeID possibleSourceNodeID, NodeID possibleDestNodeID) const noexcept;

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
    bool canConnect (Node* src, int sourceChannel, Node* dest, int destChannel) const noexcept;
    bool isLegal (Node* src, int sourceChannel, Node* dest, int destChannel) const noexcept;
    static void getNodeConnections (Node&, std::vector<Connection>&);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (InternalNodeGraph)

};
