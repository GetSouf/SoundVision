#pragma once

#include <JuceHeader.h>
#include <array>

namespace sv
{

struct AnalysisResult
{
    float leftEnergy = 0.0f;
    float rightEnergy = 0.0f;
    float midEnergy = 0.0f;
    float sideEnergy = 0.0f;
    float energy = 0.0f;
    float bandEnergy = 0.0f;
    float spectralFocus = 0.0f;
    float crest = 0.5f;
    float punch = 0.0f;
    float density = 0.5f;
};

/**
 * Stereo-field + dynamics metrics.
 * Crest/punch/density make compression and transient shaping readable as cloud texture.
 */
class StereoAnalyzer
{
public:
    void prepare (double sampleRate, int maximumBlockSize);
    void reset();

    AnalysisResult analyse (const juce::AudioBuffer<float>& dryBuffer,
                            const juce::AudioBuffer<float>& bandBuffer,
                            float lowHz,
                            float highHz) noexcept;

private:
    static constexpr int fftOrder = 11;
    static constexpr int fftSize = 1 << fftOrder;

    juce::dsp::FFT fft { fftOrder };
    juce::dsp::WindowingFunction<float> window {
        (size_t) fftSize, juce::dsp::WindowingFunction<float>::hann, true
    };

    std::array<float, fftSize * 2> fftData {};
    std::array<float, fftSize> fifo {};
    int fifoIndex = 0;
    bool nextBlockReady = false;
    double sampleRate = 44100.0;

    float peakEnv = 0.0f;
    float shortEnv = 0.0f;
    float longEnv = 0.0f;
    AnalysisResult smoothed {};

    float computeSpectralFocus (float lowHz, float highHz) noexcept;
    void pushSamplesForFft (const juce::AudioBuffer<float>& buffer) noexcept;

    static float softClip01 (float x) noexcept;
    static float smoothMeter (float current, float target) noexcept;
    static void accumulateField (const juce::AudioBuffer<float>& buffer,
                                 float& leftRms,
                                 float& rightRms,
                                 float& midRms,
                                 float& sideRms,
                                 float& peakAbs) noexcept;
};

} // namespace sv
