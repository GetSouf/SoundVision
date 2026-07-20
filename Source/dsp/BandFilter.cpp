#include "dsp/BandFilter.h"

namespace sv
{

void BandFilter::prepare (const juce::dsp::ProcessSpec& spec)
{
    sampleRate = spec.sampleRate;
    filter.prepare (spec);
    dirty = true;
    updateCoefficients();
    filter.reset();
}

void BandFilter::reset()
{
    filter.reset();
}

void BandFilter::setBand (float newCentreHz, float newBandwidthHz)
{
    centreHz = juce::jlimit (20.0f, 20000.0f, newCentreHz);
    bandwidthHz = juce::jlimit (10.0f, 8000.0f, newBandwidthHz);
    dirty = true;
}

void BandFilter::updateCoefficients()
{
    if (! dirty)
        return;

    dirty = false;

    const float q = juce::jmax (0.1f, centreHz / juce::jmax (1.0f, bandwidthHz));
    auto coeffs = juce::dsp::IIR::Coefficients<float>::makeBandPass (sampleRate, centreHz, q);
    *filter.state = *coeffs;
}

void BandFilter::process (juce::AudioBuffer<float>& buffer)
{
    if (! enabled)
        return;

    updateCoefficients();

    juce::dsp::AudioBlock<float> block (buffer);
    juce::dsp::ProcessContextReplacing<float> context (block);
    filter.process (context);
}

} // namespace sv
