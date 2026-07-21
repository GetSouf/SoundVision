#pragma once

#include "../ipc/SourceSnapshot.h"
#include <JuceHeader.h>
#include <vector>

namespace sv
{

enum class ViewStyle
{
    insight = 0,
    grid = 1
};

struct FieldSource
{
    uint32_t sourceId = 0;
    juce::Colour colour;
    std::array<float, kPolarAngleBins * kPolarRadiusBins> polarField {};
    float correlation = 1.0f;
    float panCentroid = 0.0f;
    float bandEnergy = 0.0f;
};

/** Renders Mid/Side polar field — Insight point cloud or Grid dots. */
class FieldRenderer
{
public:
    void paint (juce::Graphics& g,
                juce::Rectangle<float> bounds,
                juce::Point<float> headCentre,
                const std::vector<FieldSource>& sources,
                ViewStyle style,
                float detailScale) const;

private:
    juce::Point<float> polarToScene (int angleBin, int radiusBin) const noexcept;

    void paintInsight (juce::Graphics& g,
                       juce::Rectangle<float> bounds,
                       juce::Point<float> headCentre,
                       const std::vector<FieldSource>& sources,
                       float detailScale) const;

    void paintGrid (juce::Graphics& g,
                    juce::Rectangle<float> bounds,
                    juce::Point<float> headCentre,
                    const std::vector<FieldSource>& sources,
                    float detailScale) const;

    void paintCorrelationMeter (juce::Graphics& g,
                                juce::Rectangle<float> bounds,
                                const std::vector<FieldSource>& sources) const;
};

} // namespace sv
