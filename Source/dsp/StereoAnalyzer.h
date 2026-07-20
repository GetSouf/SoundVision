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
};

/**
 * Stable stereo-field metrics: per-channel RMS plus true Mid/Side RMS,
 * with fast-attack / slow-release smoothing to kill frame jitter.
 */
class StereoAnalyzer
{
public:
    void prepare (double sampleRate, int maximumBlockSize);
    void reset();

    AnalysisResult analyse (const juce::AudioBuffer<float>& dryBuffer,
                            const juce::AudioBuffer<float>& bandBuffer,
                            float centreHz,
                            float bandwidthHz) noexcept;

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

    AnalysisResult smoothed {};

    float computeSpectralFocus (float centreHz, float bandwidthHz) noexcept;
    void pushSamplesForFft (const juce::AudioBuffer<float>& buffer) noexcept;

    static float softClip01 (float x) noexcept;
    static float smoothMeter (float current, float target) noexcept;
    static void accumulateField (const juce::AudioBuffer<float>& buffer,
                                 float& leftRms,
                                 float& rightRms,
                                 float& midRms,
                                 float& sideRms) noexcept;
};

} // namespace sv
