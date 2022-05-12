/*
  ==============================================================================

    GraphEditorPanel.cpp
    Created: 15 Apr 2022 5:35:09pm
    Author:  Jonas

  ==============================================================================
*/

#include "GraphEditorPanel.h"

#include "PluginProcessor.h"
#include "CustomRange.h"


void LookAndFeel::drawRotarySlider(juce::Graphics& g, int x, int y, int width, int height,
                                   float sliderPosProportional, float rotaryStartAngle, float rotaryEndAngle, juce::Slider& slider)
{
	using namespace juce;
	auto bounds = Rectangle<float>(x, y, width, height);
    auto center = bounds.getCentre();

    g.setColour(juce::Colours::red);
    g.drawRect(bounds, 2);

    bounds.reduce((width-height)/2 + 10, 10);


    g.setColour(juce::Colours::grey);
    g.fillEllipse(bounds);


	Path p;

	Rectangle<float> r;
	r.setLeft(center.getX() - 2);
	r.setRight(center.getX() + 2);
	r.setTop(bounds.getY() + 5);
	r.setBottom(center.getY() - 20);

	p.addRoundedRectangle(r, 2.f);

	jassert(rotaryStartAngle < rotaryEndAngle);

	auto sliderAngRad = jmap(sliderPosProportional, 0.f, 1.f, rotaryStartAngle, rotaryEndAngle);

	p.applyTransform(AffineTransform().rotated(sliderAngRad, center.getX(), center.getY()));

	g.setColour(juce::Colours::white);
	g.fillPath(p);


	r.setLeft(center.getX() - 2);
	r.setRight(center.getX() + 2);
	r.setTop(bounds.getY() - 15);
	r.setBottom(bounds.getY() - 5);
	p.clear();
	p.addRoundedRectangle(r, 2.f);
	p.applyTransform(AffineTransform().rotated(rotaryStartAngle, center.getX(), center.getY()));

	g.setColour(juce::Colours::white);
	g.fillPath(p);

    r.setLeft(center.getX() - 2);
	r.setRight(center.getX() + 2);
	r.setTop(bounds.getY() - 15);
	r.setBottom(bounds.getY() - 5);
	p.clear();
	p.addRoundedRectangle(r, 2.f);
	p.applyTransform(AffineTransform().rotated(rotaryEndAngle, center.getX(), center.getY()));

    g.fillPath(p);

}


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

         auto colour = juce::Colours::red;
        juce::Path p;
        p.addEllipse (w * 0.25f, h * 0.25f, w * 0.5f, h * 0.5f);
        p.addRectangle (w * 0.4f, isInput ? (0.5f * h) : 0.0f, w * 0.2f, h * 0.5f);


        g.setColour(colour);

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

