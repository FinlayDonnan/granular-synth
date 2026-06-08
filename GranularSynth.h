/*
  ==============================================================================

    GranularSynth.h
    Created: 10 Apr 2026 2:39:45am
    Author:  Finlay Donnan UoE

  ==============================================================================
*/

#pragma once

/**
 Create JUCE synth sound class for our granular synth.
 */
class GranularSynthSound : public juce::SynthesiserSound
{
public:
    /**
     Returns true if this sound should be played when a given midi note is pressed
     The Synthesiser will use this information when deciding which sounds to trigger for a given note
     Set as true always as we always want our synth to play when a key is pressed
     
     @param midiNoteNumber note number of midi key pressed
     @return true
    */
    bool appliesToNote (int midiNoteNumber) override
    {
        return true;
    }

    /**
     Returns true if the sound should be triggered by midi events on a given channel
     The Synthesiser will use this information when deciding which sounds to trigger for a given note
     Set as true always as we always want our synth to play at midi events on a channel
     
     @param midiChannel given channel
     @return true
    */
    bool appliesToChannel (int midiChannel) override
    {
        return true;
    }
};

/**
 Creates JUCE synth voice class for our granular synth. This is where the granular synth programming will go.
 */
class GranularSynthVoice : public juce::SynthesiserVoice
{
public:
    
    /**
     Initialise sample rate (for project and JUCE envelope)
     Set number of grains
     Initialise smoothing functions for output volume, grain length, grain density, and distortion level
     
     @param _SR sample rate
     */
    void prepare(float _SR)
    {
        sampleRate = _SR;  // set sample rate

        grains.resize (128);  // set number of grains

        envelope.setSampleRate (_SR);  // set sample rate for JUCE envelope
        
        // Initialise smoothing function for output volume, setting smoothing time to 50ms
        smoothVol.reset (_SR, 0.05f);
        smoothVol.setCurrentAndTargetValue (1.0f);        // default as 1
        
        // Initialise smoothing function for grain length, setting smoothing time to 20ms
        smoothGLength.reset (_SR, 0.02f);
        smoothGLength.setCurrentAndTargetValue (100.0f);  // default same as in parameter
        
        // Initialise smoothing function for grain density, setting smoothing time to 30ms
        smoothGDensity.reset (_SR, 0.03f);
        smoothGDensity.setCurrentAndTargetValue (10.0f);  // default same as in parameter
        
        // Initialise smoothing function for distortion level, setting smoothing time to 50ms
        smoothDistLevel.reset (_SR, 0.05f);
        smoothDistLevel.setCurrentAndTargetValue (1.0f);  // default same as in parameter
    }
    
    /**
     Set pointers to our 3 buffers
     
     @param buffers our buffers
     */
    void setSampleBuffers(std::array<juce::AudioBuffer<float>, 3>& buffers)
    {
        sampleBuffersPtr = &buffers;
    }
    
    /**
     Connect our UI parameters to their respective pointers
     
     @param apvts our AudioProcessorValueTreeState
     */
    void connectParameters (juce::AudioProcessorValueTreeState& apvts)
    {
        
        // Connect grain parameters
        gLengthParamPtr = apvts.getRawParameterValue("gLength");
        gDensityParamPtr = apvts.getRawParameterValue("gDensity");
        gPosParamPtr = apvts.getRawParameterValue("gPos");
        gSprayParamPtr = apvts.getRawParameterValue("gSpray");
        gStereoWidthPtr = apvts.getRawParameterValue("gStereoWidth");
        pitchShiftPtr = apvts.getRawParameterValue("pitchShift");
        detuneAmountPtr = apvts.getRawParameterValue("detuneAmount");
        sampleSelectPtr = apvts.getRawParameterValue("sampleSelect");
        
        // Connect volume parameter
        outputGainPtr = apvts.getRawParameterValue("outputGain");
        
        // Connect distortion parameter
        distLevelPtr = apvts.getRawParameterValue("distLevel");
    }
    
