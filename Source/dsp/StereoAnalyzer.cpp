#include "dsp/StereoAnalyzer.h"
#include <cmath>

namespace sv
{

namespace
{
float channelRms (const juce::AudioBuffer<float>& buffer, int channel) noexcept
{
    if (! juce::isPositiveAndBelow (channel, buffer.getNumChannels()))
        return 0.0f;

    const auto* data = buffer.getReadPointer (channel);
    const int n = buffer.getNumSamples();
    double sum = 0.0;

    for (int i = 0; i < n; ++i)
        sum += (double) data[i] * (double) data[i];

    return n > 0 ? (float) std::sqrt (sum / (double) n) : 0.0f;
}

float smoothTowards (float current, float target, float coeff) noexcept
{
    return current + coeff * (target - current);
}
} // namespace

void StereoAnalyzer::prepare (double newSampleRate, int /*maximumBlockSize*/)
{
    sampleRate = newSampleRate > 0.0 ? newSampleRate : 44100.0;
    reset();
}

void StereoAnalyzer::reset()
{
    fifo.fill (0.0f);
    fftData.fill (0.0f);
    fifoIndex = 0;
    nextBlockReady = false;
    smoothedPan = smoothedDepth = smoothedEnergy = smoothedBandEnergy = smoothedSpectral = 0.0f;
}

float StereoAnalyzer::softClip01 (float x) noexcept
{
    return juce::jlimit (0.0f, 1.0f, 1.0f - std::exp (-3.5f * juce::jmax (0.0f, x)));
}

void StereoAnalyzer::pushSamplesForFft (const juce::AudioBuffer<float>& buffer) noexcept
{
    const int numChannels = buffer.getNumChannels();
    const int numSamples = buffer.getNumSamples();

    for (int i = 0; i < numSamples; ++i)
    {
        float mono = 0.0f;

        for (int ch = 0; ch < numChannels; ++ch)
            mono += buffer.getSample (ch, i);

        mono *= numChannels > 0 ? (1.0f / (float) numChannels) : 0.0f;
        fifo[(size_t) fifoIndex] = mono;

        if (++fifoIndex == fftSize)
        {
            fifoIndex = 0;
            nextBlockReady = true;
        }
    }
}

float StereoAnalyzer::computeSpectralFocus (float centreHz, float bandwidthHz) noexcept
{
    if (! nextBlockReady)
        return smoothedSpectral;

    nextBlockReady = false;

    std::copy (fifo.begin(), fifo.end(), fftData.begin());
    window.multiplyWithWindowingTable (fftData.data(), (size_t) fftSize);
    fft.performFrequencyOnlyForwardTransform (fftData.data());

    const float binHz = (float) sampleRate / (float) fftSize;
    const float lowHz = juce::jmax (0.0f, centreHz - 0.5f * bandwidthHz);
    const float highHz = centreHz + 0.5f * bandwidthHz;
    const int lowBin = juce::jlimit (0, fftSize / 2, (int) std::floor (lowHz / binHz));
    const int highBin = juce::jlimit (lowBin, fftSize / 2, (int) std::ceil (highHz / binHz));

    float peak = 0.0f;
    float sum = 0.0f;
    int count = 0;

    for (int bin = lowBin; bin <= highBin; ++bin)
    {
        const float mag = fftData[(size_t) bin];
        peak = juce::jmax (peak, mag);
        sum += mag;
        ++count;
    }

    const float mean = count > 0 ? sum / (float) count : 0.0f;
    return softClip01 (0.65f * peak + 0.35f * mean);
}

AnalysisResult StereoAnalyzer::analyse (const juce::AudioBuffer<float>& dryBuffer,
                                        const juce::AudioBuffer<float>& bandBuffer,
                                        float centreHz,
                                        float bandwidthHz) noexcept
{
    const float left = channelRms (dryBuffer, 0);
    const float right = dryBuffer.getNumChannels() > 1 ? channelRms (dryBuffer, 1) : left;
    const float mid = 0.5f * (left + right);
    const float side = 0.5f * std::abs (left - right);
    const float denom = juce::jmax (1.0e-6f, left + right);

    const float panTarget = (right - left) / denom;
    const float depthTarget = juce::jlimit (-1.0f, 1.0f, (mid - side) / juce::jmax (1.0e-6f, mid + side));
    const float energyTarget = softClip01 (std::sqrt (left * left + right * right));

    const float bandL = channelRms (bandBuffer, 0);
    const float bandR = bandBuffer.getNumChannels() > 1 ? channelRms (bandBuffer, 1) : bandL;
    const float bandEnergyTarget = softClip01 (std::sqrt (bandL * bandL + bandR * bandR));

    pushSamplesForFft (dryBuffer);
    const float spectralTarget = computeSpectralFocus (centreHz, bandwidthHz);

    constexpr float coeff = 0.18f;
    smoothedPan = smoothTowards (smoothedPan, panTarget, coeff);
    smoothedDepth = smoothTowards (smoothedDepth, depthTarget, coeff);
    smoothedEnergy = smoothTowards (smoothedEnergy, energyTarget, coeff);
    smoothedBandEnergy = smoothTowards (smoothedBandEnergy, bandEnergyTarget, coeff);
    smoothedSpectral = smoothTowards (smoothedSpectral, spectralTarget, coeff);

    return { smoothedPan, smoothedDepth, smoothedEnergy, smoothedBandEnergy, smoothedSpectral };
}

} // namespace sv