struct GraphEditorPanel::NodeComponent : public Component, // private AudioProcessorParameter::Listener,
                                         private juce::AsyncUpdater
{
    NodeComponent (GraphEditorPanel& p, InternalNodeGraph::NodeID id)
	: panel (p), graph (p.graph), nodeID (id), numInputs(graph.getNodeForId(nodeID)->getNumInputs()), numOutputs(graph.getNodeForId(nodeID)->getNumOutputs())
    {
	    shadow.setShadowProperties (juce::DropShadow (juce::Colours::black.withAlpha (0.5f), 3, { 0, 1 }));
        setComponentEffect (&shadow);

        /*NodeComponent (const NodeComponent&) = delete;
		NodeComponent& operator= (const NodeComponent&) = delete;*/

         for (int i = 0; i < numInputs; ++i)
                addAndMakeVisible (pins.add (new PinComponent (panel, { nodeID, i }, true)));



    	for (int i = 0; i < numOutputs; ++i)
                addAndMakeVisible (pins.add (new PinComponent (panel, { nodeID, i }, false)));



        //setSize (150, 60);
    }

    void mouseDown (const juce::MouseEvent& e) override
    {
        originalPos = localPointToGlobal (juce::Point<int>());

        toFront (false);


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


    //void mouseUp (const juce::MouseEvent& e) override
    //{
    //    if (e.mouseWasDraggedSinceMouseDown())
    //    {
    //        graph.setChangedFlag (true); !!!!!!!!!!
    //    }

    //}


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
	    if (auto f = graph.getNodeForId(nodeID))
	    {
		    for (auto* pin : pins)
		    {
			    const bool isInput = pin->isInput;
			    auto channelIndex = pin->pin.channelIndex;
			    int busIdx = 0;
			    //processor->getOffsetInBusBufferForAbsoluteChannelIndex (isInput, channelIndex, busIdx);

			    const int total = isInput ? numInputs : numOutputs;
			    //const int index = pin->pin.isMIDI() ? (total - 1) : channelIndex;

			    auto totalSpaces = static_cast<float>(total);
			    // + (static_cast<float> (juce::jmax (0, processor->getBusCount (isInput) - 1)) * 0.5f);
			    auto indexPos = static_cast<float>(channelIndex) + (static_cast<float>(busIdx) * 0.5f);

			    pin->setBounds(proportionOfWidth((1.0f + indexPos) / (totalSpaces + 1.0f)) - pinSize / 2,
			                   pin->isInput ? 0 : (getHeight() - pinSize),
			                   pinSize, pinSize);
		    }
	    }

        onResize();
    }


    virtual void onResize()
    {

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



        //int w = getWidth();
        //int h = getHeight();

        //w = juce::jmax (w, (juce::jmax (numInputs, numOutputs) + 1) * 20);

        //const int textWidth = 300;//font.getStringWidth (processor.getName());
        //w = juce::jmax (w, 16 + juce::jmin (textWidth, 300));
        //if (textWidth > 300)
        //    h = 100;

        //setSize (w, h);
        //setName (processor.getName() + formatSuffix);


            auto p = panel.getNodePosition (nodeID);
            setCentreRelative ((float) p.x, (float) p.y);


            resized();

    }

    virtual void showPopupMenu()
    {

    }


    void handleAsyncUpdate() override { repaint(); }


    GraphEditorPanel& panel;
    InternalNodeGraph& graph;
    const InternalNodeGraph::NodeID nodeID;
    juce::OwnedArray<PinComponent> pins;
    int numInputs, numOutputs;
     int pinSize = 16;
    juce::Point<int> originalPos;
    juce::Font font { 13.0f, juce::Font::bold };
    //int numIns = 0, numOuts = 0;
    juce::DropShadowEffect shadow;
    std::unique_ptr<juce::PopupMenu> menu;

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





struct GraphEditorPanel::ExpressionNodeComponent : NodeComponent
{

    ExpressionNodeComponent(GraphEditorPanel& p, InternalNodeGraph::NodeID id) : NodeComponent(p, id)
    {

	    addAndMakeVisible(textBox);
        textBox.setMultiLine(false);
        textBox.setFont(font);

        auto* node = graph.getNodeForId(nodeID);
        textBox.setText(node->properties.getWithDefault("Expression", ""));


        textBox.onReturnKey = [this] ()
		{
            unfocusAllComponents();
        };

        textBox.onFocusLost = [this] ()
		{
            if(auto* node = static_cast<InternalNodeGraph::ExpressionNode*>(graph.getNodeForId(nodeID)))
            {
	            auto expressionString = textBox.getText();
				node->properties.set("Expression", expressionString );

				//node.parse(expressionString);
            }

        };

        textBox.onTextChange = [this] ()
		{
            auto textWidth = font.getStringWidthFloat(textBox.getText());
            auto w = juce::jmin(juce::jmax(textWidth + characterWidth + pinSize * 2, defaultWidth), maxWidth);
        	setSize(w, getHeight());

        };

        characterWidth = font.getStringWidthFloat("0");

        defaultWidth = characterWidth * 16 + pinSize * 2;
        maxWidth = characterWidth * 64 + pinSize * 2;

        setSize(defaultWidth, font.getHeight() + pinSize * 4);


    }

    void showPopupMenu() override
    {
                menu.reset (new juce::PopupMenu);
        menu->addItem (1, "Delete");
        menu->addItem (2, "Disconnect all pins");

         menu->showMenuAsync ({}, juce::ModalCallbackFunction::create
                             ([this] (int r) {
        switch (r)
        {
            case 1:   graph.removeNode (nodeID); break;
            case 2:   graph.disconnectNode (nodeID); break;

        }
        }));
    }

    void onResize() override
    {

        auto bounds = getLocalBounds();

        bounds.reduce(pinSize, pinSize * 1.75);


	    textBox.setBounds(bounds);
    }

private:

	juce::TextEditor textBox;
    juce::Font font {juce::Font::getDefaultMonospacedFontName(), 20, 0};
    float characterWidth;
    float defaultWidth;
    float maxWidth;
};


struct GraphEditorPanel::OutputNodeComponent : NodeComponent
{
    OutputNodeComponent(GraphEditorPanel& p, InternalNodeGraph::NodeID id) : NodeComponent(p, id)
    {

        addAndMakeVisible(outSymbol);

        juce::Path path;
        auto constexpr pi = juce::MathConstants<float>::pi;
        path.addArc(0, 0, 50, 50, pi * 1.5, pi * 2.5, true);
        juce::Line<float> line(25, 25, 25, 55);
        path.addArrow(line, 1, 25, 12);

        outSymbol.setPath(path);
        outSymbol.setFill(juce::FillType(juce::Colours::transparentBlack));
        outSymbol.setStrokeType(juce::PathStrokeType(12, juce::PathStrokeType::JointStyle::curved, juce::PathStrokeType::EndCapStyle::rounded));
        outSymbol.setStrokeFill(juce::FillType(juce::Colours::limegreen));



        setSize(pinSize * 5, pinSize * 6);
    }

    void onResize() override
    {
        auto centre = getLocalBounds().getCentre();

        juce::Rectangle<float> outSymbolArea(centre.x - pinSize, centre.y - pinSize, pinSize * 2, pinSize * 2);

        outSymbol.setTransformToFit(outSymbolArea, juce::RectanglePlacement::centred);
    }

     void showPopupMenu() override
    {
                menu.reset (new juce::PopupMenu);
        menu->addItem (1, "Delete");
        menu->addItem (2, "Disconnect all pins");

         menu->showMenuAsync ({}, juce::ModalCallbackFunction::create
                             ([this] (int r) {
        switch (r)
        {
            case 1:   graph.removeNode (nodeID); break;
            case 2:   graph.disconnectNode (nodeID); break;

        }
        }));
    }

     juce::DrawablePath outSymbol;

};


struct GraphEditorPanel::ParameterNodeComponent : NodeComponent
{

    ParameterNodeComponent(GraphEditorPanel& p, InternalNodeGraph::NodeID id, juce::AudioProcessorValueTreeState& apvts, juce::String paramID) : NodeComponent(p, id),
	range(apvts.getParameterRange(paramID))//, parameterID(paramID)
    {

	    addAndMakeVisible(paramSlider);
        paramSlider.setSliderStyle(juce::Slider::SliderStyle::RotaryHorizontalVerticalDrag);
        paramSlider.setNumDecimalPlacesToDisplay(2);
        paramSlider.setTextBoxStyle(juce::Slider::TextEntryBoxPosition::TextBoxBelow, false, paramSlider.getTextBoxWidth(), paramSlider.getTextBoxHeight());
        paramSlider.setLookAndFeel(&laf);
        //paramSlider.setNumDecimalPlacesToDisplay(0);

        //auto paramID = param.getParameterID();

        sliderAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
            apvts, paramID, paramSlider);


        addAndMakeVisible(minLabel);
        addAndMakeVisible(maxLabel);
        minLabel.setEditable(true);
        maxLabel.setEditable(true);

        minLabel.onTextChange = [this] ()
        {
	        auto newStart = minLabel.getText().getFloatValue();
            auto newEnd = range.end;

            if (log)
            {
                if (newStart <= 0.f) newStart = 0.01f;
                if (newStart >= newEnd) newEnd = newStart + 0.01f;

                range = logRange<float>(newStart, newEnd);
                paramSlider.setNormalisableRange(logRange<double>(newStart, newEnd));
            }
            else
            {
	            if (newStart >= newEnd) newStart = newEnd - 0.01f;

                range = juce::NormalisableRange<float>(newStart, newEnd, 0);
                paramSlider.setNormalisableRange(juce::NormalisableRange<double>(newStart, newEnd, 0));
            }

            paramSlider.repaint();

            minLabel.setText(juce::String(newStart), juce::dontSendNotification);
            maxLabel.setText(juce::String(newEnd), juce::dontSendNotification);
        };

        maxLabel.onTextChange = [this] ()
        {
            auto newStart = range.start;
	        auto newEnd = maxLabel.getText().getFloatValue();

            if (log)
            {
                if (newEnd <= 0.f) newEnd = 0.01f;
                if (newEnd <= newStart) newEnd = newStart + 0.01f;

                range = logRange<float>(newStart, newEnd);
                paramSlider.setNormalisableRange(logRange<double>(newStart, newEnd));
            }
            else
            {
				if (newEnd <= newStart) newEnd = newStart + 0.01f;

            	range = juce::NormalisableRange<float>(newStart, newEnd, 0);
                paramSlider.setNormalisableRange(juce::NormalisableRange<double>(newStart, newEnd, 0));
            }

        	paramSlider.repaint();

            minLabel.setText(juce::String(newStart), juce::dontSendNotification);
            maxLabel.setText(juce::String(newEnd), juce::dontSendNotification);
        };

    	minLabel.setText(juce::String(range.start), juce::dontSendNotification);
        maxLabel.setText(juce::String(range.end), juce::dontSendNotification);

        addAndMakeVisible(paramNameLabel);
        paramNameLabel.setEditable(false);
        paramNameLabel.setText(paramName + " : linear", juce::dontSendNotification);
        paramNameLabel.setJustificationType(juce::Justification::centred);

        setSize(200, 200);

    }

    ~ParameterNodeComponent() override
    {
	    paramSlider.setLookAndFeel(nullptr);
    }

   void showPopupMenu() override
    {
    	menu.reset (new juce::PopupMenu);
        menu->addItem (1, "Delete");
        menu->addItem (2, "Disconnect all pins");
        menu->addSeparator();
        if (log)
			menu->addItem(3, "Convert to linear parameter");
        else
            menu->addItem(4, "Convert to logarithmic parameter");


    	menu->showMenuAsync ({}, juce::ModalCallbackFunction::create
                             ([this] (int r) {
        switch (r)
        {
        case 1: graph.removeNode(nodeID);
        	break;
        case 2: graph.disconnectNode(nodeID);
	        break;

        case 3:
	        range = juce::NormalisableRange<float>(range.start, range.end);

            paramSlider.setNormalisableRange(juce::NormalisableRange<double>(range.start, range.end));
            paramSlider.repaint();

            paramNameLabel.setText(paramName + " : linear", juce::dontSendNotification);

            log = false;
	        break;

        case 4:
	        range = logRange<float>(range.start > 0.f ? range.start : 0.01f, range.end);
            if (range.start > range.end) range.end += 0.01f;

            paramSlider.setNormalisableRange(logRange<double>(range.start, range.end));
            paramSlider.repaint();

            minLabel.setText(juce::String(range.start), juce::dontSendNotification);
            paramNameLabel.setText(paramName + " : logarithmic", juce::dontSendNotification);

            log = true;
	        break;

        default: break;
        }
        }));
    }


    void onResize() override
    {
        auto bounds = getLocalBounds();

        bounds.removeFromTop(pinSize);
        paramNameLabel.setBounds(bounds.removeFromTop(pinSize));
        auto lableArea = bounds.removeFromBottom(pinSize * 3);
        lableArea.removeFromBottom(pinSize * 1.5);
        lableArea.reduce(pinSize * 0.5, 0);


        bounds.reduce((bounds.getWidth()-bounds.getHeight() )/ 2, 0);
	    paramSlider.setBounds(bounds);

        minLabel.setBounds(lableArea.removeFromLeft(proportionOfWidth(0.5)));
        maxLabel.setBounds(lableArea);
    }

    bool log {false};

    juce::NormalisableRange<float>& range;
    //juce::AudioParameterFloat& param;
    //juce::String parameterID;
    juce::String paramName;
	juce::Slider paramSlider;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> sliderAttachment;

    juce::Label minLabel, maxLabel, paramNameLabel;

   LookAndFeel laf;

};




