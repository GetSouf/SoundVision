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
    lowHz = juce::jlimit (0.0f, 20000.0f, newLowHz);
    highHz = juce::jlimit (juce::jmax (lowHz + 10.0f, 10.0f), 20000.0f, newHighHz);
    dirty = true;
}

void BandFilter::updateCoefficients()
{
    if (! dirty)
        return;

    dirty = false;

    useHighPass = lowHz >= 5.0f;
    useLowPass = highHz < (float) sampleRate * 0.45f;

    if (useHighPass)
    {
        const float hp = juce::jlimit (5.0f, (float) sampleRate * 0.45f, lowHz);
        *highPass.state = *juce::dsp::IIR::Coefficients<float>::makeHighPass (sampleRate, hp, 0.707f);
    }

    if (useLowPass)
    {
        const float lpFloor = useHighPass ? juce::jmax (lowHz, 5.0f) + 10.0f : 10.0f;
        const float lp = juce::jlimit (lpFloor, (float) sampleRate * 0.49f, highHz);
        *lowPass.state = *juce::dsp::IIR::Coefficients<float>::makeLowPass (sampleRate, lp, 0.707f);
    }
}

void BandFilter::process (juce::AudioBuffer<float>& buffer)
{
    if (! enabled)
        return;

    updateCoefficients();

    juce::dsp::AudioBlock<float> block (buffer);
    juce::dsp::ProcessContextReplacing<float> context (block);

    if (useHighPass)
        highPass.process (context);
    if (useLowPass)
        lowPass.process (context);
}

} // namespace sv
