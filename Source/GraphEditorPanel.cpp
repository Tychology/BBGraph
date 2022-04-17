/*
  ==============================================================================

    GraphEditorPanel.cpp
    Created: 15 Apr 2022 5:35:09pm
    Author:  Jonas

  ==============================================================================
*/

#include "GraphEditorPanel.h"



struct GraphEditorPanel::NodeComponent : public Component, // private AudioProcessorParameter::Listener,
                                             private juce::AsyncUpdater
{
    NodeComponent (GraphEditorPanel& p, InternalNodeGraph::NodeID id) : panel (p), graph (p.graph), nodeID (id)
    {
	    shadow.setShadowProperties (juce::DropShadow (juce::Colours::black.withAlpha (0.5f), 3, { 0, 1 }));
        setComponentEffect (&shadow);

        /*NodeComponent (const NodeComponent&) = delete;
		NodeComponent& operator= (const NodeComponent&) = delete;*/

        setSize (150, 60);
    }

    void mouseDown (const juce::MouseEvent& e) override
    {
        originalPos = localPointToGlobal (juce::Point<int>());

        toFront (true);


    	if (e.mods.isPopupMenu())
    		showPopupMenu();
    }

    void mouseDrag (const juce::MouseEvent& e) override
    {

        if (! e.mods.isPopupMenu())
        {
            auto pos = originalPos + e.getOffsetFromDragStart();

            if (getParentComponent() != nullptr)
                pos = getParentComponent()->getLocalPoint (nullptr, pos);

            pos += getLocalBounds().getCentre();


            //!!!!!!!!!!!!!!!!!!!!!!!
            panel.setNodePosition (nodeID,
                                   { pos.x / (double) getParentWidth(),
                                     pos.y / (double) getParentHeight() });

            panel.updateComponents();
        }
    }


    void mouseUp (const juce::MouseEvent& e) override
    {
        if (e.mouseWasDraggedSinceMouseDown())
        {
            //graph.setChangedFlag (true); !!!!!!!!!!
        }

    }


     bool hitTest (int x, int y) override
    {
        for (auto* child : getChildren())
            if (child->getBounds().contains (x, y))
                return true;

        return x >= 3 && x < getWidth() - 6 && y >= pinSize && y < getHeight() - pinSize;
    }


    void paint (juce::Graphics& g) override
    {
        auto boxArea = getLocalBounds().reduced (4, pinSize);
        bool isBypassed = false;



        auto boxColour = findColour (juce::TextEditor::backgroundColourId);

        g.setColour (boxColour);
        g.fillRect (boxArea.toFloat());

        g.setColour (findColour (juce::TextEditor::textColourId));
        g.setFont (font);
        g.drawFittedText (getName(), boxArea, juce::Justification::centred, 2);
    }

    void resized() override
    {
        if (auto f = graph.getNodeForId (nodeID))
        {
            if (auto* processor = f->getProcessor())
            {
                for (auto* pin : pins)
                {
                    const bool isInput = pin->isInput;
                    auto channelIndex = pin->pin.channelIndex;
                    int busIdx = 0;
                    processor->getOffsetInBusBufferForAbsoluteChannelIndex (isInput, channelIndex, busIdx);

                    const int total = isInput ? numIns : numOuts;
                    const int index = pin->pin.isMIDI() ? (total - 1) : channelIndex;

                    auto totalSpaces = static_cast<float> (total) + (static_cast<float> (jmax (0, processor->getBusCount (isInput) - 1)) * 0.5f);
                    auto indexPos = static_cast<float> (index) + (static_cast<float> (busIdx) * 0.5f);

                    pin->setBounds (proportionOfWidth ((1.0f + indexPos) / (totalSpaces + 1.0f)) - pinSize / 2,
                                    pin->isInput ? 0 : (getHeight() - pinSize),
                                    pinSize, pinSize);
                }
            }
        }
    }


    juce::Point<float> getPinPos (int index, bool isInput) const
    {
        for (auto* pin : pins)
            if (pin->pin.channelIndex == index && isInput == pin->isInput)
                return getPosition().toFloat() + pin->getBounds().getCentre().toFloat();

        return {};
    }

    void update()
    {
        const InternalNodeGraph::Node::Ptr f (graph.getNodeForId (nodeID));
        jassert (f != nullptr);

        auto& processor = *f->getProcessor();

        numIns = processor.getTotalNumInputChannels();
        if (processor.acceptsMidi())
            ++numIns;

        numOuts = processor.getTotalNumOutputChannels();
        if (processor.producesMidi())
            ++numOuts;

        int w = 100;
        int h = 60;

        w = juce::jmax (w, (jmax (numIns, numOuts) + 1) * 20);

        const int textWidth = font.getStringWidth (processor.getName());
        w = juce::jmax (w, 16 + jmin (textWidth, 300));
        if (textWidth > 300)
            h = 100;

        setSize (w, h);
        setName (processor.getName() + formatSuffix);

        {
            auto p = graph.getNodePosition (pluginID);
            setCentreRelative ((float) p.x, (float) p.y);
        }

        if (numIns != numInputs || numOuts != numOutputs)
        {
            numInputs = numIns;
            numOutputs = numOuts;

            pins.clear();

            for (int i = 0; i < processor.getTotalNumInputChannels(); ++i)
                addAndMakeVisible (pins.add (new PinComponent (panel, { pluginID, i }, true)));

            if (processor.acceptsMidi())
                addAndMakeVisible (pins.add (new PinComponent (panel, { pluginID, AudioProcessorGraph::midiChannelIndex }, true)));

            for (int i = 0; i < processor.getTotalNumOutputChannels(); ++i)
                addAndMakeVisible (pins.add (new PinComponent (panel, { pluginID, i }, false)));

            if (processor.producesMidi())
                addAndMakeVisible (pins.add (new PinComponent (panel, { pluginID, AudioProcessorGraph::midiChannelIndex }, false)));

            resized();
        }
    }

    void showPopupMenu();


    void handleAsyncUpdate() override { repaint(); }


    GraphEditorPanel& panel;
    InternalNodeGraph graph;
    const InternalNodeGraph::NodeID nodeID;
    juce::OwnedArray<PinComponent> pins;
    int numInputs = 0, numOutputs = 0;
     int pinSize = 16;
    juce::Point<int> originalPos;
    juce::Font font { 13.0f, juce::Font::bold };
    int numIns = 0, numOuts = 0;
    juce::DropShadowEffect shadow;
    std::unique_ptr<juce::PopupMenu> menu;

};


