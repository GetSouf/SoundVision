#pragma once

#include "../ipc/SourceSnapshot.h"
#include <JuceHeader.h>
#include <vector>

namespace sv
{

/** Colour-coded source list kept off the spatial canvas. */
class SourceLegend final : public juce::Component
{
public:
    void setSources (std::vector<SourceSnapshot> sources);
    void paint (juce::Graphics& g) override;

private:
    std::vector<SourceSnapshot> sources;
};

} // namespace sv
