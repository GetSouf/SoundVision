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
};

/**
 * Particle field decoded from L / R / Mid energies.
 * One source can illuminate both ears (wide/side content) without a second label.
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
    int maxParticles = 1600;
    juce::Random random;

    void emitLobe (juce::Point<float> origin,
                   float energy,
                   juce::Colour colour,
                   uint32_t sourceId,
                   float amount,
                   float outwardBiasX);
};

} // namespace sv
