#include "params/PluginParameters.h"
#include <cmath>

namespace sv
{

juce::StringArray PluginParameters::colourChoices()
{
    return {
        "Teal", "Coral", "Gold", "Violet",
        "Lime", "Sky", "Rose", "Amber"
    };
}

juce::Colour PluginParameters::colourFromIndex (int index)
{
    static const juce::Colour palette[] {
        juce::Colour (0xff4ecdc4),
        juce::Colour (0xffff6b6b),
        juce::Colour (0xffffd166),
        juce::Colour (0xff9b5de5),
        juce::Colour (0xff90ee90),
        juce::Colour (0xff4ea8de),
        juce::Colour (0xffff8fab),
        juce::Colour (0xffffb703)
    };

    const int n = (int) (sizeof (palette) / sizeof (palette[0]));
    return palette[juce::jlimit (0, n - 1, index)];
}

int PluginParameters::nearestColourIndex (juce::Colour colour)
{
    int best = 0;
    float bestScore = 1.0e9f;

    for (int i = 0; i < colourChoices().size(); ++i)
    {
        const auto c = colourFromIndex (i);
        const float score = std::abs (c.getFloatRed() - colour.getFloatRed())
                          + std::abs (c.getFloatGreen() - colour.getFloatGreen())
                          + std::abs (c.getFloatBlue() - colour.getFloatBlue());
        if (score < bestScore)
        {
            bestScore = score;
            best = i;
        }
    }

    return best;
}

juce::AudioProcessorValueTreeState::ParameterLayout PluginParameters::createParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    params.push_back (std::make_unique<juce::AudioParameterChoice> (
        juce::ParameterID { ParamIDs::mode, 1 },
        "Mode",
        juce::StringArray { "Sender", "Receiver" },
        0));

    params.push_back (std::make_unique<juce::AudioParameterChoice> (
        juce::ParameterID { ParamIDs::sourceColour, 1 },
        "Source Colour",
        colourChoices(),
        0));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { ParamIDs::freqLowHz, 1 },
        "Low Hz",
        juce::NormalisableRange<float> (0.0f, 20000.0f, 1.0f, 0.35f),
        0.0f,
        juce::AudioParameterFloatAttributes().withLabel ("Hz")));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { ParamIDs::freqHighHz, 1 },
        "High Hz",
        juce::NormalisableRange<float> (0.0f, 20000.0f, 1.0f, 0.35f),
        20000.0f,
        juce::AudioParameterFloatAttributes().withLabel ("Hz")));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { ParamIDs::particleRate, 1 },
        "Detail",
        juce::NormalisableRange<float> (0.2f, 2.5f, 0.01f),
        1.1f));

    params.push_back (std::make_unique<juce::AudioParameterBool> (
        juce::ParameterID { ParamIDs::showOnlyBand, 1 },
        "Filter Audio",
        false));

    return { params.begin(), params.end() };
}

} // namespace sv
