/*
  ==============================================================================

    Delay.h
    Created: 10 Mar 2026 1:02:47pm
    Author:  Finlay Donnan UoE

  ==============================================================================
*/

#pragma once
#include <vector>

/**
 Add delay to a sample. Must set max delay length, delay length. Can set feedback
 */
class Delay
{
public:
    
    /**
     Set maximum delay length and resize buffer accordingly
     
     @param _maxDelaySize maximum delay length
     */
    void setMaximumDelaySize(int _maxDelaySize)
    {
        maxBufferSize = _maxDelaySize;         // set max delay length
        circularBuffer.resize(maxBufferSize);  // resize buffer
    }
    
    /**
     Set delay time, set and ensure read position is positive
     
     @param _delayTimeInSamples delay time in samples
     */
    void setDelayTimeInSamples(int _delayTimeInSamples)
    {
        delayTimeInSamples = _delayTimeInSamples;  // set delay time
        readPos = writePos - delayTimeInSamples;   // set initial read position
        
        // If read position is negative, move to same position but with positive value
        if (readPos < 0)
            readPos += maxBufferSize;
    }
    
    /**
     Set delay feedback
     
     @param _feedback delay feedback
     */
    void setFeedback(float _feedback)
    {
        feedback = _feedback;  // set delay feedback
        
        // Ensure sensible levels of feedback
        if (feedback > 1.0)   // feedback levels over 1 would result in exponential growth in volume
            feedback = 0.99;
        
        if (feedback < 0.0)   // ensures positive feedback
            feedback = 0.0;
    }
    
    /**
     Store the incoming sample, return a delayed sample
     
     @param inputSample sample to add delay to
     @return sample with delay
     */
    float process(float inputSample)
    {
        
        //---Linear interpolation---
        // Allow for smooth changes in delay time without unwanted artefacts
        int indexA = floor(readPos);
        int indexB = indexA + 1;
        indexB %= maxBufferSize;
        
        float frac = readPos - indexA;
        
        float valA = circularBuffer[indexA];
        float valB = circularBuffer[indexB];
        
        float output = (valB - valA) * frac + valA;  // linear interpolated output
        //---------------------------
        
        // Combine delayed signal with original sample and write into buffer
        float combinedSignal = inputSample + (output * feedback);
        circularBuffer[writePos] = combinedSignal;
        
        // Update read and write positions
        readPos ++;
        writePos ++;
        
        // Wrap read and write positions
        readPos = readPos = fmod(readPos, maxBufferSize);
        writePos = writePos % maxBufferSize;
        
        // return delayed sample
        return output;
    }
    
private:
    
    std::vector <float> circularBuffer;  // initialise buffer to store sample
    int maxBufferSize;                   // initialise max delay/buffer size
    float readPos;                       // initialise read position
    int writePos = 0;                    // initialise write position, setting to 0
    int delayTimeInSamples;              // initialise delay length
    
    float feedback = 0.0;                // initialise feedback and set to 0 as default
};