GraphEditorPanel::GraphEditorPanel(juce::AudioProcessorValueTreeState& apvts, InternalNodeGraph& g): graph(g), apvts(apvts)
{
	graph.addChangeListener (this);
	setOpaque (true);
}

GraphEditorPanel::~GraphEditorPanel()
{
	graph.removeChangeListener (this);
	draggingConnector = nullptr;
	nodes.clear();
	connectors.clear();
}

void GraphEditorPanel::createNewNode(NodeType nodeType, juce::Point<int> position)
{
    auto pos = position.toDouble() / juce::Point<double> ((double) getWidth(), (double) getHeight());

	if (auto node = graph.addNode(nodeType))
    {
        node->properties.set ("x", pos.x);
        node->properties.set ("y", pos.y);
       // changed();
    }
}

void GraphEditorPanel::setNodePosition(InternalNodeGraph::NodeID nodeID, juce::Point<double> pos)
{
    if (auto* n = graph.getNodeForId (nodeID))
    {
        n->properties.set ("x", juce::jlimit (0.0, 1.0, pos.x));
        n->properties.set ("y", juce::jlimit (0.0, 1.0, pos.y));
    }
}

juce::Point<double> GraphEditorPanel::getNodePosition(InternalNodeGraph::NodeID nodeID) const
{
    if (auto* n = graph.getNodeForId (nodeID))
        return { static_cast<double> (n->properties ["x"]),
                 static_cast<double> (n->properties ["y"]) };

    return {};
}

