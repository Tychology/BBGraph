#include "GraphEditorPanel.h"

#include "CustomRange.h"

struct GraphEditorPanel::PinComponent : Component, juce::SettableTooltipClient
{
	PinComponent(GraphEditorPanel& p, InternalNodeGraph::NodeAndChannel pinToUse, bool isIn)
		: panel(p), graph(p.graph), pin(pinToUse), isInput(isIn)
	{
		setSize(16, 16);
	}

	void paint(juce::Graphics& g) override
	{
		const auto w = static_cast<float>(getWidth());
		const auto h = static_cast<float>(getHeight());

		const auto colour = juce::Colours::red;

		juce::Path p;
		p.addEllipse(w * 0.25f, h * 0.25f, w * 0.5f, h * 0.5f);
		p.addRectangle(w * 0.4f, isInput ? (0.5f * h) : 0.0f, w * 0.2f, h * 0.5f);

		g.setColour(colour);
		g.fillPath(p);
	}

	void mouseDown(const juce::MouseEvent& e) override
	{
		const InternalNodeGraph::NodeAndChannel dummy{ {}, 0 };

		panel.beginConnectorDrag(isInput ? dummy : pin,
			isInput ? pin : dummy,
			e);
	}

	void mouseDrag(const juce::MouseEvent& e) override
	{
		panel.dragConnector(e);
	}

	void mouseUp(const juce::MouseEvent& e) override
	{
		panel.endDraggingConnector(e);
	}

	GraphEditorPanel& panel;
	InternalNodeGraph& graph;
	InternalNodeGraph::NodeAndChannel pin;
	const bool isInput;

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PinComponent)
};

struct GraphEditorPanel::NodeComponent : Component, private juce::AsyncUpdater
{
	NodeComponent(GraphEditorPanel& p, InternalNodeGraph::NodeID id)
		: panel(p), graph(p.graph), nodeID(id), numInputs(graph.getNodeForId(nodeID)->getNumInputs()), numOutputs(graph.getNodeForId(nodeID)->getNumOutputs())
	{
		shadow.setShadowProperties(juce::DropShadow(juce::Colours::black.withAlpha(0.5f), 3, { 0, 1 }));
		setComponentEffect(&shadow);

		for (int i = 0; i < numInputs; ++i)
			addAndMakeVisible(pins.add(new PinComponent(panel, { nodeID, i }, true)));

		for (int i = 0; i < numOutputs; ++i)
			addAndMakeVisible(pins.add(new PinComponent(panel, { nodeID, i }, false)));
	}

	void mouseDown(const juce::MouseEvent& e) override
	{
		originalPos = localPointToGlobal(juce::Point<int>());

		toFront(false);

		if (e.mods.isPopupMenu())
			showPopupMenu();
	}

	void mouseDrag(const juce::MouseEvent& e) override
	{
		if (!e.mods.isPopupMenu())
		{
			auto pos = originalPos + e.getOffsetFromDragStart();

			if (getParentComponent() != nullptr)
				pos = getParentComponent()->getLocalPoint(nullptr, pos);

			pos += getLocalBounds().getCentre();

			panel.setNodePosition(nodeID,
				{ pos.x / static_cast<double>(getParentWidth()),
				  pos.y / static_cast<double>(getParentHeight()) });

			panel.updateComponents();
		}
	}

	bool hitTest(int x, int y) override
	{
		for (const auto* child : getChildren())
			if (child->getBounds().contains(x, y))
				return true;

		return x >= 3 && x < getWidth() - 6 && y >= pinSize && y < getHeight() - pinSize;
	}

	void paint(juce::Graphics& g) override
	{
		const auto boxArea = getLocalBounds().reduced(4, pinSize);

		const auto boxColour = findColour(juce::TextEditor::backgroundColourId);

		g.setColour(boxColour);
		g.fillRect(boxArea.toFloat());

		g.setColour(findColour(juce::TextEditor::textColourId));
		g.setFont(font);
		g.drawFittedText(getName(), boxArea, juce::Justification::centred, 2);
	}

