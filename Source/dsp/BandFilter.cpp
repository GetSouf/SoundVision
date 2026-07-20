#include "dsp/BandFilter.h"

namespace sv
{

void BandFilter::prepare (const juce::dsp::ProcessSpec& spec)
{
    sampleRate = spec.sampleRate;
    highPass.prepare (spec);
    lowPass.prepare (spec);
    dirty = true;
    updateCoefficients();
    reset();
}

void BandFilter::reset()
{
    highPass.reset();
    lowPass.reset();
}

void BandFilter::setRange (float newLowHz, float newHighHz)
{
    lowHz = juce::jlimit (1.0f, 20000.0f, newLowHz);
    highHz = juce::jlimit (lowHz + 10.0f, 20000.0f, newHighHz);
    dirty = true;
}

void BandFilter::updateCoefficients()
{
    if (! dirty)
        return;

    dirty = false;

    const float hp = juce::jlimit (1.0f, (float) sampleRate * 0.45f, lowHz);
    const float lp = juce::jlimit (hp + 10.0f, (float) sampleRate * 0.49f, highHz);

    *highPass.state = *juce::dsp::IIR::Coefficients<float>::makeHighPass (sampleRate, hp, 0.707f);
    *lowPass.state = *juce::dsp::IIR::Coefficients<float>::makeLowPass (sampleRate, lp, 0.707f);
}

void BandFilter::process (juce::AudioBuffer<float>& buffer)
{
    if (! enabled)
        return;

    updateCoefficients();

    juce::dsp::AudioBlock<float> block (buffer);
    juce::dsp::ProcessContextReplacing<float> context (block);
    highPass.process (context);
    lowPass.process (context);
}

} // namespace sv
