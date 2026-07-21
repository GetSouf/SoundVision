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
    peakEnv = shortEnv = longEnv = 0.0f;
    smoothed = {};
}

float StereoAnalyzer::softClip01 (float x) noexcept
{
    return juce::jlimit (0.0f, 1.0f, 1.0f - std::exp (-3.2f * juce::jmax (0.0f, x)));
}

float StereoAnalyzer::smoothMeter (float current, float target) noexcept
{
    const float coeff = target > current ? 0.32f : 0.07f;
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

void StereoAnalyzer::fillSpectrum (std::array<float, kSpectrumBins>& out) noexcept
{
    out.fill (0.0f);

    const float nyquist = (float) sampleRate * 0.5f;
    const float binHz = (float) sampleRate / (float) fftSize;
    const int maxFftBin = fftSize / 2;

    for (int i = 0; i < kSpectrumBins; ++i)
    {
        const float t0 = (float) i / (float) kSpectrumBins;
        const float t1 = (float) (i + 1) / (float) kSpectrumBins;
        // Log frequency mapping 20 Hz .. nyquist (matches knob feel).
        const float f0 = 20.0f * std::pow (nyquist / 20.0f, t0);
        const float f1 = 20.0f * std::pow (nyquist / 20.0f, t1);

        const int b0 = juce::jlimit (1, maxFftBin, (int) std::floor (f0 / binHz));
        const int b1 = juce::jlimit (b0, maxFftBin, (int) std::ceil (f1 / binHz));

        float peak = 0.0f;
        for (int b = b0; b <= b1; ++b)
            peak = juce::jmax (peak, fftData[(size_t) b]);

        out[(size_t) i] = softClip01 (peak * 0.08f);
    }
}

void StereoAnalyzer::buildPolarField (const juce::AudioBuffer<float>& buffer,
                                      std::array<float, kPolarAngleBins * kPolarRadiusBins>& outField,
                                      float& correlationOut,
                                      float& panCentroidOut,
                                      float& rmsOut,
                                      float& peakOut,
                                      float& diffusenessOut) noexcept
{
    outField.fill (0.0f);
    correlationOut = 1.0f;
    panCentroidOut = 0.0f;
    rmsOut = 0.0f;
    peakOut = 0.0f;
    diffusenessOut = 0.0f;

    const int numSamples = buffer.getNumSamples();
    const int numChannels = buffer.getNumChannels();
    if (numSamples <= 0 || numChannels <= 0)
        return;

    const float* left = buffer.getReadPointer (0);
    const float* right = numChannels > 1 ? buffer.getReadPointer (1) : left;

    double sumL2 = 0.0, sumR2 = 0.0, sumLR = 0.0, sumE = 0.0;
    float peak = 0.0f;
    double panWeighted = 0.0, panWeight = 0.0;

    for (int i = 0; i < numSamples; ++i)
    {
        const float l = left[i];
        const float r = right[i];
        const float mid = 0.5f * (l + r);
        const float side = 0.5f * (l - r);
        const float msEnergy = mid * mid + side * side;

        sumL2 += (double) l * l;
        sumR2 += (double) r * r;
        sumLR += (double) l * r;
        sumE += (double) msEnergy;
        peak = juce::jmax (peak, std::abs (l), std::abs (r));

        if (msEnergy < 1.0e-14f)
            continue;

        // Mid/Side angle: 0 = centre (mono), -pi/2 = left, +pi/2 = right.
        // This tracks mixer pan on the bus accurately.
        const float angle = std::atan2 (side, mid + 1.0e-12f);
        const float mag = std::sqrt (msEnergy);

        const float angleNorm = (angle + juce::MathConstants<float>::halfPi)
                              / juce::MathConstants<float>::pi;
        const int angleBin = juce::jlimit (0, kPolarAngleBins - 1,
                                           (int) std::floor (angleNorm * (float) (kPolarAngleBins - 1)));
        const int radiusBin = juce::jlimit (0, kPolarRadiusBins - 1,
                                            (int) std::floor (softClip01 (mag * 2.2f) * (float) (kPolarRadiusBins - 1)));

        outField[(size_t) polarIndex (angleBin, radiusBin)] += msEnergy;

        const float pan = (r - l) / juce::jmax (1.0e-6f, std::abs (l) + std::abs (r));
        panWeighted += (double) pan * (double) msEnergy;
        panWeight += (double) msEnergy;
    }

    const double denom = std::sqrt (juce::jmax (1.0e-20, sumL2 * sumR2));
    correlationOut = denom > 0.0 ? (float) juce::jlimit (-1.0, 1.0, sumLR / denom) : 1.0f;
    diffusenessOut = juce::jlimit (0.0f, 1.0f, 1.0f - std::abs (correlationOut));
    rmsOut = numSamples > 0 ? (float) std::sqrt (sumE / (double) numSamples) : 0.0f;
    peakOut = peak;
    panCentroidOut = panWeight > 0.0 ? (float) (panWeighted / panWeight) : 0.0f;

    // Normalise polar field.
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
    std::array<float, kPolarAngleBins * kPolarRadiusBins> field {};
    float correlation = 1.0f, panCentroid = 0.0f, bandRms = 0.0f, bandPeak = 0.0f, diffuseness = 0.0f;
    buildPolarField (bandBuffer, field, correlation, panCentroid, bandRms, bandPeak, diffuseness);

    float dryCorr = 1, dryPan = 0, dryRms = 0, dryPeak = 0, dryDiff = 0;
    std::array<float, kPolarAngleBins * kPolarRadiusBins> dryField {};
    buildPolarField (dryBuffer, dryField, dryCorr, dryPan, dryRms, dryPeak, dryDiff);
    juce::ignoreUnused (dryField, dryCorr, dryPan, dryPeak, dryDiff);

    peakEnv = juce::jmax (bandPeak, peakEnv * 0.92f);
    shortEnv = smoothTowards (shortEnv, bandRms, 0.35f);
    longEnv = smoothTowards (longEnv, bandRms, 0.04f);

    const float crestRaw = peakEnv / juce::jmax (1.0e-5f, bandRms);
    const float crestNorm = juce::jlimit (0.0f, 1.0f, (crestRaw - 1.2f) / 8.0f);
    const float punchRaw = shortEnv / juce::jmax (1.0e-5f, longEnv);
    const float punchNorm = juce::jlimit (0.0f, 1.0f, (punchRaw - 0.85f) / 1.8f);

    pushSamplesForFft (dryBuffer);
    const float spectralTarget = computeSpectralFocus (lowHz, highHz);

    AnalysisResult out;
    out.polarField = field;
    fillSpectrum (out.spectrum);
    out.panCentroid = panCentroid;
    out.correlation = correlation;
    out.diffuseness = diffuseness;
    out.bandEnergy = softClip01 (bandRms);
    out.energy = softClip01 (dryRms);
    out.spectralFocus = spectralTarget;
    out.crest = crestNorm;
    out.punch = punchNorm;
    out.density = 1.0f - crestNorm;

    // L/C/R legend from angle thirds of polar field.
    const int third = kPolarAngleBins / 3;
    float lSum = 0, cSum = 0, rSum = 0;
    for (int r = 0; r < kPolarRadiusBins; ++r)
    {
        for (int a = 0; a < kPolarAngleBins; ++a)
        {
            const float v = field[(size_t) polarIndex (a, r)];
            if (a < third)
                lSum += v;
            else if (a < 2 * third)
                cSum += v;
            else
                rSum += v;
        }
    }
    const float norm = juce::jmax (1.0e-6f, (float) (third * kPolarRadiusBins));
    out.leftEnergy = softClip01 (lSum / norm);
    out.centreEnergy = softClip01 (cSum / norm);
    out.rightEnergy = softClip01 (rSum / norm);

    for (size_t i = 0; i < out.polarField.size(); ++i)
        out.polarField[i] = smoothMeter (smoothed.polarField[i], out.polarField[i]);

    for (size_t i = 0; i < out.spectrum.size(); ++i)
        out.spectrum[i] = smoothMeter (smoothed.spectrum[i], out.spectrum[i]);

    out.panCentroid = smoothMeter (smoothed.panCentroid, out.panCentroid);
    out.correlation = smoothMeter (smoothed.correlation, out.correlation);
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
