#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>
#include <juce_audio_utils/juce_audio_utils.h>
#include <array>
class DrumFilterAudioProcessor final : public juce::AudioProcessor
{
public:
    DrumFilterAudioProcessor();
    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }
    const juce::String getName() const override { return "DrumFilter"; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }
    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram(int) override {}
    const juce::String getProgramName(int) override { return {}; }
    void changeProgramName(int, const juce::String&) override {}
    void getStateInformation(juce::MemoryBlock&) override;
    void setStateInformation(const void*, int) override;
private:
    juce::AudioProcessorValueTreeState state;
    static juce::AudioProcessorValueTreeState::ParameterLayout makeLayout();
    juce::dsp::IIR::Filter<float> detectorHigh, detectorLow;
    std::array<std::array<juce::dsp::IIR::Filter<float>, 4>, 2> filters;
    double fs = 44100.0, envelope = 1.0, detectorEnv = 0.0, previousDetectorEnv = 0.0;
    int holdoff = 0, coefficientCounter = 0;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(DrumFilterAudioProcessor)
};