void GraphEditorPanel::updateComponents()
{
    for (int i = nodes.size(); --i >= 0;)
        if (graph.getNodeForId (nodes.getUnchecked(i)->nodeID) == nullptr)
            nodes.remove (i);

    for (int i = connectors.size(); --i >= 0;)
        if (! graph.isConnected (connectors.getUnchecked(i)->connection))
            connectors.remove (i);

    for (auto* fc : nodes)
        fc->update();

    for (auto* cc : connectors)
        cc->update();

    for (auto* f : graph.getNodes())
    {
        if (getComponentForNode (f->nodeID) == nullptr)
        {
            NodeComponent* comp;

	        if (auto* expf = dynamic_cast<InternalNodeGraph::ExpressionNode*>(f))
	        {
		        comp = nodes.add(new ExpressionNodeComponent(*this, f->nodeID));
	        }
	        else if (auto* outf = dynamic_cast<InternalNodeGraph::OutputNode*>(f))
	        {
		        comp = nodes.add(new OutputNodeComponent(*this, f->nodeID));
	        }
	        else if (auto* paramf = dynamic_cast<InternalNodeGraph::ParameterNode*>(f))
	        {
		        comp = nodes.add(new ParameterNodeComponent(*this, f->nodeID, apvts, paramf->parameterID));
	        }
	        else
	        {
		        jassertfalse;
                continue;
	        }

            addAndMakeVisible(comp);
        	comp->update();
        }
    }

     for (auto& c : graph.getConnections())
    {
        if (getComponentForConnection (c) == nullptr)
        {
            auto* comp = connectors.add (new ConnectorComponent (*this));
            addAndMakeVisible (comp);

            comp->setInput (c.source);
            comp->setOutput (c.destination);
        }
    }

}

