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
    sv::SourceSnapshot makeLocalSnapshot() const;

    float getFreqLowHz() const;
    float getFreqHighHz() const;
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
        std::atomic<float> leftEnergy { 0.0f };
        std::atomic<float> rightEnergy { 0.0f };
        std::atomic<float> midEnergy { 0.0f };
        std::atomic<float> sideEnergy { 0.0f };
        std::atomic<float> energy { 0.0f };
        std::atomic<float> bandEnergy { 0.0f };
        std::atomic<float> spectralFocus { 0.0f };
        std::atomic<float> crest { 0.5f };
        std::atomic<float> punch { 0.0f };
        std::atomic<float> density { 0.5f };

        void store (const sv::AnalysisResult& r) noexcept
        {
            leftEnergy.store (r.leftEnergy, std::memory_order_relaxed);
            rightEnergy.store (r.rightEnergy, std::memory_order_relaxed);
            midEnergy.store (r.midEnergy, std::memory_order_relaxed);
            sideEnergy.store (r.sideEnergy, std::memory_order_relaxed);
            energy.store (r.energy, std::memory_order_relaxed);
            bandEnergy.store (r.bandEnergy, std::memory_order_relaxed);
            spectralFocus.store (r.spectralFocus, std::memory_order_relaxed);
            crest.store (r.crest, std::memory_order_relaxed);
            punch.store (r.punch, std::memory_order_relaxed);
            density.store (r.density, std::memory_order_relaxed);
        }

        sv::AnalysisResult load() const noexcept
        {
            return {
                leftEnergy.load (std::memory_order_relaxed),
                rightEnergy.load (std::memory_order_relaxed),
                midEnergy.load (std::memory_order_relaxed),
                sideEnergy.load (std::memory_order_relaxed),
                energy.load (std::memory_order_relaxed),
                bandEnergy.load (std::memory_order_relaxed),
                spectralFocus.load (std::memory_order_relaxed),
                crest.load (std::memory_order_relaxed),
                punch.load (std::memory_order_relaxed),
                density.load (std::memory_order_relaxed)
            };
        }
    };

    AtomicAnalysis lastAnalysis;
    std::atomic<uint64_t> sampleCounter { 0 };

    void ensureSenderRegistration();
    void unregisterSender();
    void publishSenderSnapshot();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SoundVisionAudioProcessor)
};
