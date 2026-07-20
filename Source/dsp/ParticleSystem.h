#pragma once

#include "../ipc/SourceSnapshot.h"
#include <JuceHeader.h>
#include <array>
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
    std::array<float, kAngularBins> field {};
    float bandEnergy = 0.0f;
    float diffuseness = 0.0f;
    float crest = 0.5f;
    float punch = 0.0f;
    float density = 0.5f;
    float leftEnergy = 0.0f;
    float centreEnergy = 0.0f;
    float rightEnergy = 0.0f;
};

/** Particles sample the continuous angular field — the headphone soundstage. */
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
    int maxParticles = 3200;
    juce::Random random;

    void emitFromField (const VisualSource& source, float deltaSeconds, float emissionScale);
    int sampleBin (const VisualSource& source);
};

} // namespace sv
