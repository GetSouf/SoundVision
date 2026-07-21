#pragma once

#include "dsp/StereoAnalyzer.h"
#include <JuceHeader.h>
#include <array>
#include <functional>

namespace sv
{

/** Compact horizontal spectrum with Low/High cut handles (synced to APVTS knobs). */
class BandSpectrumView final : public juce::Component
{
public:
    using FreqSetter = std::function<void (float lowHz, float highHz)>;

    void setSpectrum (const std::array<float, kSpectrumBins>& values);
    void setCuts (float lowHz, float highHz);
    void setFreqSetter (FreqSetter setter);

    void paint (juce::Graphics& g) override;
    void mouseDown (const juce::MouseEvent& e) override;
    void mouseDrag (const juce::MouseEvent& e) override;
    void mouseUp (const juce::MouseEvent& e) override;

private:
    enum class DragTarget { none, low, high };

    std::array<float, kSpectrumBins> spectrum {};
    float lowHz = 0.0f;
    float highHz = 20000.0f;
    FreqSetter freqSetter;
    DragTarget drag = DragTarget::none;

    float hzToX (float hz, float width) const noexcept;
    float xToHz (float x, float width) const noexcept;
    DragTarget hitTest (float x, float width) const noexcept;
    void applyDrag (float x, float width);
};

} // namespace sv
