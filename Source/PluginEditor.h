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
            bpmLabel.setText(juce::String(value) + " bpm", juce::dontSendNotification);
        };

        addAndMakeVisible(bpmLabel);

        updateBPM();


        addAndMakeVisible(attackSlider);
        attackSlider.setTextBoxStyle(attackSlider.NoTextBox, false, 0,0);
        attackSlider.setSliderStyle(attackSlider.RotaryHorizontalVerticalDrag);
        attackSliderAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(audioProcessor.apvts, "Attack", attackSlider);

        addAndMakeVisible(decaySlider);
    	decaySlider.setTextBoxStyle(decaySlider.NoTextBox, false, 0,0);
    	decaySlider.setSliderStyle(attackSlider.RotaryHorizontalVerticalDrag);
        decaySliderAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(audioProcessor.apvts, "Decay", decaySlider);

        addAndMakeVisible(sustainSlider);
    	sustainSlider.setTextBoxStyle(sustainSlider.NoTextBox, false, 0,0);
    	sustainSlider.setSliderStyle(attackSlider.RotaryHorizontalVerticalDrag);
        sustainSliderAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(audioProcessor.apvts, "Sustain", sustainSlider);

        addAndMakeVisible(releaseSlider);
    	releaseSlider.setTextBoxStyle(releaseSlider.NoTextBox, false, 0,0);
    	releaseSlider.setSliderStyle(attackSlider.RotaryHorizontalVerticalDrag);
        releaseSliderAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(audioProcessor.apvts, "Release", releaseSlider);

        addAndMakeVisible(volumeSlider);
        volumeSlider.setTextBoxStyle(volumeSlider.NoTextBox, false, 0,0);
        volumeSlider.setSliderStyle(attackSlider.RotaryHorizontalVerticalDrag);
        volumeSliderAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(audioProcessor.apvts, "Volume", volumeSlider);


        addAndMakeVisible(adsrLabel);
        adsrLabel.setText("ADSR", juce::dontSendNotification);

        addAndMakeVisible(volumeLabel);
        volumeLabel.setText("Volume", juce::dontSendNotification);

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
       
        syncToHostButton.setBounds(bounds.removeFromLeft(120));
        bpmLabel.setBounds(bounds.removeFromLeft(75));
        bpmLabel.setBounds(bpmLabel.getBounds().reduced(0, 20));
        bpmLabel.setEditable(true);

        bounds.removeFromLeft(50);
        adsrLabel.setBounds(bounds.removeFromLeft(75));

        attackSlider.setBounds(bounds.removeFromLeft(75));
        decaySlider.setBounds(bounds.removeFromLeft(75));
        sustainSlider.setBounds(bounds.removeFromLeft(75));
        releaseSlider.setBounds(bounds.removeFromLeft(75));

        bounds.removeFromLeft(50);
        volumeLabel.setBounds(bounds.removeFromLeft(75));
        volumeSlider.setBounds(bounds.removeFromLeft(75));
	}

    void updateBPM ()
    {
	     bpmLabel.setText(juce::String(audioProcessor.beatsPerMinute.get()) + " bpm", juce::dontSendNotification);
    }

    void changeListenerCallback(juce::ChangeBroadcaster* source) override
    {
	    updateBPM();
    }

private:

    juce::ToggleButton syncToHostButton;
    juce::Label bpmLabel;

    juce::Slider attackSlider;
    juce::Slider decaySlider;
    juce::Slider sustainSlider;
    juce::Slider releaseSlider;
    juce::Slider volumeSlider;

    juce::Label adsrLabel;

    juce::Label volumeLabel;

    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> attackSliderAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> decaySliderAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> sustainSliderAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> releaseSliderAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> volumeSliderAttachment;



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

