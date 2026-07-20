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

int panToBin (float pan) noexcept
{
    const float x = juce::jlimit (0.0f, 1.0f, 0.5f * (pan + 1.0f));
    return juce::jlimit (0, kAngularBins - 1, (int) std::floor (x * (float) (kAngularBins - 1) + 1.0e-4f));
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
    peakEnv = shortEnv = longEnv = 0.0f;
    smoothed = {};
}

float StereoAnalyzer::softClip01 (float x) noexcept
{
    return juce::jlimit (0.0f, 1.0f, 1.0f - std::exp (-3.2f * juce::jmax (0.0f, x)));
}

float StereoAnalyzer::smoothMeter (float current, float target) noexcept
{
    const float coeff = target > current ? 0.30f : 0.06f;
    return smoothTowards (current, target, coeff);
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

float StereoAnalyzer::computeSpectralFocus (float lowHz, float highHz) noexcept
{
    if (! nextBlockReady)
        return smoothed.spectralFocus;

    nextBlockReady = false;
    std::copy (fifo.begin(), fifo.end(), fftData.begin());
    window.multiplyWithWindowingTable (fftData.data(), (size_t) fftSize);
    fft.performFrequencyOnlyForwardTransform (fftData.data());

    const float binHz = (float) sampleRate / (float) fftSize;
    const int lowBin = juce::jlimit (0, fftSize / 2, (int) std::floor (lowHz / binHz));
    const int highBin = juce::jlimit (lowBin, fftSize / 2, (int) std::ceil (highHz / binHz));

    float peak = 0.0f, sum = 0.0f;
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

void StereoAnalyzer::buildAngularField (const juce::AudioBuffer<float>& buffer,
                                        std::array<float, kAngularBins>& outField,
                                        float& diffusenessOut,
                                        float& rmsOut,
                                        float& peakOut) noexcept
{
    outField.fill (0.0f);
    diffusenessOut = 0.0f;
    rmsOut = 0.0f;
    peakOut = 0.0f;

    const int numSamples = buffer.getNumSamples();
    const int numChannels = buffer.getNumChannels();
    if (numSamples <= 0 || numChannels <= 0)
        return;

    const float* left = buffer.getReadPointer (0);
    const float* right = numChannels > 1 ? buffer.getReadPointer (1) : left;

    double sumL2 = 0.0, sumR2 = 0.0, sumLR = 0.0, sumE = 0.0;
    float peak = 0.0f;
    std::array<float, kAngularBins> sharp {};
    sharp.fill (0.0f);

    for (int i = 0; i < numSamples; ++i)
    {
        const float l = left[i];
        const float r = right[i];
        const float l2 = l * l;
        const float r2 = r * r;
        const float e = l2 + r2;

        sumL2 += (double) l2;
        sumR2 += (double) r2;
        sumLR += (double) l * (double) r;
        sumE += (double) e;
        peak = juce::jmax (peak, std::abs (l), std::abs (r));

        if (e < 1.0e-12f)
            continue;

        // Energy balance: -1 hard left, +1 hard right. Moves with mixer pan.
        const float pan = (r2 - l2) / e;
        sharp[(size_t) panToBin (pan)] += e;
    }

    const double denom = std::sqrt (juce::jmax (1.0e-20, sumL2 * sumR2));
    const float corr = denom > 0.0 ? (float) juce::jlimit (-1.0, 1.0, sumLR / denom) : 1.0f;
    diffusenessOut = juce::jlimit (0.0f, 1.0f, 1.0f - std::abs (corr));
    rmsOut = numSamples > 0 ? (float) std::sqrt (sumE / (double) numSamples) : 0.0f;
    peakOut = peak;

    // Blur: dry/correlated = tight image; reverb/decorrelated = fills the stage.
    const float sigma = 0.35f + diffusenessOut * 7.5f; // bins
    const float twoSigma2 = 2.0f * sigma * sigma;
    const int radius = juce::jlimit (1, kAngularBins / 2, (int) std::ceil (sigma * 3.0f));

    for (int i = 0; i < kAngularBins; ++i)
    {
        float acc = 0.0f;
        float wsum = 0.0f;

        for (int d = -radius; d <= radius; ++d)
        {
            const int j = i + d;
            if (j < 0 || j >= kAngularBins)
                continue;

            const float w = std::exp (-(float) (d * d) / twoSigma2);
            acc += sharp[(size_t) j] * w;
            wsum += w;
        }

        outField[(size_t) i] = wsum > 0.0f ? acc / wsum : 0.0f;
    }

    float maxBin = 0.0f;
    for (float v : outField)
        maxBin = juce::jmax (maxBin, v);

    if (maxBin > 1.0e-12f)
        for (float& v : outField)
            v = softClip01 (v / maxBin);
}

AnalysisResult StereoAnalyzer::analyse (const juce::AudioBuffer<float>& dryBuffer,
                                        const juce::AudioBuffer<float>& bandBuffer,
                                        float lowHz,
                                        float highHz) noexcept
{
    std::array<float, kAngularBins> field {};
    float diffuseness = 0.0f, bandRms = 0.0f, bandPeak = 0.0f;
    buildAngularField (bandBuffer, field, diffuseness, bandRms, bandPeak);

    float dryDiff = 0, dryRms = 0, dryPeak = 0;
    std::array<float, kAngularBins> dryField {};
    buildAngularField (dryBuffer, dryField, dryDiff, dryRms, dryPeak);
    juce::ignoreUnused (dryField, dryDiff, dryPeak);

    peakEnv = juce::jmax (bandPeak, peakEnv * 0.92f);
    shortEnv = smoothTowards (shortEnv, bandRms, 0.35f);
    longEnv = smoothTowards (longEnv, bandRms, 0.04f);

    const float crestRaw = peakEnv / juce::jmax (1.0e-5f, bandRms);
    const float crestNorm = juce::jlimit (0.0f, 1.0f, (crestRaw - 1.2f) / 8.0f);
    const float punchRaw = shortEnv / juce::jmax (1.0e-5f, longEnv);
    const float punchNorm = juce::jlimit (0.0f, 1.0f, (punchRaw - 0.85f) / 1.8f);

    pushSamplesForFft (bandBuffer);
    const float spectralTarget = computeSpectralFocus (lowHz, highHz);

    AnalysisResult out;
    out.field = field;
    out.diffuseness = diffuseness;
    out.bandEnergy = softClip01 (bandRms);
    out.energy = softClip01 (dryRms);
    out.spectralFocus = spectralTarget;
    out.crest = crestNorm;
    out.punch = punchNorm;
    out.density = 1.0f - crestNorm;

    // Legend thirds from the continuous field.
    const int third = kAngularBins / 3;
    float lSum = 0, cSum = 0, rSum = 0;
    for (int i = 0; i < kAngularBins; ++i)
    {
        const float v = field[(size_t) i];
        if (i < third)
            lSum += v;
        else if (i < 2 * third)
            cSum += v;
        else
            rSum += v;
    }
    const float thirdNorm = juce::jmax (1.0e-6f, (float) third);
    out.leftEnergy = softClip01 (lSum / thirdNorm);
    out.centreEnergy = softClip01 (cSum / thirdNorm);
    out.rightEnergy = softClip01 (rSum / thirdNorm);

    // Smooth scalars + field bins.
    for (int i = 0; i < kAngularBins; ++i)
        out.field[(size_t) i] = smoothMeter (smoothed.field[(size_t) i], out.field[(size_t) i]);

    out.leftEnergy = smoothMeter (smoothed.leftEnergy, out.leftEnergy);
    out.centreEnergy = smoothMeter (smoothed.centreEnergy, out.centreEnergy);
    out.rightEnergy = smoothMeter (smoothed.rightEnergy, out.rightEnergy);
    out.energy = smoothMeter (smoothed.energy, out.energy);
    out.bandEnergy = smoothMeter (smoothed.bandEnergy, out.bandEnergy);
    out.spectralFocus = smoothMeter (smoothed.spectralFocus, out.spectralFocus);
    out.diffuseness = smoothMeter (smoothed.diffuseness, out.diffuseness);
    out.crest = smoothMeter (smoothed.crest, out.crest);
    out.punch = smoothMeter (smoothed.punch, out.punch);
    out.density = smoothMeter (smoothed.density, out.density);

    smoothed = out;
    return out;
}

} // namespace sv
