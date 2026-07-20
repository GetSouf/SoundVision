#pragma once

#include "../ipc/SourceSnapshot.h"
#include <JuceHeader.h>
#include <cmath>
#include <vector>

namespace sv
{

struct Particle
{
    juce::Point<float> position;
    juce::Point<float> velocity;
    juce::Colour colour;
    float life = 0.0f;
    float maxLife = 1.0f;
    float size = 2.0f;
    uint32_t sourceId = 0;
};

struct VisualSource
{
    uint32_t sourceId = 0;
    juce::String name;
    juce::Colour colour;
    juce::Point<float> position; // normalised scene coords, centre = head
    float energy = 0.0f;
    float bandEnergy = 0.0f;
};

/**
 * Lightweight CPU particle field driven by source energy in the selected band.
 */
class ParticleSystem
{
public:
    void setMaxParticles (int newLimit);
    void clear();

    void update (float deltaSeconds, const std::vector<VisualSource>& sources, float emissionScale);

    void paint (juce::Graphics& g, juce::Rectangle<float> bounds, juce::Point<float> headCentre) const;

    const std::vector<VisualSource>& getSources() const noexcept { return lastSources; }

private:
    std::vector<Particle> particles;
    std::vector<VisualSource> lastSources;
    int maxParticles = 1200;
    juce::Random random;

    void emitFromSource (const VisualSource& source, float amount);
};

/** Maps pan/depth into a 2D point around the virtual head. */
inline juce::Point<float> snapshotToScenePoint (const SourceSnapshot& snap) noexcept
{
    // Place louder sources slightly closer to the head.
    const float radius = juce::jmap (1.0f - snap.energy, 0.35f, 0.92f);
    const float angle = snap.pan * (juce::MathConstants<float>::halfPi); // -90 .. +90 deg
    const float depthBias = snap.depth * 0.18f;

    return {
        std::sin (angle) * radius,
        -std::cos (angle) * radius * 0.72f - depthBias
    };
}

} // namespace sv
