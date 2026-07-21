#include "ui/FieldRenderer.h"
#include <cmath>

namespace sv
{

juce::Point<float> FieldRenderer::polarToScene (int angleBin, int radiusBin) const noexcept
{
    const float angleNorm = (float) angleBin / (float) juce::jmax (1, kPolarAngleBins - 1);
    const float azimuth = (angleNorm - 0.5f) * juce::MathConstants<float>::pi;
    const float radius = 0.12f + ((float) radiusBin / (float) juce::jmax (1, kPolarRadiusBins - 1)) * 0.78f;

    return {
        std::sin (azimuth) * radius,
        -std::cos (azimuth) * radius * 0.88f
    };
}

void FieldRenderer::paintCorrelationMeter (juce::Graphics& g,
                                           juce::Rectangle<float> bounds,
                                           const std::vector<FieldSource>& sources) const
{
    auto meter = bounds.removeFromRight (22.0f).reduced (4.0f, 18.0f);
    g.setColour (juce::Colour (0xff2a3544));
    g.fillRoundedRectangle (meter, 4.0f);

    float corr = 0.0f;
    float weight = 0.0f;
    for (const auto& s : sources)
    {
        const float w = juce::jmax (0.01f, s.bandEnergy);
        corr += s.correlation * w;
        weight += w;
    }
    if (weight > 0.0f)
        corr /= weight;

    const float t = (corr + 1.0f) * 0.5f;
    const float y = meter.getY() + meter.getHeight() * (1.0f - t);
    g.setColour (juce::Colour (0xff4ecdc4));
    g.fillRoundedRectangle (meter.getX() + 3.0f, y - 2.0f, meter.getWidth() - 6.0f, 4.0f, 2.0f);

    g.setColour (juce::Colours::white.withAlpha (0.45f));
    g.setFont (juce::FontOptions (8.0f));
    g.drawText ("+1", meter.translated (0, -10.0f), juce::Justification::centred);
    g.drawText ("0", meter.withHeight (12.0f).withCentre (meter.getCentre()), juce::Justification::centred);
    g.drawText ("-1", meter.translated (0, meter.getHeight() - 2.0f), juce::Justification::centred);
}

void FieldRenderer::paintInsight (juce::Graphics& g,
                                  juce::Rectangle<float> bounds,
                                  juce::Point<float> headCentre,
                                  const std::vector<FieldSource>& sources,
                                  float detailScale) const
{
    const float scale = juce::jmin (bounds.getWidth(), bounds.getHeight()) * 0.42f;
    juce::Random rng (12345);

    auto toScreen = [&] (juce::Point<float> scene) -> juce::Point<float>
    {
        return { headCentre.x + scene.x * scale, headCentre.y + scene.y * scale };
    };

    for (const auto& source : sources)
    {
        for (int r = 0; r < kPolarRadiusBins; ++r)
        {
            for (int a = 0; a < kPolarAngleBins; ++a)
            {
                const float energy = source.polarField[(size_t) polarIndex (a, r)];
                if (energy < 0.04f)
                    continue;

                const int dotCount = juce::jmax (1, (int) std::ceil ((double) energy * detailScale * 6.0));
                const auto base = polarToScene (a, r);

                for (int d = 0; d < dotCount; ++d)
                {
                    const float jitter = 0.025f * (1.0f - (float) r / (float) kPolarRadiusBins);
                    const auto scene = juce::Point<float> {
                        base.x + (rng.nextFloat() - 0.5f) * jitter,
                        base.y + (rng.nextFloat() - 0.5f) * jitter
                    };
                    const auto screen = toScreen (scene);
                    const float alpha = juce::jlimit (0.08f, 0.85f, energy * 0.75f);
                    const float size = 1.0f + energy * 2.5f;

                    g.setColour (source.colour.withAlpha (alpha));
                    g.fillEllipse (screen.x - size * 0.5f, screen.y - size * 0.5f, size, size);
                }
            }
        }
    }
}

void FieldRenderer::paintGrid (juce::Graphics& g,
                               juce::Rectangle<float> bounds,
                               juce::Point<float> headCentre,
                               const std::vector<FieldSource>& sources,
                               float detailScale) const
{
    const float scale = juce::jmin (bounds.getWidth(), bounds.getHeight()) * 0.42f;

    auto toScreen = [&] (juce::Point<float> scene) -> juce::Point<float>
    {
        return { headCentre.x + scene.x * scale, headCentre.y + scene.y * scale };
    };

    struct CellContribution
    {
        float energy = 0.0f;
        juce::Colour colour { 0xff000000 };
        int count = 0;
    };

    std::array<CellContribution, kPolarAngleBins * kPolarRadiusBins> composite {};

    for (const auto& source : sources)
    {
        for (int r = 0; r < kPolarRadiusBins; ++r)
        {
            for (int a = 0; a < kPolarAngleBins; ++a)
            {
                const size_t idx = (size_t) polarIndex (a, r);
                const float e = source.polarField[idx];
                if (e < 0.03f)
                    continue;

                auto& cell = composite[idx];
                cell.energy += e;
                cell.colour = cell.colour.overlaidWith (source.colour.withAlpha (e));
                ++cell.count;
            }
        }
    }

    // Grid spacing: every 2 angle bins, every 2 radius bins for readability.
    for (int r = 0; r < kPolarRadiusBins; r += 2)
    {
        for (int a = 0; a < kPolarAngleBins; a += 2)
        {
            const size_t idx = (size_t) polarIndex (a, r);
            const auto& cell = composite[idx];

            float totalE = cell.energy;
            for (int dr = 0; dr <= 1; ++dr)
            {
                for (int da = 0; da <= 1; ++da)
                {
                    if (dr == 0 && da == 0)
                        continue;
                    const int rr = r + dr;
                    const int aa = a + da;
                    if (rr < kPolarRadiusBins && aa < kPolarAngleBins)
                        totalE += composite[(size_t) polarIndex (aa, rr)].energy * 0.5f;
                }
            }

            if (totalE < 0.05f)
                continue;

            const auto screen = toScreen (polarToScene (a, r));
            const float dotSize = 3.0f + totalE * detailScale * 14.0f;

            g.setColour (cell.colour.withAlpha (juce::jlimit (0.25f, 0.95f, totalE)));
            g.fillEllipse (screen.x - dotSize * 0.5f, screen.y - dotSize * 0.5f, dotSize, dotSize);

            // Multiple sources in same cell: small satellite markers.
            if (cell.count > 1)
            {
                g.setColour (juce::Colours::white.withAlpha (0.55f));
                g.setFont (juce::FontOptions (8.0f));
                g.drawText (juce::String (cell.count),
                            juce::Rectangle<float> (screen.x + dotSize * 0.3f,
                                                    screen.y - dotSize * 0.3f,
                                                    12.0f, 10.0f),
                            juce::Justification::centred);
            }
        }
    }
}

void FieldRenderer::paint (juce::Graphics& g,
                           juce::Rectangle<float> bounds,
                           juce::Point<float> headCentre,
                           const std::vector<FieldSource>& sources,
                           ViewStyle style,
                           float detailScale) const
{
    if (style == ViewStyle::insight)
        paintInsight (g, bounds, headCentre, sources, detailScale);
    else
        paintGrid (g, bounds, headCentre, sources, detailScale);

    if (! sources.empty())
        paintCorrelationMeter (g, bounds, sources);
}

} // namespace sv
