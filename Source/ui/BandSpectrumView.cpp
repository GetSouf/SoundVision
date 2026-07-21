#include "ui/BandSpectrumView.h"
#include <cmath>

namespace sv
{

namespace
{
constexpr float kMinHz = 20.0f;
constexpr float kMaxHz = 20000.0f;

float hzToNorm (float hz) noexcept
{
    hz = juce::jlimit (kMinHz, kMaxHz, juce::jmax (kMinHz, hz));
    return std::log (hz / kMinHz) / std::log (kMaxHz / kMinHz);
}

float normToHz (float t) noexcept
{
    t = juce::jlimit (0.0f, 1.0f, t);
    return kMinHz * std::pow (kMaxHz / kMinHz, t);
}
} // namespace

void BandSpectrumView::setSpectrum (const std::array<float, kSpectrumBins>& values)
{
    spectrum = values;
    repaint();
}

void BandSpectrumView::setCuts (float newLowHz, float newHighHz)
{
    lowHz = juce::jlimit (0.0f, 20000.0f, newLowHz);
    highHz = juce::jlimit (juce::jmax (lowHz + 10.0f, 10.0f), 20000.0f, newHighHz);
    repaint();
}

void BandSpectrumView::setFreqSetter (FreqSetter setter)
{
    freqSetter = std::move (setter);
}

float BandSpectrumView::hzToX (float hz, float width) const noexcept
{
    return hzToNorm (juce::jmax (kMinHz, hz)) * width;
}

float BandSpectrumView::xToHz (float x, float width) const noexcept
{
    if (width <= 1.0f)
        return kMinHz;
    return normToHz (x / width);
}

BandSpectrumView::DragTarget BandSpectrumView::hitTest (float x, float width) const noexcept
{
    const float lowX = hzToX (juce::jmax (kMinHz, lowHz), width);
    const float highX = hzToX (highHz, width);
    const float hit = 10.0f;

    if (std::abs (x - lowX) <= hit)
        return DragTarget::low;
    if (std::abs (x - highX) <= hit)
        return DragTarget::high;

    // Prefer nearer handle if clicking in the middle of empty space near one.
    if (std::abs (x - lowX) < std::abs (x - highX))
        return DragTarget::low;
    return DragTarget::high;
}

void BandSpectrumView::applyDrag (float x, float width)
{
    float hz = xToHz (x, width);
    float newLow = lowHz;
    float newHigh = highHz;

    if (drag == DragTarget::low)
        newLow = juce::jlimit (0.0f, newHigh - 10.0f, hz <= kMinHz + 1.0f ? 0.0f : hz);
    else if (drag == DragTarget::high)
        newHigh = juce::jlimit (newLow + 10.0f, 20000.0f, hz);

    lowHz = newLow;
    highHz = newHigh;

    if (freqSetter)
        freqSetter (lowHz, highHz);

    repaint();
}

void BandSpectrumView::mouseDown (const juce::MouseEvent& e)
{
    const float w = (float) getWidth();
    drag = hitTest ((float) e.x, w);
    applyDrag ((float) e.x, w);
}

void BandSpectrumView::mouseDrag (const juce::MouseEvent& e)
{
    if (drag == DragTarget::none)
        return;
    applyDrag ((float) e.x, (float) getWidth());
}

void BandSpectrumView::mouseUp (const juce::MouseEvent&)
{
    drag = DragTarget::none;
}

void BandSpectrumView::paint (juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat().reduced (1.0f);
    g.setColour (juce::Colour (0xff121820));
    g.fillRoundedRectangle (bounds, 8.0f);
    g.setColour (juce::Colour (0xff2a3544));
    g.drawRoundedRectangle (bounds, 8.0f, 1.0f);

    auto plot = bounds.reduced (8.0f, 10.0f);
    const float w = plot.getWidth();
    const float h = plot.getHeight();

    const float lowX = plot.getX() + hzToX (juce::jmax (kMinHz, lowHz <= 0.0f ? kMinHz : lowHz), w);
    const float highX = plot.getX() + hzToX (highHz, w);

    // Dim outside the passband.
    g.setColour (juce::Colour (0xff0a0e14).withAlpha (0.55f));
    g.fillRect (plot.getX(), plot.getY(), juce::jmax (0.0f, lowX - plot.getX()), h);
    g.fillRect (highX, plot.getY(), juce::jmax (0.0f, plot.getRight() - highX), h);

    // Spectrum bars.
    const float barW = w / (float) kSpectrumBins;
    for (int i = 0; i < kSpectrumBins; ++i)
    {
        const float x = plot.getX() + (float) i * barW;
        const float level = juce::jlimit (0.0f, 1.0f, spectrum[(size_t) i]);
        const float barH = level * h;
        const float cx = x + barW * 0.5f;
        const bool inside = cx >= lowX && cx <= highX;

        g.setColour (inside ? juce::Colour (0xff4ecdc4).withAlpha (0.75f)
                            : juce::Colour (0xff4ecdc4).withAlpha (0.22f));
        g.fillRect (x + 0.5f, plot.getBottom() - barH, juce::jmax (1.0f, barW - 1.0f), barH);
    }

    // Cut handles.
    auto drawHandle = [&] (float x, bool active)
    {
        g.setColour (active ? juce::Colour (0xffffd166) : juce::Colour (0xffe8eef5));
        g.drawLine (x, plot.getY(), x, plot.getBottom(), 1.6f);
        g.fillRoundedRectangle (x - 4.0f, plot.getCentreY() - 10.0f, 8.0f, 20.0f, 3.0f);
    };

    drawHandle (lowX, drag == DragTarget::low);
    drawHandle (highX, drag == DragTarget::high);

    g.setColour (juce::Colour (0xff9aa7b5));
    g.setFont (juce::FontOptions (9.0f));
    g.drawText ("20", plot.withHeight (12.0f).withY (plot.getBottom() - 1.0f),
                juce::Justification::centredLeft);
    g.drawText ("20k", plot.withHeight (12.0f).withY (plot.getBottom() - 1.0f),
                juce::Justification::centredRight);
}

} // namespace sv
