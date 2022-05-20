/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "GraphEditorPanel.h"
#include "InternalNodeGraph.h"
#include "PluginProcessor.h"

//==============================================================================
/**
*/
class TopBarComponent : public juce::Component, public juce::ChangeListener
{
public:
    TopBarComponent(ByteBeatNodeGraphAudioProcessor& p) : audioProcessor(p)
    {
        audioProcessor.addChangeListener(this);

	    addAndMakeVisible(syncToHostButton);
        syncToHostButton.setButtonText("Sync to host");
        syncToHostButton.setState(audioProcessor.syncToHost.get() ? juce::ToggleButton::buttonDown : juce::ToggleButton::buttonNormal);

        syncToHostButton.onStateChange = [this] ()
        {
            if (syncToHostButton.getToggleState())
            {
	            bpmLabel.setEditable(false);
                audioProcessor.syncToHost.set(true);
                bpmLabel.setColour(juce::Label::ColourIds::outlineColourId, juce::Colours::grey);
            }
            else
            {
	            bpmLabel.setEditable(true);
                audioProcessor.syncToHost.set(false);
                bpmLabel.setColour(juce::Label::ColourIds::outlineColourId, juce::Colours::white);
            }
        };

        bpmLabel.onTextChange = [this] ()
        {
            auto value = bpmLabel.getText().getFloatValue();
            value = juce::jlimit(10.f, 400.f, value);
            audioProcessor.beatsPerMinute.set(value);
            bpmLabel.setText(juce::String(value), juce::dontSendNotification);
        };



        addAndMakeVisible(bpmLabel);
        updateBPM();
    }

    ~TopBarComponent() override
    {
	    audioProcessor.removeChangeListener(this);
    }

	void paint (juce::Graphics&) override
	{
	}


    void resized() override
	{
        auto bounds = getLocalBounds();
        syncToHostButton.setBounds(bounds.removeFromLeft(bounds.getWidth() / 2));
        bpmLabel.setBounds(bounds);
	}

    void updateBPM ()
    {
	     bpmLabel.setText(juce::String(audioProcessor.beatsPerMinute.get()), juce::dontSendNotification);
    }

    void changeListenerCallback(juce::ChangeBroadcaster* source) override
    {
	    updateBPM();
    }

private:

    juce::ToggleButton syncToHostButton;
    juce::Label bpmLabel;
    //float bpm = 0;

    ByteBeatNodeGraphAudioProcessor& audioProcessor;
};

class ByteBeatNodeGraphAudioProcessorEditor  : public juce::AudioProcessorEditor
{
public:
    ByteBeatNodeGraphAudioProcessorEditor (ByteBeatNodeGraphAudioProcessor&);
    ~ByteBeatNodeGraphAudioProcessorEditor() override;

    //==============================================================================
    void paint (juce::Graphics&) override;
    void resized() override;

private:
    // This reference is provided as a quick way for your editor to
    // access the processor object that created it.
    juce::Viewport viewport;
    InternalNodeGraph& graph;
    TopBarComponent topBar;
    ByteBeatNodeGraphAudioProcessor& audioProcessor;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ByteBeatNodeGraphAudioProcessorEditor)
};