    /**
     Called to start a new note
     This will be called during the rendering callback, so must be fast and thread-safe
     
     @param midiNoteNumber midi key pressed
     @param velocity how hard key was pressed (not used)
     @param sound pointer to a synth sound
     @param currentPitchWheelPosition pitch wheel position (not used)
    */
    virtual void startNote (int midiNoteNumber,
                            float velocity,
                            juce::SynthesiserSound* sound,
                            int currentPitchWheelPosition) override
    {
        
        isPlaying = true;  // register that note is playing
        
        // Calculate range of available MiDi notes
        float noteRange = (float) (maxMidiNote - minMidiNote);

        // Limit number of MiDi note pressed to be within the min and max
        float clampedNote = juce::jlimit((float) minMidiNote, (float) maxMidiNote, (float) midiNoteNumber);

        // Converts the current MiDi note to position in range 0 to 1 across the allowed keyboard range
        float normPos = (clampedNote - minMidiNote) / noteRange;
        
        // Set position of grain in sample using this normalised position
        positionFromMidi = normPos;

        // Set envelope sample rate
        envelope.setSampleRate(getSampleRate());
        
        // Initialise envelope parameters
        juce::ADSR::Parameters envelopeParams;
        
        // Set envelope parameters
        // Set only to avoid clicking, not used as parameters as not very interesting for this project
        envelopeParams.attack = 0.1;
        envelopeParams.decay = 0.1;
        envelopeParams.sustain = 0.9;
        envelopeParams. release = 0.1;
        
        // Apply envelope parameters
        envelope.setParameters(envelopeParams);
        
        // Begin playing
        envelope.noteOn();
    }

    /**
     Called to stop a note
     This will be called during the rendering callback, so must be fast and thread-safe
     
     @param velocity how quickly note was released, 0 is slowly, 1 is quickly (not used)
     @param allowTailOff false for no tail-off, true for yes tail-off (not used)
    */
    void stopNote (float velocity, bool allowTailOff) override
    {
        // End playing
        envelope.noteOff();
    }
    
