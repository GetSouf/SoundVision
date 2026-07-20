#include "dsp/StereoAnalyzer.h"
#include <cmath>

namespace sv
{

namespace
{
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
    smoothed = {};
}

float StereoAnalyzer::softClip01 (float x) noexcept
{
    return juce::jlimit (0.0f, 1.0f, 1.0f - std::exp (-3.2f * juce::jmax (0.0f, x)));
}

float StereoAnalyzer::smoothMeter (float current, float target) noexcept
{
    // Fast attack, slow release — stops the scene from shimmering.
    const float coeff = target > current ? 0.22f : 0.045f;
    return smoothTowards (current, target, coeff);
}

void StereoAnalyzer::accumulateField (const juce::AudioBuffer<float>& buffer,
                                      float& leftRms,
                                      float& rightRms,
                                      float& midRms,
                                      float& sideRms) noexcept
{
    const int numSamples = buffer.getNumSamples();
    const int numChannels = buffer.getNumChannels();

    if (numSamples <= 0 || numChannels <= 0)
    {
        leftRms = rightRms = midRms = sideRms = 0.0f;
        return;
    }

    const float* left = buffer.getReadPointer (0);
    const float* right = numChannels > 1 ? buffer.getReadPointer (1) : left;

    double sumL = 0.0, sumR = 0.0, sumM = 0.0, sumS = 0.0;

    for (int i = 0; i < numSamples; ++i)
    {
        const double l = (double) left[i];
        const double r = (double) right[i];
        const double m = 0.5 * (l + r);
        const double s = 0.5 * (l - r);

        sumL += l * l;
        sumR += r * r;
        sumM += m * m;
        sumS += s * s;
    }

    const double inv = 1.0 / (double) numSamples;
    leftRms = (float) std::sqrt (sumL * inv);
    rightRms = (float) std::sqrt (sumR * inv);
    midRms = (float) std::sqrt (sumM * inv);
    sideRms = (float) std::sqrt (sumS * inv);
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
        return smoothed.spectralFocus;

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
    float dryL = 0, dryR = 0, dryM = 0, dryS = 0;
    float bandL = 0, bandR = 0, bandM = 0, bandS = 0;

    accumulateField (dryBuffer, dryL, dryR, dryM, dryS);
    accumulateField (bandBuffer, bandL, bandR, bandM, bandS);

    const float energyTarget = softClip01 (std::sqrt (dryL * dryL + dryR * dryR));
    const float bandEnergyTarget = softClip01 (std::sqrt (bandL * bandL + bandR * bandR));

    pushSamplesForFft (bandBuffer);
    const float spectralTarget = computeSpectralFocus (centreHz, bandwidthHz);

    AnalysisResult out;
    out.leftEnergy = smoothMeter (smoothed.leftEnergy, softClip01 (bandL * 1.35f));
    out.rightEnergy = smoothMeter (smoothed.rightEnergy, softClip01 (bandR * 1.35f));
    out.midEnergy = smoothMeter (smoothed.midEnergy, softClip01 (bandM * 1.35f));
    out.sideEnergy = smoothMeter (smoothed.sideEnergy, softClip01 (bandS * 1.35f));
    out.energy = smoothMeter (smoothed.energy, energyTarget);
    out.bandEnergy = smoothMeter (smoothed.bandEnergy, bandEnergyTarget);
    out.spectralFocus = smoothMeter (smoothed.spectralFocus, spectralTarget);

    smoothed = out;
    return out;
}

} // namespace sv
