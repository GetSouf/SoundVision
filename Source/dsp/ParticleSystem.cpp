#include "dsp/ParticleSystem.h"
#include <algorithm>
#include <cmath>

namespace sv
{

void ParticleSystem::setMaxParticles (int newLimit)
{
    maxParticles = juce::jmax (128, newLimit);
    if ((int) particles.size() > maxParticles)
        particles.resize ((size_t) maxParticles);
}

void ParticleSystem::clear()
{
    particles.clear();
    lastSources.clear();
}

int ParticleSystem::sampleBin (const VisualSource& source)
{
    float total = 0.0f;
    for (float v : source.field)
        total += v;

    if (total < 1.0e-6f)
        return kAngularBins / 2;

    float pick = random.nextFloat() * total;
    for (int i = 0; i < kAngularBins; ++i)
    {
        pick -= source.field[(size_t) i];
        if (pick <= 0.0f)
            return i;
    }

    return kAngularBins - 1;
}

void ParticleSystem::emitFromField (const VisualSource& source, float deltaSeconds, float emissionScale)
{
    float fieldMass = 0.0f;
    for (float v : source.field)
        fieldMass += v;

    const float presence = juce::jmax (source.bandEnergy, fieldMass / (float) kAngularBins);
    if (presence < 0.012f)
        return;

    const float densityBias = 0.40f + source.density * 1.25f + source.diffuseness * 0.55f;
    const float punchBias = 0.50f + source.punch * 0.95f;
    const float spawn = presence * emissionScale * densityBias * punchBias * 70.0f * deltaSeconds;
    const int count = (int) std::ceil ((double) spawn);

    for (int i = 0; i < count; ++i)
    {
        if ((int) particles.size() >= maxParticles)
            break;

        const int bin = sampleBin (source);
        const float pan = (float) bin / (float) (kAngularBins - 1) * 2.0f - 1.0f; // -1..1
        const float azimuth = pan * juce::MathConstants<float>::halfPi; // -90..+90

        // Near head when loud/focused; farther when diffuse/quiet.
        const float radius = 0.28f
                           + random.nextFloat() * (0.50f + source.diffuseness * 0.22f)
                           + (1.0f - presence) * 0.08f;

        const float x = std::sin (azimuth) * radius;
        const float y = -std::cos (azimuth) * radius * 0.78f;

        const float swirl = (random.nextFloat() - 0.5f) * (0.06f + source.crest * 0.22f + source.diffuseness * 0.12f);
        const float radial = 0.015f + source.punch * random.nextFloat() * 0.30f;

        Particle p;
        p.position = {
            x + (random.nextFloat() - 0.5f) * 0.04f,
            y + (random.nextFloat() - 0.5f) * 0.04f
        };
        p.velocity = {
            std::cos (azimuth) * swirl + std::sin (azimuth) * radial * 0.3f,
            -std::sin (azimuth) * swirl - std::cos (azimuth) * radial * 0.22f
        };
        p.colour = source.colour;
        p.life = 0.0f;
        p.maxLife = 0.30f + random.nextFloat() * (0.50f + source.density * 0.55f + source.diffuseness * 0.25f);
        p.size = 1.0f + presence * (1.1f + source.density * 2.0f) * random.nextFloat();
        p.angle = azimuth + (random.nextFloat() - 0.5f) * 0.9f;
        p.dashLength = 2.5f + source.crest * 9.0f * random.nextFloat() + source.punch * 5.0f;
        p.sourceId = source.sourceId;
        particles.push_back (p);
    }
}

void ParticleSystem::update (float deltaSeconds,
                             const std::vector<VisualSource>& sources,
                             float emissionScale)
{
    lastSources = sources;
    deltaSeconds = juce::jlimit (0.0f, 0.05f, deltaSeconds);

    for (const auto& source : lastSources)
        emitFromField (source, deltaSeconds, emissionScale);

    for (auto& p : particles)
    {
        p.life += deltaSeconds;
        p.position += p.velocity * deltaSeconds;
        p.velocity *= (1.0f - 0.45f * deltaSeconds);
        p.size *= (1.0f - 0.18f * deltaSeconds);
        p.angle += p.velocity.x * 0.8f;
    }

    particles.erase (std::remove_if (particles.begin(), particles.end(),
                                     [] (const Particle& p)
                                     {
                                         return p.life >= p.maxLife || p.size < 0.3f;
                                     }),
                     particles.end());
}

void ParticleSystem::paint (juce::Graphics& g,
                            juce::Rectangle<float> bounds,
                            juce::Point<float> headCentre) const
{
    const float scale = juce::jmin (bounds.getWidth(), bounds.getHeight()) * 0.42f;

    auto toScreen = [&] (juce::Point<float> scene) -> juce::Point<float>
    {
        return { headCentre.x + scene.x * scale, headCentre.y + scene.y * scale };
    };

    // Soft field wash behind particles (continuous, not three blobs).
    for (const auto& source : lastSources)
    {
        for (int i = 0; i < kAngularBins; i += 2)
        {
            const float v = source.field[(size_t) i];
            if (v < 0.08f)
                continue;

            const float pan = (float) i / (float) (kAngularBins - 1) * 2.0f - 1.0f;
            const float azimuth = pan * juce::MathConstants<float>::halfPi;
            const float radius = 0.55f + source.diffuseness * 0.12f;
            const auto screen = toScreen ({ std::sin (azimuth) * radius,
                                            -std::cos (azimuth) * radius * 0.78f });
            const float glow = 4.0f + v * 14.0f;
            g.setColour (source.colour.withAlpha (0.04f + v * 0.14f));
            g.fillEllipse (screen.x - glow, screen.y - glow, glow * 2.0f, glow * 2.0f);
        }
    }

    for (const auto& p : particles)
    {
        const float t = juce::jlimit (0.0f, 1.0f, p.life / juce::jmax (0.001f, p.maxLife));
        const float alpha = (1.0f - t) * 0.80f;
        const auto screen = toScreen (p.position);
        const float dx = std::cos (p.angle) * p.dashLength * 0.5f;
        const float dy = std::sin (p.angle) * p.dashLength * 0.5f;

        g.setColour (p.colour.withAlpha (alpha));
        g.drawLine (screen.x - dx, screen.y - dy, screen.x + dx, screen.y + dy,
                    juce::jmax (1.0f, p.size * 0.55f));
    }
}

} // namespace sv