    /**
     Renders the next block of data for this voice
     Adds output data to current contents of provided buffer
     Returns immediately is voice is currently silent
     Calls clearCurrentNote() to tell synth that voice has finished playing
     
     @param outputBuffer buffer to add data to
     @param startSample start position
     @param numSamples number of samples in buffer
    */
    void renderNextBlock (juce::AudioBuffer<float>& outputBuffer,
                          int startSample,
                          int numSamples) override
    {
        // Check if voice is playing
        if (isPlaying)
        {
            
            // Get grain parameter values from pointers
            grainLengthMs = gLengthParamPtr->load();
            grainDensity = gDensityParamPtr->load();
            positionInSample = gPosParamPtr->load();
            spray = gSprayParamPtr->load();
            stereoWidth = gStereoWidthPtr->load();
            pitchShift = pitchShiftPtr->load();
            detuneAmount = detuneAmountPtr->load();
            
            // Get volume parameter value from pointer
            outputGain = outputGainPtr->load();
            
            // Get distortion parameter value from pointer
            distLevel = distLevelPtr->load();
            
            // Input these values into the relevent smoothing function
            smoothVol.setTargetValue (outputGain);
            smoothGLength.setTargetValue (grainLengthMs);
            smoothGDensity.setTargetValue (grainDensity);
            smoothDistLevel.setTargetValue (distLevel);
            
            // DSP
            // Only the region of the buffer between startSample and (startSample + numSamples) should be altered by this method
            for (int i=startSample; i<startSample + numSamples; i++)
            {
                // Get smoothed parameter values from their respective smoothing functions
                float smoothVolParam = smoothVol.getNextValue();
                grainLengthMs = smoothGLength.getNextValue();
                grainDensity = smoothGDensity.getNextValue();
                distLevel = smoothDistLevel.getNextValue();
                
                // Check sample selection pointer is not null and get the blended value
                if (sampleSelectPtr)
                    sampleBlend = sampleSelectPtr->load();
                    
                // Limit sample blend value to ensure it relates to the samples
                // (eg. anything <0 or >2 does not relate to any sample)
                sampleBlend = juce::jlimit(0.0f, 2.0f, sampleBlend);
                
                //---Linear interpolation---
                // Implemented to be able to blend samples
                // Pretty much same implementation as for delay
                int indexA = floor (sampleBlend);
                int indexB = juce::jmin (indexA + 1, 2);
                
                float frac = sampleBlend - (float) indexA;
                //--------------------------
                
                // Initialise semitone specifier for pitch transpose
                float semitones = 0.0f;

                // If pitch transpose is not 0, specify amount of semitones to transpose
                if (pitchShift)
                    semitones = pitchShift;

                // Calculate pitch ratio, used to transpose necessary amount
                pitchRatio = std::pow(2.0f, semitones / 12.0f);
                
                // Get JUCE envelope val (0-1)
                float envVal = envelope.getNextSample();
                
                // Call updateGrainSpawning function to begin spawning grains
                updateGrainSpawning();

                // Initialise sample values
                float sampleValue = 0.0f;
                float leftSample = 0.0f;
                float rightSample = 0.0f;
                
                // Initialise active grain counter
                int activeGrainCount = 0;

                // First loop through counts active grains
                for (auto& g : grains)
                {
                    // Check is grain is active and not at end
                    if (g.active && g.position < g.length)
                        activeGrainCount++;  // increment counter
                }

                // Second loop through processes the grains
                for (auto& g : grains)
                {
                    // If grain is not active, next iteration in loop
                    if (!g.active)
                        continue;
                    
                    // If position has reached end of grain
                    if (g.position >= g.length)
                    {
                        g.active = false;  // specify than grain is not playing
                        continue;          // next iteration in loop
                    }
                    
                    // Calculate phase for Hann window envelope
                    float phase = (float) g.position / (float) g.length;
                    
                    // Get envelope value from function
                    float env = envHann(phase);
                    
                    // Calculate read position
                    int readPos = g.start + g.position;
                    
                    // Initialise two buffers as required for linear interpolation and sample blending
                    auto& bufferA = (*sampleBuffersPtr)[indexA];
                    auto& bufferB = (*sampleBuffersPtr)[indexB];

                    // If read position is larger than the size of either buffers, next iteration in loop
                    if (readPos >= bufferA.getNumSamples() || readPos >= bufferB.getNumSamples())
                        continue;

                    // Fill buffers with sample
                    float sA = bufferA.getSample(0, readPos);
                    float sB = bufferB.getSample(0, readPos);
                    
                    // Gives a smoother and more natural sounding blend of signal from each sample
                    float gainA = std::cos(frac * juce::MathConstants<float>::halfPi);
                    float gainB = std::sin(frac * juce::MathConstants<float>::halfPi);

                    // Linear interpolated sample
                    float s = sA * gainA + sB * gainB;
                    
                    // Add sample to sample value, wrapping with envelope and applying granular gain
                    sampleValue += s * env * g.gain;
                    
                    // Allow width of granular pan to be adjustable in UI
                    float pan = g.pan * stereoWidth;
                    
                    // Calculate gain for each channel use pan value
                    // sine and cosine used to ensure smooth volume with pan
                    // arguments of sine and cosine adjusted to be in range [0, pi/2]
                    float leftGain  = std::cos(0.25f * juce::MathConstants<float>::pi * (pan + 1.0f));
                    float rightGain = std::sin(0.25f * juce::MathConstants<float>::pi * (pan + 1.0f));
                    
                    // Fill each sample with its respective volume controlled sample value
                    leftSample += sampleValue * leftGain;
                    rightSample += sampleValue * rightGain;
                    
                    // Apply JUCE envelope to samples
                    leftSample *= envVal;
                    rightSample *= envVal;
                    
                    // Apply tanh distortion to samples
                    leftSample = tanhDistortion(leftSample, distLevel);
                    rightSample = tanhDistortion(rightSample, distLevel);
                    
                    // Apply smoothed volume parameter
                    leftSample *= smoothVolParam;
                    rightSample *= smoothVolParam;
                    
                    // Increment grain position
                    g.position += g.playbackRate;
                }
                
                // Add each sample to its respective channel in the output buffer
                outputBuffer.addSample(0, i, leftSample);
                outputBuffer.addSample(1, i, rightSample);
                
                // Check if JUCE envelope is not active, that is, the release has finished
                if (!envelope.isActive())
                {
                    // Free up this voice
                    isPlaying = false;
                    clearCurrentNote();
                }
            }
        }
    }
    
