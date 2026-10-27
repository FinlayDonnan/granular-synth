/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
AP_Ass3AudioProcessor::AP_Ass3AudioProcessor()
#ifndef JucePlugin_PreferredChannelConfigurations
     : AudioProcessor (BusesProperties()
                     #if ! JucePlugin_IsMidiEffect
                      #if ! JucePlugin_IsSynth
                       .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                      #endif
                       .withOutput ("Output", juce::AudioChannelSet::stereo(), true)
                     #endif
                       ),
#endif

// Load in AudioProcessorValueTreeState for plug in parameters
apvts (*this, nullptr, "PARAMETERS", createParameterLayout())
{
    
    // Create synthesiser sound from granular synth sound class
    synth.addSound (new GranularSynthSound());
    
    // Add chosen number of voices to synth, chosen here to be 12 to allow for interesting compositions
    for (int i=0; i<voiceCount; i++)
    {
        synth.addVoice (new GranularSynthVoice());  // add voice to synth
        
        // Get pointer to our newly created voice
        GranularSynthVoice* voicePtr = dynamic_cast<GranularSynthVoice*> (synth.getVoice(i));
        
        // Call function on that voice, passing in a reference to the preset manager
        voicePtr->connectParameters (apvts);
    }
    
    // Set pointers for reverb parameters
    reverbDryPtr = apvts.getRawParameterValue ("reverbDry");      // dry signal parameter
    reverbWetPtr = apvts.getRawParameterValue ("reverbWet");      // wet signal parameter
    reverbRoomPtr = apvts.getRawParameterValue ("reverbRoom");    // room size parameter
    reverbWidthPtr = apvts.getRawParameterValue ("reverbWidth");  // stereo width parameter
    
    // Set pointers for delay parameters
    delayTimePtr = apvts.getRawParameterValue ("delayTime");          // delay time parameter (in ms)
    delayFeedbackPtr = apvts.getRawParameterValue ("delayFeedback");  // feedback parameter
    delayDryPtr = apvts.getRawParameterValue ("delayDry");            // dry signal parameter
    delayWetPtr = apvts.getRawParameterValue ("delayWet");            // wet signal parameter
}

AP_Ass3AudioProcessor::~AP_Ass3AudioProcessor()
{
}

//==============================================================================
const juce::String AP_Ass3AudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool AP_Ass3AudioProcessor::acceptsMidi() const
{
   #if JucePlugin_WantsMidiInput
    return true;
   #else
    return false;
   #endif
}

bool AP_Ass3AudioProcessor::producesMidi() const
{
   #if JucePlugin_ProducesMidiOutput
    return true;
   #else
    return false;
   #endif
}

bool AP_Ass3AudioProcessor::isMidiEffect() const
{
   #if JucePlugin_IsMidiEffect
    return true;
   #else
    return false;
   #endif
}

double AP_Ass3AudioProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int AP_Ass3AudioProcessor::getNumPrograms()
{
    return 1;   // NB: some hosts don't cope very well if you tell them there are 0 programs,
                // so this should be at least 1, even if you're not really implementing programs.
}

int AP_Ass3AudioProcessor::getCurrentProgram()
{
    return 0;
}

void AP_Ass3AudioProcessor::setCurrentProgram (int index)
{
}

const juce::String AP_Ass3AudioProcessor::getProgramName (int index)
{
    return {};
}

void AP_Ass3AudioProcessor::changeProgramName (int index, const juce::String& newName)
{
}

//==============================================================================
void AP_Ass3AudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    // Use this method as the place to do any pre-playback
    // initialisation that you need..
    
    // Set sample rate for synth
    synth.setCurrentPlaybackSampleRate (sampleRate);
    
    // Load music box scratch sample into first buffer
    scratchRecording.loadIntoAudioBuffer(BinaryData::scratchRecording_wav, BinaryData::scratchRecording_wavSize, sampleBuffers[0]);
    
    // Load drum fill sample into second buffer
    drumsRecording.loadIntoAudioBuffer(BinaryData::deja_vu_drums_wav, BinaryData::deja_vu_drums_wavSize, sampleBuffers[1]);
    
    // Load song sample into third buffer
    cameronWinterRec.loadIntoAudioBuffer(BinaryData::ifYouTurnBackNow_wav, BinaryData::ifYouTurnBackNow_wavSize, sampleBuffers[2]);
    
    // Initialise and set up necessary parameters of all synth voices
    for (int i = 0; i < synth.getNumVoices(); ++i)
    {
        
        // dynamic_cast used to ensure we work with our synth voice and not a generic JUCE synth voice
        if (auto* voice = dynamic_cast<GranularSynthVoice*> (synth.getVoice(i)))
        {
            
            // Set sample rate and sample rate of envelope, set amount of grains, set up smoothing functions
            voice->prepare(sampleRate);
            
            // Set up sample buffers
            voice->setSampleBuffers(sampleBuffers);
            
            // Connect pointers for each parameter to their respective values in the AudioProcessorValueTreeState
            voice->connectParameters(apvts);
        }
    }
    
    // Initialise reverb
    reverb.reset();
    
    // Set maximum delay lengths for both delays
    delayLeft.setMaximumDelaySize(sampleRate);
    delayRight.setMaximumDelaySize(sampleRate);
}

