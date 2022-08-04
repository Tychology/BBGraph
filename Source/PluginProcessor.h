#pragma once

#include <JuceHeader.h>

#include "InternalNodeGraph.h"
#include "ParameterManager.h"

class ByteBeatNodeGraphAudioProcessor  : public juce::AudioProcessor , public juce::ChangeBroadcaster
{
public:
    //==============================================================================
    ByteBeatNodeGraphAudioProcessor();
    ~ByteBeatNodeGraphAudioProcessor() override = default;

    //==============================================================================
    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;

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
    
    void setNodeProcessorSequence(GraphRenderSequence& sequence);
    
    juce::Atomic<double> beatsPerMinute{0};
    juce::Atomic<bool> syncToHost{false};
    
    juce::AudioProcessorValueTreeState apvts;

private:
    ParameterManager parameterManager;
public:
    InternalNodeGraph graph;
private:

    juce::Synthesiser synth;
    
    juce::AudioProcessorValueTreeState::ParameterLayout createParameters() const;

    double freeSeconds = 0;
    double freeSamples = 0;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ByteBeatNodeGraphAudioProcessor)
};
