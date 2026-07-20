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

float ParticleSystem::sampleAngle (const VisualSource& source)
{
    // 0 = front (up), -pi/2 = left, +pi/2 = right.
    const float weights[3] {
        juce::jmax (0.001f, source.leftEnergy),
        juce::jmax (0.001f, source.midEnergy),
        juce::jmax (0.001f, source.rightEnergy)
    };
    const float centres[3] {
        -juce::MathConstants<float>::halfPi,
        0.0f,
        juce::MathConstants<float>::halfPi
    };

    const float total = weights[0] + weights[1] + weights[2];
    float pick = random.nextFloat() * total;
    int lobe = 2;
    for (int i = 0; i < 3; ++i)
    {
        pick -= weights[i];
        if (pick <= 0.0f)
        {
            lobe = i;
            break;
        }
    }

    const float widthSpread = 0.18f + source.sideEnergy * 0.55f;
    const float focus = juce::jmap (source.density, 0.35f, 0.12f);
    const float sigma = widthSpread * focus + (lobe == 1 ? 0.22f : 0.28f);
    return centres[lobe] + (random.nextFloat() - 0.5f) * 2.0f * sigma;
}

void ParticleSystem::emitCloud (const VisualSource& source, float deltaSeconds, float emissionScale)
{
    const float presence = juce::jmax (source.bandEnergy,
                                       0.55f * (source.leftEnergy + source.midEnergy + source.rightEnergy) / 3.0f);
    if (presence < 0.015f)
        return;

    const float densityBias = 0.35f + source.density * 1.4f;
    const float punchBias = 0.45f + source.punch * 1.1f;
    const float spawn = presence * emissionScale * densityBias * punchBias * 55.0f * deltaSeconds;
    const int count = (int) std::ceil ((double) spawn);

    for (int i = 0; i < count; ++i)
    {
        if ((int) particles.size() >= maxParticles)
            break;

        const float angle = sampleAngle (source);
        const float radius = 0.35f + random.nextFloat() * (0.45f + source.sideEnergy * 0.18f);
        const float x = std::sin (angle) * radius;
        const float y = -std::cos (angle) * radius * 0.78f;

        const float swirl = (random.nextFloat() - 0.5f) * (0.08f + source.crest * 0.25f);
        const float radial = 0.02f + source.punch * random.nextFloat() * 0.35f;

        Particle p;
        p.position = { x, y };
        p.velocity = {
            std::cos (angle) * swirl + std::sin (angle) * radial * 0.35f,
            -std::sin (angle) * swirl - std::cos (angle) * radial * 0.25f
        };
        p.colour = source.colour;
        p.life = 0.0f;
        p.maxLife = 0.35f + random.nextFloat() * (0.55f + source.density * 0.55f);
        p.size = 1.0f + presence * (1.2f + source.density * 2.2f) * random.nextFloat();
        p.angle = angle + (random.nextFloat() - 0.5f) * 0.8f;
        p.dashLength = 3.0f + source.crest * 10.0f * random.nextFloat() + source.punch * 6.0f;
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
        emitCloud (source, deltaSeconds, emissionScale);

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

    for (const auto& p : particles)
    {
        const float t = juce::jlimit (0.0f, 1.0f, p.life / juce::jmax (0.001f, p.maxLife));
        const float alpha = (1.0f - t) * 0.78f;
        const auto screen = toScreen (p.position);
        const float dx = std::cos (p.angle) * p.dashLength * 0.5f;
        const float dy = std::sin (p.angle) * p.dashLength * 0.5f;

        g.setColour (p.colour.withAlpha (alpha));
        g.drawLine (screen.x - dx, screen.y - dy, screen.x + dx, screen.y + dy,
                    juce::jmax (1.0f, p.size * 0.55f));
    }
}

} // namespace sv