void GraphEditorPanel::showPopupMenu(juce::Point<int> position)
{
	menu.reset(new juce::PopupMenu);


	menu->addItem(NodeType::Expression, "Expression Node");
	menu->addItem(NodeType::Output, "Output Node");
    menu->addItem(NodeType::Parameter, "Parameter Node");


	menu->showMenuAsync({},
	                    juce::ModalCallbackFunction::create([this, position](int r)
	                    {
		                    if (r > 0)
			                    createNewNode(NodeType(r), position);
	                    }));
}

void GraphEditorPanel::beginConnectorDrag(InternalNodeGraph::NodeAndChannel source,
	InternalNodeGraph::NodeAndChannel dest, const juce::MouseEvent& e)
{
    auto* c = dynamic_cast<ConnectorComponent*> (e.originalComponent);
    connectors.removeObject (c, false);
    draggingConnector.reset (c);

    if (draggingConnector == nullptr)
        draggingConnector.reset (new ConnectorComponent (*this));

    draggingConnector->setInput (source);
    draggingConnector->setOutput (dest);

    addAndMakeVisible (draggingConnector.get());
    draggingConnector->toFront (false);

    dragConnector (e);
}

void GraphEditorPanel::dragConnector(const juce::MouseEvent& e)
{
    auto e2 = e.getEventRelativeTo (this);

    if (draggingConnector != nullptr)
    {
        draggingConnector->setTooltip ({});

        auto pos = e2.position;

        if (auto* pin = findPinAt (pos))
        {
            auto connection = draggingConnector->connection;

            if (connection.source.nodeID == InternalNodeGraph::NodeID() && ! pin->isInput)
            {
                connection.source = pin->pin;
            }
            else if (connection.destination.nodeID == InternalNodeGraph::NodeID() && pin->isInput)
            {
                connection.destination = pin->pin;
            }

            if (graph.canConnect (connection))
            {
                pos = (pin->getParentComponent()->getPosition() + pin->getBounds().getCentre()).toFloat();
                draggingConnector->setTooltip (pin->getTooltip());
            }
        }

        if (draggingConnector->connection.source.nodeID == InternalNodeGraph::NodeID())
            draggingConnector->dragStart (pos);
        else
            draggingConnector->dragEnd (pos);
    }
}

