#pragma once

#include "dsp/ParticleSystem.h"
#include <JuceHeader.h>
#include <functional>

namespace sv
{

class SpatialView final : public juce::Component, private juce::Timer
{
public:
    using SnapshotProvider = std::function<std::vector<SourceSnapshot>()>;

    SpatialView();
    ~SpatialView() override;

    void setSnapshotProvider (SnapshotProvider provider);
    void setEmissionScale (float scale) noexcept { emissionScale = scale; }
    void setBandLabel (float centreHz, float bandwidthHz);

    void paint (juce::Graphics& g) override;
    void resized() override;

private:
    void timerCallback() override;
    void drawHead (juce::Graphics& g, juce::Point<float> centre) const;
    void drawGrid (juce::Graphics& g, juce::Rectangle<float> area, juce::Point<float> centre) const;

    SnapshotProvider provider;
    ParticleSystem particles;
    float emissionScale = 1.0f;
    float centreHz = 500.0f;
    float bandwidthHz = 200.0f;
    double lastTickSeconds = 0.0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SpatialView)
};

} // namespace sv
