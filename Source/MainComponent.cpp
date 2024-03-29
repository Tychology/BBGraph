#include "MainComponent.h"

//==============================================================================
MainComponent::MainComponent() : viewport("")
{
    addAndMakeVisible(viewport);
    viewport.setViewedComponent(new GraphEditorPanel(graph), true);
    viewport.getViewedComponent()->setSize(1000, 1000);

    setSize (600, 400);
}

MainComponent::~MainComponent()
{
}

//==============================================================================
void MainComponent::paint (juce::Graphics& g)
{
    // (Our component is opaque, so we must completely fill the background with a solid colour)
    g.fillAll (getLookAndFeel().findColour (juce::ResizableWindow::backgroundColourId));

    g.setFont (juce::Font (16.0f));
    g.setColour (juce::Colours::white);
    g.drawText ("Hello World!", getLocalBounds(), juce::Justification::centred, true);
}

void MainComponent::resized()
{
    viewport.setBounds(getLocalBounds());

    // This is called when the MainComponent is resized.
    // If you add any child components, this is where you should
    // update their positions.
}
