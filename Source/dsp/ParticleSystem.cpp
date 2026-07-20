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

void ParticleSystem::emitFromSource (const VisualSource& source, float amount)
{
    const int count = (int) std::ceil ((double) amount);

    for (int i = 0; i < count; ++i)
    {
        if ((int) particles.size() >= maxParticles)
            break;

        const float angle = random.nextFloat() * juce::MathConstants<float>::twoPi;
        const float speed = 0.08f + random.nextFloat() * 0.35f * juce::jmax (0.05f, source.bandEnergy);

        Particle p;
        p.position = source.position;
        p.velocity = { std::cos (angle) * speed, std::sin (angle) * speed };
        p.colour = source.colour;
        p.life = 0.0f;
        p.maxLife = 0.45f + random.nextFloat() * 0.85f;
        p.size = 1.6f + source.bandEnergy * 4.5f * random.nextFloat();
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

    for (auto& source : lastSources)
    {
        const float emitPower = source.bandEnergy * emissionScale * 18.0f * deltaSeconds;
        if (emitPower > 0.02f)
            emitFromSource (source, emitPower);
    }

    for (auto& p : particles)
    {
        p.life += deltaSeconds;
        p.position += p.velocity * deltaSeconds;
        p.velocity *= (1.0f - 0.55f * deltaSeconds); // drag
        p.size *= (1.0f - 0.35f * deltaSeconds);
    }

    particles.erase (std::remove_if (particles.begin(), particles.end(),
                                     [] (const Particle& p)
                                     {
                                         return p.life >= p.maxLife || p.size < 0.4f;
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

    for (const auto& p : particles)
    {
        const float t = juce::jlimit (0.0f, 1.0f, p.life / juce::jmax (0.001f, p.maxLife));
        const float alpha = (1.0f - t) * 0.85f;
        const auto screen = toScreen (p.position);
        g.setColour (p.colour.withAlpha (alpha));
        g.fillEllipse (screen.x - p.size * 0.5f,
                       screen.y - p.size * 0.5f,
                       p.size,
                       p.size);
    }

    for (const auto& source : lastSources)
    {
        const auto screen = toScreen (source.position);
        const float glow = 6.0f + source.bandEnergy * 16.0f;

        g.setColour (source.colour.withAlpha (0.18f + source.bandEnergy * 0.35f));
        g.fillEllipse (screen.x - glow, screen.y - glow, glow * 2.0f, glow * 2.0f);

        g.setColour (source.colour.withAlpha (0.95f));
        g.fillEllipse (screen.x - 4.0f, screen.y - 4.0f, 8.0f, 8.0f);

        g.setColour (juce::Colours::white.withAlpha (0.75f));
        g.setFont (juce::FontOptions (12.0f));
        g.drawText (source.name,
                    juce::Rectangle<float> (screen.x - 40.0f, screen.y + 8.0f, 80.0f, 16.0f),
                    juce::Justification::centred,
                    true);
    }
}

} // namespace sv
