#include "dsp/ParticleSystem.h"
#include <algorithm>
#include <cmath>

namespace sv
{

void ParticleSystem::setMaxParticles (int newLimit)
{
    maxParticles = juce::jmax (64, newLimit);

    if ((int) particles.size() > maxParticles)
        particles.resize ((size_t) maxParticles);
}

void ParticleSystem::clear()
{
    particles.clear();
    lastSources.clear();
}

void ParticleSystem::emitLobe (juce::Point<float> origin,
                               float energy,
                               juce::Colour colour,
                               uint32_t sourceId,
                               float amount,
                               float outwardBiasX)
{
    if (energy < 0.02f || amount < 0.015f)
        return;

    const int count = (int) std::ceil ((double) amount);

    for (int i = 0; i < count; ++i)
    {
        if ((int) particles.size() >= maxParticles)
            break;

        const float jitterAngle = (random.nextFloat() - 0.5f) * 1.1f;
        const float speed = 0.05f + random.nextFloat() * 0.22f * energy;

        Particle p;
        p.position = {
            origin.x + (random.nextFloat() - 0.5f) * 0.06f,
            origin.y + (random.nextFloat() - 0.5f) * 0.06f
        };
        p.velocity = {
            std::cos (jitterAngle) * speed * outwardBiasX,
            std::sin (jitterAngle) * speed - 0.02f
        };
        p.colour = colour;
        p.life = 0.0f;
        p.maxLife = 0.55f + random.nextFloat() * 0.7f;
        p.size = 1.4f + energy * 3.8f * random.nextFloat();
        p.sourceId = sourceId;
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
    {
        const float width = source.sideEnergy
                          / juce::jmax (0.05f, source.midEnergy + source.sideEnergy);
        const float radius = 0.52f + width * 0.28f;

        // Fixed lobes around the head — no orbiting centroid.
        const juce::Point<float> leftOrigin { -radius, 0.02f };
        const juce::Point<float> rightOrigin { radius, 0.02f };
        const juce::Point<float> midOrigin { 0.0f, -0.28f - source.midEnergy * 0.08f };

        const float scale = emissionScale * 14.0f * deltaSeconds;
        emitLobe (leftOrigin, source.leftEnergy, source.colour, source.sourceId,
                  source.leftEnergy * scale, -1.0f);
        emitLobe (rightOrigin, source.rightEnergy, source.colour, source.sourceId,
                  source.rightEnergy * scale, 1.0f);
        emitLobe (midOrigin, source.midEnergy, source.colour, source.sourceId,
                  source.midEnergy * scale * 0.85f, 0.0f);
    }

    for (auto& p : particles)
    {
        p.life += deltaSeconds;
        p.position += p.velocity * deltaSeconds;
        p.velocity *= (1.0f - 0.65f * deltaSeconds);
        p.size *= (1.0f - 0.28f * deltaSeconds);
    }

    particles.erase (std::remove_if (particles.begin(), particles.end(),
                                     [] (const Particle& p)
                                     {
                                         return p.life >= p.maxLife || p.size < 0.35f;
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

    // Soft energy wash (no labels / no bouncing markers).
    for (const auto& source : lastSources)
    {
        const float width = source.sideEnergy
                          / juce::jmax (0.05f, source.midEnergy + source.sideEnergy);
        const float radius = 0.52f + width * 0.28f;

        auto wash = [&] (juce::Point<float> origin, float energy)
        {
            if (energy < 0.03f)
                return;

            const auto screen = toScreen (origin);
            const float glow = 10.0f + energy * 28.0f;
            g.setColour (source.colour.withAlpha (0.10f + energy * 0.28f));
            g.fillEllipse (screen.x - glow, screen.y - glow, glow * 2.0f, glow * 2.0f);
        };

        wash ({ -radius, 0.02f }, source.leftEnergy);
        wash ({ radius, 0.02f }, source.rightEnergy);
        wash ({ 0.0f, -0.28f }, source.midEnergy);
    }

    for (const auto& p : particles)
    {
        const float t = juce::jlimit (0.0f, 1.0f, p.life / juce::jmax (0.001f, p.maxLife));
        const float alpha = (1.0f - t) * 0.8f;
        const auto screen = toScreen (p.position);
        g.setColour (p.colour.withAlpha (alpha));
        g.fillEllipse (screen.x - p.size * 0.5f,
                       screen.y - p.size * 0.5f,
                       p.size,
                       p.size);
    }
}

} // namespace sv
