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
    sv::AnalysisResult getLocalAnalysis() const noexcept { return lastAnalysis.load(); }

    float getCentreHz() const;
    float getBandwidthHz() const;
    float getParticleRate() const;
    bool isBandOnly() const;
    juce::Colour getSourceColour() const;

private:
    juce::AudioProcessorValueTreeState apvts;
    juce::SharedResourcePointer<sv::SoundVisionHub> hub;

    sv::BandFilter bandFilter;
    sv::StereoAnalyzer analyzer;
    juce::AudioBuffer<float> analysisBuffer;

    std::atomic<int> hubSlot { -1 };
    uint32_t instanceId = 0;
    juce::String sourceName { "Source" };
    mutable juce::SpinLock nameLock;

    struct AtomicAnalysis
    {
        std::atomic<float> pan { 0.0f };
        std::atomic<float> depth { 0.0f };
        std::atomic<float> energy { 0.0f };
        std::atomic<float> bandEnergy { 0.0f };
        std::atomic<float> spectralFocus { 0.0f };

        void store (const sv::AnalysisResult& r) noexcept
        {
            pan.store (r.pan, std::memory_order_relaxed);
            depth.store (r.depth, std::memory_order_relaxed);
            energy.store (r.energy, std::memory_order_relaxed);
            bandEnergy.store (r.bandEnergy, std::memory_order_relaxed);
            spectralFocus.store (r.spectralFocus, std::memory_order_relaxed);
        }

        sv::AnalysisResult load() const noexcept
        {
            return {
                pan.load (std::memory_order_relaxed),
                depth.load (std::memory_order_relaxed),
                energy.load (std::memory_order_relaxed),
                bandEnergy.load (std::memory_order_relaxed),
                spectralFocus.load (std::memory_order_relaxed)
            };
        }
    };

    AtomicAnalysis lastAnalysis;
    std::atomic<uint64_t> sampleCounter { 0 };

    void ensureSenderRegistration();
    void unregisterSender();
    void publishSenderSnapshot (const sv::AnalysisResult& result);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SoundVisionAudioProcessor)
};
