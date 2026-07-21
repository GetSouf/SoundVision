#pragma once

#include "ipc/SourceSnapshot.h"
#include <JuceHeader.h>
#include <array>

namespace sv
{

constexpr int kSpectrumBins = 64;

struct AnalysisResult
{
    std::array<float, kPolarAngleBins * kPolarRadiusBins> polarField {};
    std::array<float, kSpectrumBins> spectrum {};
    float panCentroid = 0.0f;
    float correlation = 1.0f;
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
 * Mid/Side polar histogram — same family as vectorscope / iZotope Sound Field.
 * Pan of the bus is encoded in the Mid/Side angle of every sample.
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
    void fillSpectrum (std::array<float, kSpectrumBins>& out) noexcept;
    void pushSamplesForFft (const juce::AudioBuffer<float>& buffer) noexcept;
    void buildPolarField (const juce::AudioBuffer<float>& buffer,
                          std::array<float, kPolarAngleBins * kPolarRadiusBins>& outField,
                          float& correlationOut,
                          float& panCentroidOut,
                          float& rmsOut,
                          float& peakOut,
                          float& diffusenessOut) noexcept;

    static float softClip01 (float x) noexcept;
    static float smoothMeter (float current, float target) noexcept;
};

} // namespace sv