    /**
     Must return true if this voice object is capable of playing the given sound
     A typical implementation of this method may just return true if there's only one type of voice and sound, this is what happens here
     
     @return true
    */
    bool canPlaySound (juce::SynthesiserSound*) override
    {
        return true;
    }

    /**
     Called to let the voice know that the pitch wheel has been moved
     This will be called during the rendering callback, so must be fast and thread-safe
     Not used for this project
    */
    void pitchWheelMoved (int newPitchWheelValue) override
    {}

    /**
     Called to let the voice know that a midi controller has been moved
     This will be called during the rendering callback, so must be fast and thread-safe
     Not used for this project
    */
    void controllerMoved (int controllerNumber, int newControllerValue) override
    {}
    
private:
        
    // Initialise and set sample rate
    float sampleRate = 44100.0;
        
    // Manages file formats and creates an AudioFormatReader
    juce::AudioFormatManager formatManager;
        
    // Initialise pointer to array holding 3 sample buffers, one for each of the 3 samples
    std::array<juce::AudioBuffer<float>, 3>* sampleBuffersPtr;
    
    // Check is a note is playing
    bool isPlaying = false;
    
    // Initialise grain parameter pointers
    std::atomic<float>* gLengthParamPtr;
    std::atomic<float>* gDensityParamPtr;
    std::atomic<float>* gPosParamPtr;
    std::atomic<float>* gSprayParamPtr;
    std::atomic<float>* gStereoWidthPtr;
    std::atomic<float>* pitchShiftPtr;
    std::atomic<float>* detuneAmountPtr;
    std::atomic<float>* sampleSelectPtr;
    
    // Initialise volume parameter pointer
    std::atomic<float>* outputGainPtr;
    
    // Initialise distortion parameter pointer
    std::atomic<float>* distLevelPtr;
    
    // Initialise smoothing functions for output volume, grain length and density, and distortion level
    juce::SmoothedValue<float> smoothVol;
    juce::SmoothedValue<float> smoothGLength;
    juce::SmoothedValue<float> smoothGDensity;
    juce::SmoothedValue<float> smoothDistLevel;
    
    // Initialise JUCE envelope
    juce::ADSR envelope;
    
    // Initalise pitch ratio, used in implementing pitch transpose, default is 1
    float pitchRatio = 1.0f;
    
    // Set maximum and minimum midi notes
    // These are the lowest and highest notes on the keyboard that this synth will play
    int minMidiNote = 36;  // C2
    int maxMidiNote = 84;  // C6
    
    // Position in sample dictated by MiDi note, 0 is start of sample, 1 is end, default is 0.5 (halfway through)
    float positionFromMidi = 0.5f;
    
    // Initialise a JUCE random variable
    juce::Random random;
        
    // Grain structure with necessary parameters
    // Struct to be able to group all basic variables for each grain in one place
    struct Grain
    {
        float position = 0.0f;      // playback position in grain
        float start = 0.0f;         // position in sample for start of grain
        float length = 0.0f;        // length of grain (in samples)
        bool active = false;        // check if grain is playing
        float pan = 0.0f;           // -1 = left, +1 = right
        float playbackRate = 1.0f;  // rate of playback through grain, default 1 is normal speed
        float gain = 0.5f;          // volume of grain
    };
    
    // Initialise a vector of grains
    std::vector<Grain> grains;
    
    // Initialise parameters for sliders in UI
    float grainLengthMs;     // length of each grain in ms
    float grainDensity;      // grains per second
    float positionInSample;  // where in sample grain is taken from (from 0 (start) to 1 (end))
    int spray;               // size of range in samples in which grains can spawn
    float stereoWidth;       // amount of random panning, 0 = mono, 1 = full width
    float outputGain;        // overall output volume
    int pitchShift;          // transpose parameter
    float detuneAmount;        // amount of random detuning of each grain
    float distLevel;         // amount of distortion
    // blend of samples, 0 = music box scratch, 1 = drums, 2 = song, anything in between is a mix of two
    float sampleBlend = 0.0f;
    
