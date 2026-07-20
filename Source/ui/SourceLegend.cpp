#include "ui/SourceLegend.h"

namespace sv
{

void SourceLegend::setSources (std::vector<SourceSnapshot> newSources)
{
    sources = std::move (newSources);
    repaint();
}

void SourceLegend::paint (juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat().reduced (1.0f);
    g.setColour (juce::Colour (0xff1b2430));
    g.fillRoundedRectangle (bounds, 10.0f);
    g.setColour (juce::Colour (0xff334155));
    g.drawRoundedRectangle (bounds, 10.0f, 1.0f);

    auto area = bounds.reduced (10.0f);
    g.setColour (juce::Colour (0xffe8eef5));
    g.setFont (juce::FontOptions (13.0f, juce::Font::bold));
    g.drawText ("Sources", area.removeFromTop (18.0f), juce::Justification::centredLeft);

    g.setFont (juce::FontOptions (11.0f));
    g.setColour (juce::Colour (0xff9aa7b5));
    g.drawText ("L / M / R = imaging lobes", area.removeFromTop (16.0f), juce::Justification::centredLeft);
    area.removeFromTop (6.0f);

    if (sources.empty())
    {
        g.drawText ("No active senders", area.removeFromTop (20.0f), juce::Justification::centredLeft);
        return;
    }

    for (const auto& snap : sources)
    {
        auto row = area.removeFromTop (28.0f);
        area.removeFromTop (4.0f);

        g.setColour (juce::Colour (snap.colourARGB));
        g.fillEllipse (row.getX(), row.getCentreY() - 6.0f, 12.0f, 12.0f);

        g.setColour (juce::Colour (0xffe8eef5));
        g.setFont (juce::FontOptions (12.0f));
        const auto name = juce::String::fromUTF8 (snap.name);
        g.drawText (name.isNotEmpty() ? name : "Source",
                    row.withTrimmedLeft (18.0f).removeFromLeft (100.0f),
                    juce::Justification::centredLeft,
                    true);

        auto meters = row.withTrimmedLeft (120.0f);
        const float meterW = meters.getWidth() / 3.0f;
        const float values[3] { snap.leftEnergy, snap.midEnergy, snap.rightEnergy };
        const char* labels[3] { "L", "M", "R" };

        for (int i = 0; i < 3; ++i)
        {
            auto cell = meters.removeFromLeft (meterW).reduced (2.0f, 4.0f);
            g.setColour (juce::Colour (0xff0f141a));
            g.fillRoundedRectangle (cell, 3.0f);

            auto fill = cell;
            fill.setWidth (cell.getWidth() * juce::jlimit (0.0f, 1.0f, values[i]));
            g.setColour (juce::Colour (snap.colourARGB).withAlpha (0.85f));
            g.fillRoundedRectangle (fill, 3.0f);

            g.setColour (juce::Colours::white.withAlpha (0.55f));
            g.setFont (juce::FontOptions (9.0f));
            g.drawText (labels[i], cell, juce::Justification::centred);
        }
    }
}

} // namespace sv
