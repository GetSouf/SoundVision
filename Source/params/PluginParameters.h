#pragma once

#include <JuceHeader.h>

namespace sv
{

enum class PluginMode
{
    sender = 0,
    receiver = 1
};

namespace ParamIDs
{
inline constexpr const char* mode = "mode";
inline constexpr const char* sourceName = "sourceName";
inline constexpr const char* sourceColour = "sourceColour";
inline constexpr const char* centreHz = "centreHz";
inline constexpr const char* bandwidthHz = "bandwidthHz";
inline constexpr const char* particleRate = "particleRate";
inline constexpr const char* showOnlyBand = "showOnlyBand";
} // namespace ParamIDs

struct PluginParameters
{
    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

    static juce::StringArray colourChoices();
    static juce::Colour colourFromIndex (int index);
    static int nearestColourIndex (juce::Colour colour);
};

} // namespace sv
