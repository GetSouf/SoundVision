#pragma once

#include <JuceHeader.h>
#include <array>

namespace sv
{

struct AnalysisResult
{
    float pan = 0.0f;
    float depth = 0.0f;
    float energy = 0.0f;
    float bandEnergy = 0.0f;
    float spectralFocus = 0.0f;
};

/**
 * Extracts stereo position + energy metrics from a buffer.
 * Optionally runs a small FFT for spectral focus around a centre frequency.
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
    static constexpr int fftOrder = 11; // 2048
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

    float smoothedPan = 0.0f;
    float smoothedDepth = 0.0f;
    float smoothedEnergy = 0.0f;
    float smoothedBandEnergy = 0.0f;
    float smoothedSpectral = 0.0f;

    float computeSpectralFocus (float centreHz, float bandwidthHz) noexcept;
    void pushSamplesForFft (const juce::AudioBuffer<float>& buffer) noexcept;

    static float softClip01 (float x) noexcept;
};

} // namespace sv
