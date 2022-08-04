#include "PluginEditor.h"

#include "GraphEditorPanel.h"

ByteBeatNodeGraphAudioProcessorEditor::ByteBeatNodeGraphAudioProcessorEditor(ByteBeatNodeGraphAudioProcessor& p)
	: AudioProcessorEditor(&p), graph(p.graph), audioProcessor(p), topBar(p)
{
	addAndMakeVisible(viewport);
	viewport.setViewedComponent(new GraphEditorPanel(p.apvts, graph), true);
	viewport.getViewedComponent()->setSize(4000, 4000);
	setLookAndFeel(&laf);

	addAndMakeVisible(topBar);

	setResizable(true, true);

	getConstrainer()->setSizeLimits(600, 400, viewport.getViewedComponent()->getWidth(), viewport.getViewedComponent()->getHeight());

	auto& widthVar = p.apvts.state.getProperty("windowWidth");
	auto& heightVar = p.apvts.state.getProperty("windowHeight");

	const int width = widthVar.isVoid() ? 1280 : widthVar;
	const int height = heightVar.isVoid() ? 720 : heightVar;

	setBoundsConstrained(juce::Rectangle<int>(getX(), getY(), width, height));
}

ByteBeatNodeGraphAudioProcessorEditor::~ByteBeatNodeGraphAudioProcessorEditor()
{
	setLookAndFeel(nullptr);
}

void ByteBeatNodeGraphAudioProcessorEditor::paint(juce::Graphics& g)
{
	g.fillAll(getLookAndFeel().findColour(juce::ResizableWindow::backgroundColourId));
}

void ByteBeatNodeGraphAudioProcessorEditor::resized()
{
	auto bounds = getLocalBounds();
	topBar.setBounds(bounds.removeFromTop(75).withWidth(viewport.getViewedComponent()->getWidth()));

	viewport.setBounds(bounds);
	audioProcessor.apvts.state.setProperty("windowWidth", getWidth(), nullptr);
	audioProcessor.apvts.state.setProperty("windowHeight", getHeight(), nullptr);
}