struct GraphEditorPanel::PinComponent   : public Component,
                                          public juce::SettableTooltipClient
{
    PinComponent (GraphEditorPanel& p, InternalNodeGraph::NodeAndChannel pinToUse, bool isIn)
        : panel (p), graph (p.graph), pin (pinToUse), isInput (isIn)
    {
        if (auto node = graph.getNodeForId (pin.nodeID))
        {
	        juce::String tip;




            setTooltip (tip);
        }

        setSize (16, 16);
    }

    void paint (juce::Graphics& g) override
    {
        auto w = (float) getWidth();
        auto h = (float) getHeight();

        juce::Path p;
        p.addEllipse (w * 0.25f, h * 0.25f, w * 0.5f, h * 0.5f);
        p.addRectangle (w * 0.4f, isInput ? (0.5f * h) : 0.0f, w * 0.2f, h * 0.5f);

        auto colour = juce::Colours::red;


        g.fillPath (p);
    }

    void mouseDown (const juce::MouseEvent& e) override
    {
        InternalNodeGraph::NodeAndChannel dummy { {}, 0 };

        panel.beginConnectorDrag (isInput ? dummy : pin,
                                  isInput ? pin : dummy,
                                  e);
    }

    void mouseDrag (const juce::MouseEvent& e) override
    {
        panel.dragConnector (e);
    }

    void mouseUp (const juce::MouseEvent& e) override
    {
        panel.endDraggingConnector (e);
    }

    GraphEditorPanel& panel;
    InternalNodeGraph& graph;
    InternalNodeGraph::NodeAndChannel pin;
    const bool isInput;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PinComponent)
};



