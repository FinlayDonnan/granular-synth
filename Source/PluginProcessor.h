/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include <vector>
#include "FileLoader.h"
#include "GranularSynth.h"
#include "Delay.h"

//==============================================================================
/**
*/
class AP_Ass3AudioProcessor  : public juce::AudioProcessor
{
public:
    //==============================================================================
    AP_Ass3AudioProcessor();
    ~AP_Ass3AudioProcessor() override;

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

private:
    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AP_Ass3AudioProcessor)
    
    // Initialise and set sample rate
    float sampleRate = 44100.0;
    
    // Initialise our synth
    juce::Synthesiser synth;
    
    // Set number of voices, 12 to have enough for interesting composition but not to many to use unnecessary CPU
    int voiceCount = 12;
    
    // Manages file formats and creates an AudioFormatReader
    juce::AudioFormatManager formatManager;
    
    // Stores our audio data locally
    std::array<juce::AudioBuffer<float>, 3> sampleBuffers;
    
    // Initialise helper for scratch recording playback
    FileLoader scratchRecording;
    
    // Initialise helper for drum beat playback
    FileLoader drumsRecording;
    
    // Initialise helper for song excert playback
    FileLoader cameronWinterRec;
    
    // Initalise reverb and its parameters
    juce::Reverb reverb;
    juce::Reverb::Parameters reverbParams;
    
    // Initialise the room size specification (type of room) and its value for parameters (float between 0.0 and 1.0)
    int reverbRoomSpec;
    float reverbRoomSize = 0.0f;  // set default to 0.0
    
    // Initalise delays
    // Seperate delays for left and right channels to preserve stereo as our delay class only outputs mono
    Delay delayLeft;
    Delay delayRight;
    
    // Initialise delay parameters, each adequately explained by their variable names
    float delayTimeSamples;
    float delayFeedback;
    float delayDry;
    float delayWet;
    
    // Parameter manager
    juce::AudioProcessorValueTreeState apvts;
    
    // Initialise pointers for reverb parameters
    std::atomic<float>* reverbDryPtr;
    std::atomic<float>* reverbWetPtr;
    std::atomic<float>* reverbRoomPtr;
    std::atomic<float>* reverbWidthPtr;
    
    // Initialise pointers for delay parameters
    std::atomic<float>* delayTimePtr;
    std::atomic<float>* delayFeedbackPtr;
    std::atomic<float>* delayDryPtr;
    std::atomic<float>* delayWetPtr;
    
    // Initialise and set up our AudioProcessorValueTreeState for storing parameters for UI
    juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout()
    {
        
        // Initialise our parameter layout
        juce::AudioProcessorValueTreeState::ParameterLayout layout;
        
        // Sample selection parameter, default is drum sample
        // 0 = music box scratch sample, 1 = drum fill sample, 2 = song sample
        // Being between any of these integers blends the outputs of two samples together
        // eg. 1.3 means roughly 70% drums and 30% song
        // Using NormalisableRange with 0.01 step size to avoid UI or sound errors and to have sensible precision
        layout.add (std::make_unique<juce::AudioParameterFloat> (juce::ParameterID("sampleSelect", 1),
                                                                 "Sample blend",
                                                                 juce::NormalisableRange<float>(0.0f, 2.0f, 0.01f),
                                                                 1.0));
        
        // Volume parameter, default is 0.5
        // Very important parameter due to volume being very volatile in granular synths because of overlap
        layout.add (std::make_unique<juce::AudioParameterFloat> (juce::ParameterID("outputGain", 1),
                                                                 "Volume (be conscious!)",
                                                                 0.0,
                                                                 1.0,
                                                                 0.5));
        
        // Grain length parameter, default is 100ms
        layout.add (std::make_unique<juce::AudioParameterFloat> (juce::ParameterID("gLength", 1),
                                                                 "Grain Length (ms)",
                                                                 5.0,
                                                                 500.0,
                                                                 100.0));
        
        // Grain length parameter, default is 10 grains per second
        layout.add (std::make_unique<juce::AudioParameterFloat> (juce::ParameterID("gDensity", 1),
                                                                 "Grain Density",
                                                                 0.5,
                                                                 100.0,
                                                                 10.0));
        
        // Grain position adjusting parameter, default is 0
        // Position in sample is controlled by MiDi but this allowed for precise adjustments
        // NormalisableRange used with 0.001 step size used to ensure precise adjustment is possible
        // Without this, parameter was snapping from 0 to (-)0.01
        layout.add (std::make_unique<juce::AudioParameterFloat> (juce::ParameterID("gPos", 1),
                                                                 "Grain Position Adjust",
                                                                 juce::NormalisableRange<float>(-0.01f, 0.01f, 0.001f),
                                                                 0.0));
        
        // Grain position spray parameter, ie. amount of randomisation/jitter in grain position in sample, default is 1
        layout.add (std::make_unique<juce::AudioParameterInt> (juce::ParameterID("gSpray", 1),
                                                                 "Position spray",
                                                                 1,
                                                                 10000,
                                                                 10));
        
        // Stereo width parameter, ie. amount of random pan on each grain, default is 0
        layout.add (std::make_unique<juce::AudioParameterFloat> (juce::ParameterID("gStereoWidth", 1),
                                                                 "Stereo Width",
                                                                 0.0,
                                                                 1.0,
                                                                 0.0));
        
        // Pitch transpose parameter, can transpose sample between octave above and below (numbers are semitones)
        // Default is 0
        layout.add (std::make_unique<juce::AudioParameterInt> (juce::ParameterID("pitchShift", 1),
                                                                 "Pitch transpose",
                                                                 -12,
                                                                 12,
                                                                 0));
        
        // Detune parameter, ie. amount of random detune on each grain, default is 0
        layout.add (std::make_unique<juce::AudioParameterFloat> (juce::ParameterID("detuneAmount", 1),
                                                                 "Amount of detune",
                                                                 0.0,
                                                                 0.25,
                                                                 0.0));
        
        // Distortion level parameter, default is 1
        layout.add (std::make_unique<juce::AudioParameterFloat> (juce::ParameterID("distLevel", 1),
                                                                 "Distortion level",
                                                                 1.0,
                                                                 100.0,
                                                                 1.0));
        
        // Delay dry signal parameter, default is 1
        layout.add (std::make_unique<juce::AudioParameterFloat> (juce::ParameterID("delayDry", 1),
                                                                 "Delay dry level",
                                                                 0.0,
                                                                 1.0,
                                                                 1.0));
        
        // Delay wet signal parameter, default is 0
        layout.add (std::make_unique<juce::AudioParameterFloat> (juce::ParameterID("delayWet", 1),
                                                                 "Delay wet level",
                                                                 0.0,
                                                                 1.0,
                                                                 0.0));
        
        // Delay time parameter, default is 500ms
        layout.add (std::make_unique<juce::AudioParameterFloat> (juce::ParameterID("delayTime", 1),
                                                                 "Delay time (ms)",
                                                                 1.0,
                                                                 1000.0,
                                                                 500.0));
        
        // Delay feedback parameter, default is 0
        layout.add (std::make_unique<juce::AudioParameterFloat> (juce::ParameterID("delayFeedback", 1),
                                                                 "Delay feedback",
                                                                 0.0,
                                                                 0.99,
                                                                 0.0));
        
        // Reverb room size choice, returns 0 for room, 1 for hall, 2 for church, used in switch statement
        // Default is 0 (room reverb)
        layout.add (std::make_unique<juce::AudioParameterChoice> (juce::ParameterID("reverbRoom", 1),
                                                                  "Reverb room size",
                                                                  juce::StringArray({"Room", "Hall", "Church"}),
                                                                  0));
        
        // Reverb dry signal parameter, default is 1
        layout.add (std::make_unique<juce::AudioParameterFloat> (juce::ParameterID("reverbDry", 1),
                                                                 "Reverb dry level",
                                                                 0.0,
                                                                 1.0,
                                                                 1.0));
        
        // Reverb wet signal parameter, default is 0
        layout.add (std::make_unique<juce::AudioParameterFloat> (juce::ParameterID("reverbWet", 1),
                                                                 "Reverb wet level",
                                                                 0.0,
                                                                 1.0,
                                                                 0.0));
        
        // Reverb width parameter, default is 0
        layout.add (std::make_unique<juce::AudioParameterFloat> (juce::ParameterID("reverbWidth", 1),
                                                                 "Reverb width",
                                                                 0.0,
                                                                 1.0,
                                                                 0.0));
        
        // return all parameters
        return layout;
    }
};