    /**
     Create smooth envelope using a Hann (or Hanning) window
     
     @param phase goes from 0 to 1 across the grain. phase = position / length
     @return envelope with Hann window
     */
    float envHann (float phase)
    {
        return 0.5f * (1.0f - std::cos(2.0f * juce::MathConstants<float>::pi * phase));  // formula for Hann window
    }
        
    /**
     Create a grain, setting all necessary parameters
     */
    void spawnGrain()
    {
        
        // Loop through each grain the grains vector individually
        for (auto& g : grains)
        {
            
            // check if grain is not active
            if (!g.active)
            {
                // Set sample buffer with desired sample and get its size
                // Having to dereference as the sample buffers are pointers
                auto& sampleBuffer = (*sampleBuffersPtr)[sampleBlend];
                int bufferLength = sampleBuffer.getNumSamples();

                // Calculate grain length in samples from ms as in UI
                int grainLengthSamples = (int)(grainLengthMs * 0.001 * sampleRate);

                // Find the combined position which is specified by which MiDi note was pressed and our adjuster parameter
                float combinedPosition = positionFromMidi + positionInSample;
                
                // Limit position in sample to not be before the start or after the end
                combinedPosition = juce::jlimit(0.0f, 1.0f, combinedPosition);
                
                // Calculate start position in samples
                // (eg. sample is size 20,000, so position 0.5 means start position is at sample 10,000)
                int startSamples = (int) (combinedPosition * bufferLength);

                // Start position random jitter
                startSamples += juce::Random::getSystemRandom().nextInt (juce::Range<int> (-spray, spray));

                // Limit start position in samples to not be before start or after end
                startSamples = juce::jlimit(0, bufferLength - grainLengthSamples, startSamples);

                // Set parameters for grain
                g.start = startSamples;
                g.length = grainLengthSamples;
                g.position = 0;
                g.active = true;  // specify that it is playing
                g.pan = random.nextFloat() * 2.0f - 1.0f;  // random pan for each grain, scaled to [-1, 1]
                
                // Find detune amount from parameter and scale
                float detune = juce::Random::getSystemRandom().nextFloat() * detuneAmount - (detuneAmount / 2.0f);
                
                // Apply detune
                g.playbackRate = pitchRatio * (1.0f + detune);
                
                // Semi-random gain for each grain to make output more interesting even with no parameters
                // Also helps avoid phase match up
                g.gain = 0.7f + 0.3f * juce::Random::getSystemRandom().nextFloat();
            
                return;
            }
        }
    }
        
    // How many grains should have spawned so far
    float spawnCounter = 0.0f;
        
    /**
     Count samples and spawn a grain when necessary
     */
    void updateGrainSpawning()
        {
        
            // How many grains per sample
            // (grains / second) / (samples / second) = grains / sample
            float grainsPerSample = grainDensity / sampleRate;

            // Increment counter
            spawnCounter += grainsPerSample;

            // While enough samples have happened, spawn a grain (could be more than one)
            while (spawnCounter >= 1.0f)
            {
                spawnGrain();
                spawnCounter -= 1.0f;  // subtract 'spent' grain
            }
        }
    
    /**
     Adds distortion to a sample by increasing the gain, and then limiting, causing clipping. Limited to [-0.5, 0.5] using the modified hyperbolic tangent function.
     
     @param inSample sample to add distortion to
     @param gainVal amount to increase gain by. Higher value means more distortion
     @return distorted sample
     */
    float tanhDistortion(float inSample, float gainVal)
    {
        float scaledInput = inSample * gainVal;  // apply the gain (e.g. x20)
        
        // Use use tanh function for soft clipping
        // tanh(2x) / 2 used to get range [-0.5, 0.5] for more prominent clipping
        float distortedSample = tanhf(2.0f * scaledInput) / 2.0f;
        
        // Return the result back to the main program that called the function
        return distortedSample;
    }
};
