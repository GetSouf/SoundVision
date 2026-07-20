#include "ui/SpatialView.h"

namespace sv
{

SpatialView::SpatialView()
{
    setOpaque (true);
    lastTickSeconds = juce::Time::getMillisecondCounterHiRes() * 0.001;
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

void SpatialView::setBandLabel (float newCentreHz, float newBandwidthHz)
{
    centreHz = newCentreHz;
    bandwidthHz = newBandwidthHz;
}

void SpatialView::resized()
{
}

void SpatialView::timerCallback()
{
    const double now = juce::Time::getMillisecondCounterHiRes() * 0.001;
    const float dt = (float) (now - lastTickSeconds);
    lastTickSeconds = now;

    std::vector<VisualSource> visuals;
    latestSnapshots.clear();

    if (provider)
    {
        latestSnapshots = provider();
        visuals.reserve (latestSnapshots.size());

        for (const auto& snap : latestSnapshots)
        {
            VisualSource vs;
            vs.sourceId = snap.sourceId;
            vs.name = juce::String::fromUTF8 (snap.name);
            vs.colour = juce::Colour (snap.colourARGB);
            vs.leftEnergy = snap.leftEnergy;
            vs.rightEnergy = snap.rightEnergy;
            vs.midEnergy = snap.midEnergy;
            vs.sideEnergy = snap.sideEnergy;
            vs.bandEnergy = juce::jmax (snap.bandEnergy, snap.spectralFocus * 0.75f);

            // Prefer band imaging; spectral focus gently boosts overall presence.
            const float boost = 0.85f + 0.15f * snap.spectralFocus;
            vs.leftEnergy *= boost;
            vs.rightEnergy *= boost;
            vs.midEnergy *= boost;
            visuals.push_back (vs);
        }
    }

    particles.update (dt, visuals, emissionScale);
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

    g.drawLine (centre.x - scale, centre.y, centre.x + scale, centre.y, 1.0f);
    g.drawLine (centre.x, centre.y - scale, centre.x, centre.y + scale, 1.0f);

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
    particles.paint (g, bounds, centre);
    drawHead (g, centre);

    g.setColour (juce::Colours::white.withAlpha (0.7f));
    g.setFont (juce::FontOptions (13.0f));
    const auto bandText = juce::String ("Band ")
                        + juce::String (centreHz, 0) + " Hz  +/- "
                        + juce::String (bandwidthHz * 0.5f, 0) + " Hz";
    g.drawText (bandText, bounds.reduced (12.0f).removeFromBottom (22.0f), juce::Justification::centredLeft);
}

} // namespace sv
