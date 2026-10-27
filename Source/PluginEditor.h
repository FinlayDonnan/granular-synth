/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"

//==============================================================================
/**
*/
class AP_Ass3AudioProcessorEditor  : public juce::AudioProcessorEditor
{
public:
    AP_Ass3AudioProcessorEditor (AP_Ass3AudioProcessor&);
    ~AP_Ass3AudioProcessorEditor() override;

    //==============================================================================
    void paint (juce::Graphics&) override;
    void resized() override;

private:
    // This reference is provided as a quick way for your editor to
    // access the processor object that created it.
    AP_Ass3AudioProcessor& audioProcessor;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AP_Ass3AudioProcessorEditor)
};
