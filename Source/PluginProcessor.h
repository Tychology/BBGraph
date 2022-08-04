/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "InternalNodeGraph.h"
#include "ParameterManager.h"
#include "SynthVoice.h"


//class InternalNodeGraph;

//==============================================================================
/**
*/
class ByteBeatNodeGraphAudioProcessor  : public juce::AudioProcessor , public juce::ChangeBroadcaster

{
public:
    //==============================================================================
    ByteBeatNodeGraphAudioProcessor();
    ~ByteBeatNodeGraphAudioProcessor() override;

    //==============================================================================
    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

   #ifndef JucePlugin_PreferredChannelConfigurations
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
   #endif

    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;


    //==============================================================================
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    //==============================================================================
    const juce::String getName() const override;

    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;

    //==============================================================================
    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int index, const juce::String& newName) override;

    //==============================================================================
    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

   /* InternalNodeGraph& getGraph() { return graph; }
    juce::AudioProcessorValueTreeState& getApvts() {return apvts;}*/

    void setNodeProcessorSequence(GraphRenderSequence& sequence);


    juce::Atomic<double> beatsPerMinute{0};
    juce::Atomic<bool> syncToHost{false};


private:
    //==============================================================================
    ParameterManager parameterManager;

public:
    juce::AudioProcessorValueTreeState apvts;
    InternalNodeGraph graph;

private:
    juce::Synthesiser synth;

    //juce::AudioPlayHead::CurrentPositionInfo positionInfo;

    juce::AudioProcessorValueTreeState::ParameterLayout createParameters();

    double freeSeconds;
    double freeSamples;


    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ByteBeatNodeGraphAudioProcessor)
};