struct GraphEditorPanel::ConnectorComponent   : public Component,
                                                public juce::SettableTooltipClient
{
    explicit ConnectorComponent (GraphEditorPanel& p)
        : panel (p), graph (p.graph)
    {
        setAlwaysOnTop (true);
    }

    void setInput (InternalNodeGraph::NodeAndChannel newSource)
    {
        if (connection.source != newSource)
        {
            connection.source = newSource;
            update();
        }
    }

    void setOutput (InternalNodeGraph::NodeAndChannel newDest)
    {
        if (connection.destination != newDest)
        {
            connection.destination = newDest;
            update();
        }
    }

    void dragStart (juce::Point<float> pos)
    {
        lastInputPos = pos;
        resizeToFit();
    }

    void dragEnd (juce::Point<float> pos)
    {
        lastOutputPos = pos;
        resizeToFit();
    }

    void update()
    {
	    juce::Point<float> p1, p2;
        getPoints (p1, p2);

        if (lastInputPos != p1 || lastOutputPos != p2)
            resizeToFit();
    }

    void resizeToFit()
    {
	    juce::Point<float> p1, p2;
        getPoints (p1, p2);

        auto newBounds = juce::Rectangle<float> (p1, p2).expanded (4.0f).getSmallestIntegerContainer();

        if (newBounds != getBounds())
            setBounds (newBounds);
        else
            resized();

        repaint();
    }

    void getPoints (juce::Point<float>& p1, juce::Point<float>& p2) const
    {
        p1 = lastInputPos;
        p2 = lastOutputPos;

        if (auto* src = panel.getComponentForNode (connection.source.nodeID))
            p1 = src->getPinPos (connection.source.channelIndex, false);

        if (auto* dest = panel.getComponentForNode (connection.destination.nodeID))
            p2 = dest->getPinPos (connection.destination.channelIndex, true);
    }

    void paint (juce::Graphics& g) override
    {
    	g.setColour (juce::Colours::red);

        g.fillPath (linePath);
    }

    bool hitTest (int x, int y) override
    {
        auto pos = juce::Point<int> (x, y).toFloat();

        if (hitPath.contains (pos))
        {
            double distanceFromStart, distanceFromEnd;
            getDistancesFromEnds (pos, distanceFromStart, distanceFromEnd);

            // avoid clicking the connector when over a pin
            return distanceFromStart > 7.0 && distanceFromEnd > 7.0;
        }

        return false;
    }

    void mouseDown (const juce::MouseEvent&) override
    {
        dragging = false;
    }

    void mouseDrag (const juce::MouseEvent& e) override
    {
        if (dragging)
        {
            panel.dragConnector (e);
        }
        else if (e.mouseWasDraggedSinceMouseDown())
        {
            dragging = true;

            graph.removeConnection (connection);

            double distanceFromStart, distanceFromEnd;
            getDistancesFromEnds (getPosition().toFloat() + e.position, distanceFromStart, distanceFromEnd);
            const bool isNearerSource = (distanceFromStart < distanceFromEnd);

            InternalNodeGraph::NodeAndChannel dummy { {}, 0 };

            panel.beginConnectorDrag (isNearerSource ? dummy : connection.source,
                                      isNearerSource ? connection.destination : dummy,
                                      e);
        }
    }

    void mouseUp (const juce::MouseEvent& e) override
    {
        if (dragging)
            panel.endDraggingConnector (e);
    }

    void resized() override
    {
	    juce::Point<float> p1, p2;
        getPoints (p1, p2);

        lastInputPos = p1;
        lastOutputPos = p2;

        p1 -= getPosition().toFloat();
        p2 -= getPosition().toFloat();

        linePath.clear();
        linePath.startNewSubPath (p1);
        linePath.cubicTo (p1.x, p1.y + (p2.y - p1.y) * 0.33f,
                          p2.x, p1.y + (p2.y - p1.y) * 0.66f,
                          p2.x, p2.y);

	    juce::PathStrokeType wideStroke (8.0f);
        wideStroke.createStrokedPath (hitPath, linePath);

	    juce::PathStrokeType stroke (2.5f);
        stroke.createStrokedPath (linePath, linePath);

        auto arrowW = 5.0f;
        auto arrowL = 4.0f;

	    juce::Path arrow;
        arrow.addTriangle (-arrowL, arrowW,
                           -arrowL, -arrowW,
                           arrowL, 0.0f);

        arrow.applyTransform (juce::AffineTransform()
                              .rotated (juce::MathConstants<float>::halfPi - (float) atan2 (p2.x - p1.x, p2.y - p1.y))
                              .translated ((p1 + p2) * 0.5f));

        linePath.addPath (arrow);
        linePath.setUsingNonZeroWinding (true);
    }

    void getDistancesFromEnds (juce::Point<float> p, double& distanceFromStart, double& distanceFromEnd) const
    {
	    juce::Point<float> p1, p2;
        getPoints (p1, p2);

        distanceFromStart = p1.getDistanceFrom (p);
        distanceFromEnd   = p2.getDistanceFrom (p);
    }

    GraphEditorPanel& panel;
    InternalNodeGraph& graph;
    InternalNodeGraph::Connection connection { { {}, 0 }, { {}, 0 } };
    juce::Point<float> lastInputPos, lastOutputPos;
    juce::Path linePath, hitPath;
    bool dragging = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ConnectorComponent)
};