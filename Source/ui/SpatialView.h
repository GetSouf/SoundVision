#pragma once

#include "ui/FieldRenderer.h"
#include <JuceHeader.h>
#include <functional>
#include <vector>

namespace sv
{

class SpatialView final : public juce::Component, private juce::Timer
{
public:
    using SnapshotProvider = std::function<std::vector<SourceSnapshot>()>;

    SpatialView();
    ~SpatialView() override;

    void setSnapshotProvider (SnapshotProvider provider);
    void setDetailScale (float scale) noexcept { detailScale = scale; }
    void setViewStyle (ViewStyle style) noexcept { viewStyle = style; }
    void setBandLabel (float lowHz, float highHz);
    std::vector<SourceSnapshot> getLatestSnapshots() const { return latestSnapshots; }

    void paint (juce::Graphics& g) override;
    void resized() override;

private:
    void timerCallback() override;
    void drawHead (juce::Graphics& g, juce::Point<float> centre) const;
    void drawGrid (juce::Graphics& g, juce::Rectangle<float> area, juce::Point<float> centre) const;

    SnapshotProvider provider;
    FieldRenderer fieldRenderer;
    std::vector<FieldSource> fieldSources;
    std::vector<SourceSnapshot> latestSnapshots;
    ViewStyle viewStyle = ViewStyle::insight;
    float detailScale = 1.0f;
    float lowHz = 20.0f;
    float highHz = 20000.0f;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SpatialView)
};

} // namespace sv
