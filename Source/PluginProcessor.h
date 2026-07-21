#pragma once

#include "dsp/BandFilter.h"
#include "dsp/StereoAnalyzer.h"
#include "ipc/SoundVisionHub.h"
#include "params/PluginParameters.h"
#include <JuceHeader.h>
#include <atomic>

class SoundVisionAudioProcessor final : public juce::AudioProcessor
{
public:
    SoundVisionAudioProcessor();
    ~SoundVisionAudioProcessor() override;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return JucePlugin_Name; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    juce::AudioProcessorValueTreeState& getAPVTS() noexcept { return apvts; }
    juce::SharedResourcePointer<sv::SoundVisionHub>& getHub() noexcept { return hub; }

    sv::PluginMode getMode() const noexcept;
    juce::String getSourceDisplayName() const;
    void setSourceDisplayName (const juce::String& name);

    std::vector<sv::SourceSnapshot> getReceiverSnapshots() const;
    sv::AnalysisResult getLocalAnalysis() const;
    sv::SourceSnapshot makeLocalSnapshot() const;

    float getFreqLowHz() const;
    float getFreqHighHz() const;
    float getParticleRate() const;
    bool isAudioFilterEnabled() const;
    juce::Colour getSourceColour() const;
    void setFreqRange (float lowHz, float highHz);

private:
    juce::AudioProcessorValueTreeState apvts;
    juce::SharedResourcePointer<sv::SoundVisionHub> hub;

    sv::BandFilter visualFilter;
    sv::BandFilter audioFilter;
    sv::StereoAnalyzer analyzer;
    juce::AudioBuffer<float> analysisBuffer;

    std::atomic<int> hubSlot { -1 };
    uint32_t instanceId = 0;
    juce::String sourceName { "Source" };
    mutable juce::SpinLock nameLock;

    sv::AnalysisResult lastAnalysis {};
    mutable juce::SpinLock analysisLock;
    std::atomic<uint64_t> sampleCounter { 0 };

    void ensureSenderRegistration();
    void unregisterSender();
    void publishSenderSnapshot();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SoundVisionAudioProcessor)
};
