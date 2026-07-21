#pragma once

#include <JuceHeader.h>

namespace sv
{

/** High-pass + low-pass pair. lowHz ~ 0 bypasses HP; high near Nyquist bypasses LP. */
class BandFilter
{
public:
    void prepare (const juce::dsp::ProcessSpec& spec);
    void reset();

    void setRange (float lowHz, float highHz);
    void setEnabled (bool shouldBeEnabled) noexcept { enabled = shouldBeEnabled; }

    void process (juce::AudioBuffer<float>& buffer);

private:
    juce::dsp::ProcessorDuplicator<juce::dsp::IIR::Filter<float>,
                                   juce::dsp::IIR::Coefficients<float>> highPass;
    juce::dsp::ProcessorDuplicator<juce::dsp::IIR::Filter<float>,
                                   juce::dsp::IIR::Coefficients<float>> lowPass;

    double sampleRate = 44100.0;
    float lowHz = 0.0f;
    float highHz = 20000.0f;
    bool enabled = true;
    bool dirty = true;
    bool useHighPass = false;
    bool useLowPass = true;

    void updateCoefficients();
};

} // namespace sv