	void resized() override
	{
		if (graph.getNodeForId(nodeID))
		{
			for (auto* pin : pins)
			{
				const bool isInput = pin->isInput;
				const auto channelIndex = pin->pin.channelIndex;
				constexpr int busIdx = 0;

				const int total = isInput ? numInputs : numOutputs;

				const auto totalSpaces = static_cast<float>(total);
				const auto indexPos = static_cast<float>(channelIndex) + (static_cast<float>(busIdx) * 0.5f);

				pin->setBounds(proportionOfWidth((1.0f + indexPos) / (totalSpaces + 1.0f)) - pinSize / 2,
					pin->isInput ? 0 : (getHeight() - pinSize),
					pinSize, pinSize);
			}
		}

		onResize();

		panel.updateConnectors();
	}

	virtual void onResize()
	{
	}

	juce::Point<float> getPinPos(int index, bool isInput) const
	{
		for (auto* pin : pins)
			if (pin->pin.channelIndex == index && isInput == pin->isInput)
				return getPosition().toFloat() + pin->getBounds().getCentre().toFloat();

		return {};
	}

	void update()
	{
		const InternalNodeGraph::Node::Ptr f(graph.getNodeForId(nodeID));
		jassert(f != nullptr);

		const auto p = panel.getNodePosition(nodeID);
		setCentreRelative(static_cast<float>(p.x), static_cast<float>(p.y));

		resized();
	}

	virtual void showPopupMenu() = 0;

	void handleAsyncUpdate() override { repaint(); }

	GraphEditorPanel& panel;
	InternalNodeGraph& graph;
	const InternalNodeGraph::NodeID nodeID;
	juce::OwnedArray<PinComponent> pins;
	int numInputs, numOutputs;
	int pinSize = 16;
	juce::Point<int> originalPos;
	juce::Font font{ 13.0f, juce::Font::bold };
	juce::DropShadowEffect shadow;
	std::unique_ptr<juce::PopupMenu> menu;
};

struct GraphEditorPanel::ConnectorComponent : Component, juce::SettableTooltipClient
{
	explicit ConnectorComponent(GraphEditorPanel& p)
		: panel(p), graph(p.graph)
	{
		setAlwaysOnTop(true);
	}

	void setInput(InternalNodeGraph::NodeAndChannel newSource)
	{
		if (connection.source != newSource)
		{
			connection.source = newSource;
			update();
		}
	}

	void setOutput(InternalNodeGraph::NodeAndChannel newDest)
	{
		if (connection.destination != newDest)
		{
			connection.destination = newDest;
			update();
		}
	}

	void dragStart(juce::Point<float> pos)
	{
		lastInputPos = pos;
		resizeToFit();
	}

	void dragEnd(juce::Point<float> pos)
	{
		lastOutputPos = pos;
		resizeToFit();
	}

	void update()
	{
		juce::Point<float> p1, p2;
		getPoints(p1, p2);

		if (lastInputPos != p1 || lastOutputPos != p2)
			resizeToFit();
	}

	void resizeToFit()
	{
		juce::Point<float> p1, p2;
		getPoints(p1, p2);

		const auto newBounds = juce::Rectangle<float>(p1, p2).expanded(4.0f).getSmallestIntegerContainer();

		if (newBounds != getBounds())
			setBounds(newBounds);
		else
			resized();

		repaint();
	}

	void getPoints(juce::Point<float>& p1, juce::Point<float>& p2) const
	{
		p1 = lastInputPos;
		p2 = lastOutputPos;

		if (const auto* src = panel.getComponentForNode(connection.source.nodeID))
			p1 = src->getPinPos(connection.source.channelIndex, false);

		if (const auto* dest = panel.getComponentForNode(connection.destination.nodeID))
			p2 = dest->getPinPos(connection.destination.channelIndex, true);
	}

