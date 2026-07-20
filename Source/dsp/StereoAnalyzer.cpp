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
    const float coeff = target > current ? 0.28f : 0.05f;
    return smoothTowards (current, target, coeff);
}

void StereoAnalyzer::accumulateField (const juce::AudioBuffer<float>& buffer,
                                      float& leftRms,
                                      float& rightRms,
                                      float& midRms,
                                      float& sideRms,
                                      float& peakAbs) noexcept
{
    const int numSamples = buffer.getNumSamples();
    const int numChannels = buffer.getNumChannels();

    if (numSamples <= 0 || numChannels <= 0)
    {
        leftRms = rightRms = midRms = sideRms = peakAbs = 0.0f;
        return;
    }

    const float* left = buffer.getReadPointer (0);
    const float* right = numChannels > 1 ? buffer.getReadPointer (1) : left;

    double sumL = 0.0, sumR = 0.0, sumM = 0.0, sumS = 0.0;
    float peak = 0.0f;

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
        peak = juce::jmax (peak, (float) std::abs (l), (float) std::abs (r));
    }

    const double inv = 1.0 / (double) numSamples;
    leftRms = (float) std::sqrt (sumL * inv);
    rightRms = (float) std::sqrt (sumR * inv);
    midRms = (float) std::sqrt (sumM * inv);
    sideRms = (float) std::sqrt (sumS * inv);
    peakAbs = peak;
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

AnalysisResult StereoAnalyzer::analyse (const juce::AudioBuffer<float>& dryBuffer,
                                        const juce::AudioBuffer<float>& bandBuffer,
                                        float lowHz,
                                        float highHz) noexcept
{
    float dryL = 0, dryR = 0, dryM = 0, dryS = 0, dryPeak = 0;
    float bandL = 0, bandR = 0, bandM = 0, bandS = 0, bandPeak = 0;

    accumulateField (dryBuffer, dryL, dryR, dryM, dryS, dryPeak);
    accumulateField (bandBuffer, bandL, bandR, bandM, bandS, bandPeak);

    const float bandRms = std::sqrt (bandL * bandL + bandR * bandR);
    peakEnv = juce::jmax (bandPeak, peakEnv * 0.92f);
    shortEnv = smoothTowards (shortEnv, bandRms, 0.35f);
    longEnv = smoothTowards (longEnv, bandRms, 0.04f);

    const float crestRaw = peakEnv / juce::jmax (1.0e-5f, bandRms);
    const float crestNorm = juce::jlimit (0.0f, 1.0f, (crestRaw - 1.2f) / 8.0f); // open dynamics -> 1
    const float punchRaw = shortEnv / juce::jmax (1.0e-5f, longEnv);
    const float punchNorm = juce::jlimit (0.0f, 1.0f, (punchRaw - 0.85f) / 1.8f);
    const float densityNorm = 1.0f - crestNorm; // compression / sustain fill

    pushSamplesForFft (bandBuffer);
    const float spectralTarget = computeSpectralFocus (lowHz, highHz);

    AnalysisResult out;
    out.leftEnergy = softClip01 (bandL * 1.4f);
    out.rightEnergy = softClip01 (bandR * 1.4f);
    out.midEnergy = softClip01 (bandM * 1.4f);
    out.sideEnergy = softClip01 (bandS * 1.4f);
    out.energy = softClip01 (std::sqrt (dryL * dryL + dryR * dryR));
    out.bandEnergy = softClip01 (bandRms);
    out.spectralFocus = spectralTarget;
    out.crest = crestNorm;
    out.punch = punchNorm;
    out.density = densityNorm;

    out.leftEnergy = smoothMeter (smoothed.leftEnergy, out.leftEnergy);
    out.rightEnergy = smoothMeter (smoothed.rightEnergy, out.rightEnergy);
    out.midEnergy = smoothMeter (smoothed.midEnergy, out.midEnergy);
    out.sideEnergy = smoothMeter (smoothed.sideEnergy, out.sideEnergy);
    out.energy = smoothMeter (smoothed.energy, out.energy);
    out.bandEnergy = smoothMeter (smoothed.bandEnergy, out.bandEnergy);
    out.spectralFocus = smoothMeter (smoothed.spectralFocus, out.spectralFocus);
    out.crest = smoothMeter (smoothed.crest, out.crest);
    out.punch = smoothMeter (smoothed.punch, out.punch);
    out.density = smoothMeter (smoothed.density, out.density);

    smoothed = out;
    return out;
}

} // namespace sv
