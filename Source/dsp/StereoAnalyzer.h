#pragma once

#include "ipc/SourceSnapshot.h"
#include <JuceHeader.h>
#include <array>

namespace sv
{

struct AnalysisResult
{
    std::array<float, kAngularBins> field {};
    float leftEnergy = 0.0f;
    float centreEnergy = 0.0f;
    float rightEnergy = 0.0f;
    float energy = 0.0f;
    float bandEnergy = 0.0f;
    float spectralFocus = 0.0f;
    float diffuseness = 0.0f;
    float crest = 0.5f;
    float punch = 0.0f;
    float density = 0.5f;
};

/**
 * Builds a continuous angular energy map of the stereo image.
 * Pan moves the mass across the stage; low L/R correlation (reverb) widens it.
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
    void buildAngularField (const juce::AudioBuffer<float>& buffer,
                            std::array<float, kAngularBins>& outField,
                            float& diffusenessOut,
                            float& rmsOut,
                            float& peakOut) noexcept;

    static float softClip01 (float x) noexcept;
    static float smoothMeter (float current, float target) noexcept;
};

} // namespace sv