	void paint(juce::Graphics& g) override
	{
		g.setColour(juce::Colours::red);

		g.fillPath(linePath);
	}

	bool hitTest(int x, int y) override
	{
		const auto pos = juce::Point<int>(x, y).toFloat();

		if (hitPath.contains(pos))
		{
			double distanceFromStart, distanceFromEnd;
			getDistancesFromEnds(pos, distanceFromStart, distanceFromEnd);

			// avoid clicking the connector when over a pin
			return distanceFromStart > 7.0 && distanceFromEnd > 7.0;
		}

		return false;
	}

	void mouseDown(const juce::MouseEvent&) override
	{
		dragging = false;
	}

	void mouseDrag(const juce::MouseEvent& e) override
	{
		if (dragging)
		{
			panel.dragConnector(e);
		}
		else if (e.mouseWasDraggedSinceMouseDown())
		{
			dragging = true;

			graph.removeConnection(connection);

			double distanceFromStart, distanceFromEnd;
			getDistancesFromEnds(getPosition().toFloat() + e.position, distanceFromStart, distanceFromEnd);
			const bool isNearerSource = (distanceFromStart < distanceFromEnd);

			InternalNodeGraph::NodeAndChannel dummy{ {}, 0 };

			panel.beginConnectorDrag(isNearerSource ? dummy : connection.source,
				isNearerSource ? connection.destination : dummy,
				e);
		}
	}

	void mouseUp(const juce::MouseEvent& e) override
	{
		if (dragging)
			panel.endDraggingConnector(e);
	}

	void resized() override
	{
		juce::Point<float> p1, p2;
		getPoints(p1, p2);

		lastInputPos = p1;
		lastOutputPos = p2;

		p1 -= getPosition().toFloat();
		p2 -= getPosition().toFloat();

		linePath.clear();
		linePath.startNewSubPath(p1);
		linePath.cubicTo(p1.x, p1.y + (p2.y - p1.y) * 0.33f,
			p2.x, p1.y + (p2.y - p1.y) * 0.66f,
			p2.x, p2.y);

		const juce::PathStrokeType wideStroke(8.0f);
		wideStroke.createStrokedPath(hitPath, linePath);

		const juce::PathStrokeType stroke(2.5f);
		stroke.createStrokedPath(linePath, linePath);

		const auto arrowW = 5.0f;
		const auto arrowL = 4.0f;

		juce::Path arrow;
		arrow.addTriangle(-arrowL, arrowW,
			-arrowL, -arrowW,
			arrowL, 0.0f);

		arrow.applyTransform(juce::AffineTransform()
			.rotated(juce::MathConstants<float>::halfPi - (float)atan2(p2.x - p1.x, p2.y - p1.y))
			.translated((p1 + p2) * 0.5f));

		linePath.addPath(arrow);
		linePath.setUsingNonZeroWinding(true);
	}

	void getDistancesFromEnds(juce::Point<float> p, double& distanceFromStart, double& distanceFromEnd) const
	{
		juce::Point<float> p1, p2;
		getPoints(p1, p2);

		distanceFromStart = p1.getDistanceFrom(p);
		distanceFromEnd = p2.getDistanceFrom(p);
	}

	GraphEditorPanel& panel;
	InternalNodeGraph& graph;
	InternalNodeGraph::Connection connection{ { {}, 0 }, { {}, 0 } };
	juce::Point<float> lastInputPos, lastOutputPos;
	juce::Path linePath, hitPath;
	bool dragging = false;

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ConnectorComponent)
};

