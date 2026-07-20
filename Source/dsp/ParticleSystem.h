#pragma once

#include "../ipc/SourceSnapshot.h"
#include <JuceHeader.h>
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
    float angle = 0.0f;
    float dashLength = 4.0f;
    uint32_t sourceId = 0;
};

struct VisualSource
{
    uint32_t sourceId = 0;
    juce::String name;
    juce::Colour colour;
    float leftEnergy = 0.0f;
    float rightEnergy = 0.0f;
    float midEnergy = 0.0f;
    float sideEnergy = 0.0f;
    float bandEnergy = 0.0f;
    float crest = 0.5f;
    float punch = 0.0f;
    float density = 0.5f;
};

/**
 * Particle clouds around a centred listener.
 * L/R/Mid weights place clouds; crest/punch/density reshape texture (compression etc.).
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
    int maxParticles = 2800;
    juce::Random random;

    void emitCloud (const VisualSource& source, float deltaSeconds, float emissionScale);
    float sampleAngle (const VisualSource& source);
};

} // namespace sv