void AP_Ass3AudioProcessor::releaseResources()
{
    // When playback stops, you can use this as an opportunity to free up any
    // spare memory, etc.
}

#ifndef JucePlugin_PreferredChannelConfigurations
bool AP_Ass3AudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
  #if JucePlugin_IsMidiEffect
    juce::ignoreUnused (layouts);
    return true;
  #else
    // This is the place where you check if the layout is supported.
    // In this template code we only support mono or stereo.
    // Some plugin hosts, such as certain GarageBand versions, will only
    // load plugins that support stereo bus layouts.
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
     && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

    // This checks if the input layout matches the output layout
   #if ! JucePlugin_IsSynth
    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
        return false;
   #endif

    return true;
  #endif
}
#endif

void AP_Ass3AudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    auto totalNumInputChannels  = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();

    // In case we have more outputs than inputs, this code clears any output
    // channels that didn't contain input data, (because these aren't
    // guaranteed to be empty - they may contain garbage).
    // This is here to avoid people getting screaming feedback
    // when they first compile a plugin, but obviously you don't need to keep
    // this code if your algorithm always overwrites all the output channels.
    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear (i, 0, buffer.getNumSamples());

    // Put output from the granular synth into the buffer
    synth.renderNextBlock(buffer, midiMessages, 0, buffer.getNumSamples());
    
    // Calculate the delay time in samples from the delay time in ms as in the apvts
    delayTimeSamples = (delayTimePtr->load() / 1000.0f) * sampleRate;
    
    // Set delay time in samples for both delays
    delayLeft.setDelayTimeInSamples(delayTimeSamples);
    delayRight.setDelayTimeInSamples(delayTimeSamples);
    
    // Get feedback value from parameter
    delayFeedback = delayFeedbackPtr->load();
    
    // Set feedback for both delays
    delayLeft.setFeedback(delayFeedback);
    delayRight.setFeedback(delayFeedback);
    
    // Get wet and dry values from parameters
    delayDry = delayDryPtr->load();
    delayWet = delayWetPtr->load();
    
    // Apply delay to sample in buffer. Left delay to left channel and right delay to right channel to preserve stereo
    for (int i = 0; i < buffer.getNumSamples(); i++)
    {
        
        // Separate left and right channels
        float left = buffer.getSample(0, i);
        float right = buffer.getSample(1, i);

        // Process left and right delays, applying to buffers
        float delayedLeft = delayLeft.process(left);
        float delayedRight = delayRight.process(right);

        // Mix wet and dry signals from parameters
        buffer.setSample(0, i, left * delayDry + delayedLeft * delayWet);
        buffer.setSample(1, i, right * delayDry + delayedRight * delayWet);
    }
    
    // Get reverb wet, dry, and stereo width parameters and set them in the reverb parameters
    reverbParams.dryLevel = reverbDryPtr->load();
    reverbParams.wetLevel = reverbWetPtr->load();
    reverbParams.width = reverbWidthPtr->load();
    
    // Get room size specification. 0 = room reverb, 1 = hall reverb, 2 = church reverb
    reverbRoomSpec = (int) reverbRoomPtr->load();
    
    // Switch statement used so as to not run unnecessary code
    switch (reverbRoomSpec)
    {
            
        // Room reverb
        case 0:
        {
            // Set room size, small for room reverb
            reverbRoomSize = 0.2f;
            
            // All break statements used so as to not run unnecessary code
            break;
        }
            
        // Hall reverb
        case 1:
        {
            // Set room size, medium for hall reverb
            reverbRoomSize = 0.6f;
            
            break;
        }
            
        // Church reverb
        case 2:
        {
            // Set room size, large for church reverb
            reverbRoomSize = 0.9f;
            
            break;
        }
            
    }
    
    // Set room size in reverb parameters
    reverbParams.roomSize = reverbRoomSize;
    
    // Apply parameters to reverb
    reverb.setParameters (reverbParams);
    
    // Apply reverb to buffer, preserving stereo
    reverb.processStereo (buffer.getWritePointer(0), buffer.getWritePointer(1), buffer.getNumSamples());

}

//==============================================================================
bool AP_Ass3AudioProcessor::hasEditor() const
{
    return true; // (change this to false if you choose to not supply an editor)
}

juce::AudioProcessorEditor* AP_Ass3AudioProcessor::createEditor()
{
    // return new AP_Ass3AudioProcessorEditor (*this);
    return new juce::GenericAudioProcessorEditor(*this);
}

//==============================================================================
void AP_Ass3AudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    // You should use this method to store your parameters in the memory block.
    // You could do that either as raw data, or use the XML or ValueTree classes
    // as intermediaries to make it easy to save and load complex data.
    
    // For setting presets
    auto state = apvts.copyState();
    std::unique_ptr<juce::XmlElement> xml (state.createXml());
    copyXmlToBinary (*xml, destData);
}

void AP_Ass3AudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    // You should use this method to restore your parameters from this memory block,
    // whose contents will have been created by the getStateInformation() call.
    
    // For getting set presets
    std::unique_ptr<juce::XmlElement> xmlState (getXmlFromBinary (data, sizeInBytes));
    if (xmlState.get() != nullptr)
    {
        if (xmlState->hasTagName (apvts.state.getType()))
        {
            apvts.replaceState (juce::ValueTree::fromXml (*xmlState));
        }
    }
}

//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new AP_Ass3AudioProcessor();
}
