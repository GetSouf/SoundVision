#include "ui/SpatialView.h"

namespace sv
{

SpatialView::SpatialView()
{
    setOpaque (true);
    startTimerHz (60);
}

SpatialView::~SpatialView()
{
    stopTimer();
}

void SpatialView::setSnapshotProvider (SnapshotProvider newProvider)
{
    provider = std::move (newProvider);
}

void SpatialView::setBandLabel (float newLowHz, float newHighHz)
{
    lowHz = newLowHz;
    highHz = newHighHz;
}

void SpatialView::resized()
{
}

void SpatialView::timerCallback()
{
    fieldSources.clear();
    latestSnapshots.clear();

    if (provider)
    {
        latestSnapshots = provider();
        fieldSources.reserve (latestSnapshots.size());

        for (const auto& snap : latestSnapshots)
        {
            FieldSource fs;
            fs.sourceId = snap.sourceId;
            fs.colour = juce::Colour (snap.colourARGB);
            fs.polarField = snap.polarField;
            fs.correlation = snap.correlation;
            fs.panCentroid = snap.panCentroid;
            fs.bandEnergy = snap.bandEnergy;
            fieldSources.push_back (fs);
        }
    }

    repaint();
}

void SpatialView::drawGrid (juce::Graphics& g, juce::Rectangle<float> area, juce::Point<float> centre) const
{
    g.setColour (juce::Colour (0xff1b2430));
    g.fillRoundedRectangle (area, 14.0f);

    const float scale = juce::jmin (area.getWidth(), area.getHeight()) * 0.42f;

    g.setColour (juce::Colour (0xff2a3544));
    for (float r : { 0.35f, 0.6f, 0.85f })
        g.drawEllipse (centre.x - scale * r, centre.y - scale * r, scale * r * 2.0f, scale * r * 2.0f, 1.0f);

    // Diagonal guides like Insight.
    for (float t : { -0.55f, 0.0f, 0.55f })
    {
        const float x0 = centre.x + t * scale;
        g.drawLine (x0, centre.y, centre.x, centre.y - scale * 0.9f, 1.0f);
    }

    g.drawLine (centre.x - scale, centre.y, centre.x + scale, centre.y, 1.0f);

    g.setColour (juce::Colours::white.withAlpha (0.35f));
    g.setFont (juce::FontOptions (11.0f));
    g.drawText ("L", juce::Rectangle<float> (area.getX() + 10.0f, centre.y - 8.0f, 20.0f, 16.0f),
                juce::Justification::centredLeft);
    g.drawText ("R", juce::Rectangle<float> (area.getRight() - 30.0f, centre.y - 8.0f, 20.0f, 16.0f),
                juce::Justification::centredRight);
    g.drawText ("FRONT", juce::Rectangle<float> (centre.x - 30.0f, area.getY() + 8.0f, 60.0f, 16.0f),
                juce::Justification::centred);
}

void SpatialView::drawHead (juce::Graphics& g, juce::Point<float> centre) const
{
    g.setColour (juce::Colour (0xffd9e2ec));
    g.fillEllipse (centre.x - 12.0f, centre.y - 14.0f, 24.0f, 28.0f);
    g.setColour (juce::Colour (0xff9aa7b5));
    g.drawEllipse (centre.x - 12.0f, centre.y - 14.0f, 24.0f, 28.0f, 1.5f);
    g.fillEllipse (centre.x - 18.0f, centre.y - 4.0f, 7.0f, 10.0f);
    g.fillEllipse (centre.x + 11.0f, centre.y - 4.0f, 7.0f, 10.0f);
}

void SpatialView::paint (juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat().reduced (2.0f);
    const auto centre = bounds.getCentre();

    drawGrid (g, bounds, centre);
    fieldRenderer.paint (g, bounds, centre, fieldSources, viewStyle, detailScale);
    drawHead (g, centre);

    g.setColour (juce::Colours::white.withAlpha (0.7f));
    g.setFont (juce::FontOptions (13.0f));
    const auto bandText = juce::String ((int) lowHz) + " - " + juce::String ((int) highHz) + " Hz";
    g.drawText (bandText, bounds.reduced (12.0f).removeFromBottom (22.0f), juce::Justification::centredLeft);
}

} // namespace sv
