#pragma once

#include <JuceHeader.h>

namespace sv
{

/** Resonant band-pass used to isolate a frequency region for visualisation. */
class BandFilter
{
public:
    void prepare (const juce::dsp::ProcessSpec& spec);
    void reset();

    void setBand (float centreHz, float bandwidthHz);
    void setEnabled (bool shouldBeEnabled) noexcept { enabled = shouldBeEnabled; }

    void process (juce::AudioBuffer<float>& buffer);

private:
    juce::dsp::ProcessorDuplicator<juce::dsp::IIR::Filter<float>,
                                   juce::dsp::IIR::Coefficients<float>> filter;
    double sampleRate = 44100.0;
    float centreHz = 500.0f;
    float bandwidthHz = 200.0f;
    bool enabled = true;
    bool dirty = true;

    void updateCoefficients();
};

} // namespace sv