void GraphEditorPanel::endDraggingConnector(const juce::MouseEvent& e)
{
    if (draggingConnector == nullptr)
        return;

    draggingConnector->setTooltip ({});

    auto e2 = e.getEventRelativeTo (this);
    auto connection = draggingConnector->connection;

    draggingConnector = nullptr;

    if (auto* pin = findPinAt (e2.position))
    {
        if (connection.source.nodeID == InternalNodeGraph::NodeID())
        {
            if (pin->isInput)
                return;

            connection.source = pin->pin;
        }
        else
        {
            if (! pin->isInput)
                return;

            connection.destination = pin->pin;
        }

        graph.addConnection (connection);
    }
}

GraphEditorPanel::NodeComponent* GraphEditorPanel::getComponentForNode(InternalNodeGraph::NodeID nodeID) const
{
    for (auto* fc : nodes)
       if (fc->nodeID == nodeID)
            return fc;

    return nullptr;
}

GraphEditorPanel::ConnectorComponent* GraphEditorPanel::getComponentForConnection(
	const InternalNodeGraph::Connection& conn) const
{
    for (auto* cc : connectors)
        if (cc->connection == conn)
            return cc;

    return nullptr;
}

GraphEditorPanel::PinComponent* GraphEditorPanel::findPinAt(juce::Point<float> pos) const
{
    for (auto* fc : nodes)
    {
        // NB: A Visual Studio optimiser error means we have to put this Component* in a local
        // variable before trying to cast it, or it gets mysteriously optimised away..
        auto* comp = fc->getComponentAt (pos.toInt() - fc->getPosition());

        if (auto* pin = dynamic_cast<PinComponent*> (comp))
            return pin;
    }

    return nullptr;
}