struct GraphEditorPanel::ExpressionNodeComponent : NodeComponent
{
	ExpressionNodeComponent(GraphEditorPanel& p, InternalNodeGraph::NodeID id) : NodeComponent(p, id)
	{
		addAndMakeVisible(textBox);
		textBox.setMultiLine(false);
		textBox.setFont(font);

		auto* node = graph.getNodeForId(nodeID);
		textBox.setText(node->properties.getWithDefault("expression", ""));
		updateOutlineColor(node);

		textBox.onReturnKey = [this]()
		{
			unfocusAllComponents();
		};

		textBox.onFocusLost = [this]()
		{
			auto node = graph.getNodeForId(nodeID);
			auto expressionString = textBox.getText();

			if (node->properties.getWithDefault("expression", "").equals(expressionString)) return;

			node->properties.set("expression", expressionString);
			node->update();
			updateOutlineColor(node);
		};

		textBox.onTextChange = [this]()
		{
			updateWidth();
		};

		characterWidth = font.getStringWidthFloat("0");

		defaultWidth = characterWidth * 16 + pinSize * 2;
		maxWidth = characterWidth * 64 + pinSize * 2;

		setSize(defaultWidth, font.getHeight() + pinSize * 4);
		updateWidth();

	}

	void showPopupMenu() override
	{
		menu.reset(new juce::PopupMenu);
		menu->addItem(1, "Delete");
		menu->addItem(2, "Disconnect all pins");

		menu->showMenuAsync({}, juce::ModalCallbackFunction::create
		([this](int r) {
				switch (r)
				{
				case 1:   graph.removeNode(nodeID); break;
				case 2:   graph.disconnectNode(nodeID); break;

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
	void updateOutlineColor(InternalNodeGraph::Node* node)
	{
		if (node->properties.getWithDefault("validExpression", true))
		{
			textBox.setColour(textBox.outlineColourId, getLookAndFeel().findColour(textBox.outlineColourId));
			textBox.setColour(textBox.focusedOutlineColourId, getLookAndFeel().findColour(textBox.focusedOutlineColourId));
		}
		else
		{
			textBox.setColour(textBox.outlineColourId, juce::Colours::red);
			textBox.setColour(textBox.focusedOutlineColourId, juce::Colours::red);
		}
	}

	void updateWidth()
	{
		auto textWidth = font.getStringWidthFloat(textBox.getText());
		auto w = juce::jmin(juce::jmax(textWidth + characterWidth + pinSize * 2, defaultWidth), maxWidth);
		auto center = getBounds().getCentre();
		setSize(w, getHeight());
		setCentrePosition(center);
		panel.updateConnectors();
	}

	juce::TextEditor textBox;
	juce::Font font{ juce::Font::getDefaultMonospacedFontName(), 20, 0 };
	float characterWidth;
	float defaultWidth;
	float maxWidth = 0;
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
		const auto centre = getLocalBounds().getCentre();

		juce::Rectangle<float> outSymbolArea(centre.x - pinSize, centre.y - pinSize, pinSize * 2, pinSize * 2);

		outSymbol.setTransformToFit(outSymbolArea, juce::RectanglePlacement::centred);
	}

	void showPopupMenu() override
	{
		menu.reset(new juce::PopupMenu);
		menu->addItem(1, "Delete");
		menu->addItem(2, "Disconnect all pins");

		menu->showMenuAsync({}, juce::ModalCallbackFunction::create
		([this](int r) {
				switch (r)
				{
				case 1:   graph.removeNode(nodeID); break;
				case 2:   graph.disconnectNode(nodeID); break;

				}
			}));
	}

private:
	juce::DrawablePath outSymbol;
};

struct GraphEditorPanel::ParameterNodeComponent : NodeComponent
{
	ParameterNodeComponent(GraphEditorPanel& p, InternalNodeGraph::NodeID id, juce::AudioProcessorValueTreeState& apvts, juce::StringRef paramID) : NodeComponent(p, id),
		range(dynamic_cast<juce::AudioParameterFloat*>(apvts.getParameter(paramID))->range), paramName(apvts.getParameter(paramID)->getName(100))//, parameterID(paramID)
	{
		const auto* node = graph.getNodeForId(nodeID);

		log = node->properties.getWithDefault("log", false);
		range.start = node->properties.getWithDefault("start", 0.0);
		range.end = node->properties.getWithDefault("end", 255.0);

		if (log)
			makeLogarithmic();
		else
			makeLinear();

		addAndMakeVisible(paramSlider);
		paramSlider.setSliderStyle(juce::Slider::SliderStyle::RotaryHorizontalVerticalDrag);
		paramSlider.setTextBoxStyle(juce::Slider::TextEntryBoxPosition::TextBoxBelow, false, paramSlider.getTextBoxWidth(), paramSlider.getTextBoxHeight());
		
		sliderAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
			apvts, paramID, paramSlider);

		paramSlider.textFromValueFunction = [](double value)
		{
			return juce::String(value, 3);
		};


		addAndMakeVisible(minLabel);
		addAndMakeVisible(maxLabel);
		minLabel.setEditable(true);
		maxLabel.setEditable(true);

		minLabel.onTextChange = [this]()
		{
			updateStart(minLabel.getText().getFloatValue());
		};

		maxLabel.onTextChange = [this]()
		{
			updateEnd(maxLabel.getText().getFloatValue());
		};

		minLabel.setText(juce::String(range.start), juce::sendNotification);
		maxLabel.setText(juce::String(range.end), juce::sendNotification);

		addAndMakeVisible(paramNameLabel);
		paramNameLabel.setEditable(false);
		paramNameLabel.setJustificationType(juce::Justification::centred);
		paramNameLabel.setInterceptsMouseClicks(false, false);

		setSize(120, 200);
	}

	~ParameterNodeComponent() override
	{
	}

	void showPopupMenu() override
	{
		menu.reset(new juce::PopupMenu);
		menu->addItem(1, "Delete");
		menu->addItem(2, "Disconnect all pins");
		menu->addSeparator();
		if (log)
			menu->addItem(3, "Convert to linear parameter");
		else
			menu->addItem(4, "Convert to logarithmic parameter");

		menu->showMenuAsync({}, juce::ModalCallbackFunction::create
		([this](int r) {
				switch (r)
				{
				case 1: graph.removeNode(nodeID);       break;
				case 2: graph.disconnectNode(nodeID);   break;
				case 3: makeLinear();                   break;
				case 4: makeLogarithmic();              break;
				default:                                break;
				}
			}));
	}

	void onResize() override
	{
		auto bounds = getLocalBounds();

		bounds.removeFromTop(pinSize);
		paramNameLabel.setBounds(bounds.removeFromTop(pinSize * 2));
		auto labelArea = bounds.removeFromBottom(pinSize * 2.5);
		labelArea.removeFromBottom(pinSize * 1.5);
		labelArea.reduce(pinSize * 0.5, 0);

		bounds.reduce(pinSize * 1.5, 0);
		paramSlider.setBounds(bounds);

		minLabel.setBounds(labelArea.removeFromLeft(proportionOfWidth(0.5)));
		maxLabel.setBounds(labelArea);
	}

private:
	void updateStart(float newStart)
	{
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

		auto* node = graph.getNodeForId(nodeID);
		node->properties.set("start", newStart);
		node->properties.set("end", newEnd);

		minLabel.setText(juce::String(newStart), juce::dontSendNotification);
		maxLabel.setText(juce::String(newEnd), juce::dontSendNotification);

		paramSlider.repaint();
	}

	void updateEnd(float newEnd)
	{
		const auto newStart = range.start;

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

		auto* node = graph.getNodeForId(nodeID);
		node->properties.set("start", newStart);
		node->properties.set("end", newEnd);

		minLabel.setText(juce::String(newStart), juce::dontSendNotification);
		maxLabel.setText(juce::String(newEnd), juce::dontSendNotification);

		paramSlider.repaint();
	}

	void makeLinear()
	{
		auto start = range.start;
		auto end = range.end;
		if (start >= end) end = range.start + 0.01f;

		range = juce::NormalisableRange<float>(start, end);

		paramSlider.setNormalisableRange(juce::NormalisableRange<double>(start, end));
		paramSlider.repaint();

		paramNameLabel.setText(paramName + "\nlinear", juce::dontSendNotification);

		log = false;
		graph.getNodeForId(nodeID)->properties.set("log", false);
	}

	void makeLogarithmic()
	{
		auto start = range.start;
		auto end = range.end;
		if (start <= 0.f) start = 0.01f;
		if (start >= end) end = range.start + 0.01f;

		range = logRange<float>(start, end);

		paramSlider.setNormalisableRange(logRange<double>(start, end));
		paramSlider.repaint();

		minLabel.setText(juce::String(range.start), juce::dontSendNotification);
		paramNameLabel.setText(paramName + "\nlogarithmic", juce::dontSendNotification);

		log = true;
		graph.getNodeForId(nodeID)->properties.set("log", true);
	}

	bool log;

	juce::NormalisableRange<float>& range;
	juce::String paramName;
	juce::Slider paramSlider;
	std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> sliderAttachment;

	juce::Label minLabel, maxLabel, paramNameLabel;
};

GraphEditorPanel::GraphEditorPanel(juce::AudioProcessorValueTreeState& apvts, InternalNodeGraph& g)
	: graph(g), apvts(apvts)
{
	graph.addChangeListener(this);
	setOpaque(true);
}

GraphEditorPanel::~GraphEditorPanel()
{
	graph.removeChangeListener(this);
	draggingConnector = nullptr;
	nodes.clear();
	connectors.clear();
}

void GraphEditorPanel::createNewNode(NodeType nodeType, juce::Point<int> position)
{
	auto pos = position.toDouble() / juce::Point<double>((double)getWidth(), (double)getHeight());

	if (auto node = graph.addNode(nodeType))
	{
		node->properties.set("x", pos.x);
		node->properties.set("y", pos.y);
	}
}

void GraphEditorPanel::setNodePosition(InternalNodeGraph::NodeID nodeID, juce::Point<double> pos)
{
	if (auto* n = graph.getNodeForId(nodeID))
	{
		n->properties.set("x", juce::jlimit(0.0, 1.0, pos.x));
		n->properties.set("y", juce::jlimit(0.0, 1.0, pos.y));
	}
}

juce::Point<double> GraphEditorPanel::getNodePosition(InternalNodeGraph::NodeID nodeID) const
{
	if (auto* n = graph.getNodeForId(nodeID))
		return { static_cast<double> (n->properties["x"]),
				 static_cast<double> (n->properties["y"]) };

	return {};
}

void GraphEditorPanel::updateComponents()
{
	for (int i = nodes.size(); --i >= 0;)
		if (graph.getNodeForId(nodes.getUnchecked(i)->nodeID) == nullptr)
			nodes.remove(i);

	for (int i = connectors.size(); --i >= 0;)
		if (!graph.isConnected(connectors.getUnchecked(i)->connection))
			connectors.remove(i);

	for (auto* fc : nodes)
		fc->update();

	for (auto* cc : connectors)
		cc->update();

	for (const auto* f : graph.getNodes())
	{
		if (getComponentForNode(f->nodeID) == nullptr)
		{
			NodeComponent* comp;

			const auto type = static_cast<NodeType>(static_cast<int>(f->properties["type"]));

			switch (type)
			{
			case Void: continue;
			case Expression:
				comp = nodes.add(new ExpressionNodeComponent(*this, f->nodeID));
				break;
			case Output:
				comp = nodes.add(new OutputNodeComponent(*this, f->nodeID));
				break;
			case Parameter:
				comp = nodes.add(new ParameterNodeComponent(*this, f->nodeID, apvts, f->properties["parameterID"].toString()));
				break;
			default: continue;
			}

			addAndMakeVisible(comp);
			comp->update();
		}
	}

	for (auto& c : graph.getConnections())
	{
		if (getComponentForConnection(c) == nullptr)
		{
			auto* comp = connectors.add(new ConnectorComponent(*this));
			addAndMakeVisible(comp);

			comp->setInput(c.source);
			comp->setOutput(c.destination);
		}
	}

}

void GraphEditorPanel::updateConnectors()
{
	for (auto* cc : connectors)
		cc->update();
}

void GraphEditorPanel::showPopupMenu(juce::Point<int> position)
{
	menu.reset(new juce::PopupMenu);
	
	menu->addItem(NodeType::Expression, "Expression Node");
	menu->addItem(NodeType::Output, "Output Node");
	menu->addItem(NodeType::Parameter, "Parameter Node");
	menu->addSeparator();
	menu->addItem(4, "Clear graph");


	menu->showMenuAsync({},
		juce::ModalCallbackFunction::create([this, position](int r)
			{
				if (r > 0 && r <= 3)
					createNewNode(NodeType(r), position);
				else if (r == 4)
					graph.clear();

			}));
}

void GraphEditorPanel::beginConnectorDrag(InternalNodeGraph::NodeAndChannel source,
	InternalNodeGraph::NodeAndChannel dest, const juce::MouseEvent& e)
{
	auto* c = dynamic_cast<ConnectorComponent*> (e.originalComponent);
	connectors.removeObject(c, false);
	draggingConnector.reset(c);

	if (draggingConnector == nullptr)
		draggingConnector.reset(new ConnectorComponent(*this));

	draggingConnector->setInput(source);
	draggingConnector->setOutput(dest);

	addAndMakeVisible(draggingConnector.get());
	draggingConnector->toFront(false);

	dragConnector(e);
}

void GraphEditorPanel::dragConnector(const juce::MouseEvent& e)
{
	const auto e2 = e.getEventRelativeTo(this);

	if (draggingConnector != nullptr)
	{
		draggingConnector->setTooltip({});

		auto pos = e2.position;

		if (auto* pin = findPinAt(pos))
		{
			auto connection = draggingConnector->connection;

			if (connection.source.nodeID == InternalNodeGraph::NodeID() && !pin->isInput)
			{
				connection.source = pin->pin;
			}
			else if (connection.destination.nodeID == InternalNodeGraph::NodeID() && pin->isInput)
			{
				connection.destination = pin->pin;
			}

			if (graph.canConnect(connection))
			{
				pos = (pin->getParentComponent()->getPosition() + pin->getBounds().getCentre()).toFloat();
				draggingConnector->setTooltip(pin->getTooltip());
			}
		}

		if (draggingConnector->connection.source.nodeID == InternalNodeGraph::NodeID())
			draggingConnector->dragStart(pos);
		else
			draggingConnector->dragEnd(pos);
	}
}

void GraphEditorPanel::endDraggingConnector(const juce::MouseEvent& e)
{
	if (draggingConnector == nullptr)
		return;

	draggingConnector->setTooltip({});

	const auto e2 = e.getEventRelativeTo(this);
	auto connection = draggingConnector->connection;

	draggingConnector = nullptr;

	if (const auto* pin = findPinAt(e2.position))
	{
		if (connection.source.nodeID == InternalNodeGraph::NodeID())
		{
			if (pin->isInput)
				return;

			connection.source = pin->pin;
		}
		else
		{
			if (!pin->isInput)
				return;

			connection.destination = pin->pin;
		}

		graph.addConnection(connection);
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
		auto* comp = fc->getComponentAt(pos.toInt() - fc->getPosition());

		if (auto* pin = dynamic_cast<PinComponent*> (comp))
			return pin;
	}

	return nullptr;
}
