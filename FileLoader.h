/*
  ==============================================================================

    FileLoader.h
    Created: 24 Mar 2026 9:51:33am
    Author:  Tom Mudd

  ==============================================================================
*/

#pragma once

/**
 Helper class for loading audio file
 
 Call loadIntoAudioBuffer() with either binary data or a file path to get data into an AudioBuffer<float>
 Or use one of the getReaderFor() functions to get the reader directly (e.g. for use with SamplerSound)
 */
class FileLoader
{
public:
    FileLoader()
    {
        // prepare for a range of file formats: wav, aif, mp3, ogg etc
        formatManager.registerBasicFormats();
    }
    
    /**
     load data from a file path into a referenced juce::AudioBuffer<float>
     
     @param juce::String& filePath  -  the path to a valid audio file
     @param juce::AudioBuffer<float>& sampleBuffer - reference to an existing audio buffer
     */
    void loadIntoAudioBuffer (const juce::String& filePath, juce::AudioBuffer<float>& sampleBuffer)
    {
        // create a File object from the audio file path
        juce::File file {filePath};
        
        // use the local formatReader to create a reader for that file format
        auto* formatReader = formatManager.createReaderFor (file);
        
        // use the reader to copy the audio data into the referenced sample buffer
        readFromReaderToAudioBuffer (formatReader, sampleBuffer);
        
        // clean up data
        delete formatReader;
    }
    
    /** 
     load data from a binary resource into a referenced juce::AudioBuffer<float>
     
     @param
     */
    void loadIntoAudioBuffer (const char* binaryData,
                                        const int binaryDataSize,
                                        juce::AudioBuffer<float>& sampleBuffer)
    {
        // create a unique pointer to a MemoryInputStream input stream from the binary data
        std::unique_ptr<juce::MemoryInputStream> inputStream;
        
        // set the unique pointer to point at a Memory Input Stream with our audio data
        inputStream.reset (new juce::MemoryInputStream (binaryData, binaryDataSize, false));
        
        // use the reader to copy the audio data into the referenced sample buffer
        auto* formatReader = formatManager.createReaderFor (std::move(inputStream));
        
        // use the reader to copy the audio data into the referenced sample buffer
        readFromReaderToAudioBuffer (formatReader, sampleBuffer);
        
        // clean up pointer
        delete formatReader;
    }
    
    /**
     Return a unique pointer to a juce::AudioFormatReader object directly, e.g. for use in juce::SamplerSound
     
     Example usage:
     @code
     
     // use this function to get an AudioFormatReader pointer
     auto reader = getReaderFor ("/path/to/file.wav");
        
     // use this to create a SamplerSound
     sampler.addSound (new juce::SamplerSound("SampleName", *reader, midiNoteRange, 60, 0.001, 0.25, 8));
     */
    std::unique_ptr<juce::AudioFormatReader> getReaderFor (const juce::String& filePath)
    {
        // create a File object from the audio file path
        juce::File file {filePath};

        // create a unique pointer to a new reader
        std::unique_ptr<juce::AudioFormatReader> readerPtr;
        readerPtr.reset (formatManager.createReaderFor (file));
       
        // return the unique_ptr
        return readerPtr;
    }
    
    
    /**
     Return a smart pointer to a juce::AudioFormatReader object directly, e.g. for use in juce::SamplerSound
     
     Example usage:
     @code
     // use this function to get an AudioFormatReader pointer
     auto reader = getReaderPointerFromBinaryData (BinaryData::MySample, BinaryData::MySampleLength);
        
     // use this to create a SamplerSound
     sampler.addSound (new juce::SamplerSound("SampleName", *reader, midiNoteRange, 60, 0.001, 0.25, 8));
     */
    std::unique_ptr<juce::AudioFormatReader> getReaderFor (const char* binaryData,
                                                        const int binaryDataSize)
    {
        // create a unique pointer to a MemoryInputStream input stream from the binary data
        std::unique_ptr<juce::MemoryInputStream> inputStream;
        
        // set the unique pointer to point at a Memory Input Stream with our audio data
        inputStream.reset (new juce::MemoryInputStream (binaryData, binaryDataSize, false));
        
        // return a pointer to a new reader
        std::unique_ptr<juce::AudioFormatReader> readerPtr;
        readerPtr.reset (formatManager.createReaderFor (std::move(inputStream)));
       
        return readerPtr;

    }
    
private:
    
    // checks which kind of file format we have and gives us the appropriate reader object
    juce::AudioFormatManager formatManager;
    
    /**
     set audio buffer to same number of channels and samples as the input audio file.
     
     Read the data into the buffer
     */
    void readFromReaderToAudioBuffer(juce::AudioFormatReader* reader, juce::AudioBuffer<float>& sampleBuffer)
    {
        // resize the referenced buffer
        sampleBuffer.setSize ((int)reader->numChannels, (int)reader->lengthInSamples);
        
        // read data into the buffer
        reader->read (&sampleBuffer, 0, (int)reader->lengthInSamples, 0, true, false);

    }
};
