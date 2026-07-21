#pragma once

#include <array>
#include <atomic>
#include <cstdint>

namespace sv
{

constexpr int kMaxSources = 32;
constexpr int kMaxNameChars = 32;

/** Polar sound-field bins: angle (L..front..R) x radius (quiet..loud). */
constexpr int kPolarAngleBins = 48;
constexpr int kPolarRadiusBins = 20;

inline int polarIndex (int angleBin, int radiusBin) noexcept
{
    return radiusBin * kPolarAngleBins + angleBin;
}

/**
 * Sender frame: Mid/Side polar density map (Insight-style) + dynamics scalars.
 */
struct SourceSnapshot
{
    uint32_t sourceId = 0;
    char name[kMaxNameChars] {};
    uint32_t colourARGB = 0xff4ecdc4;

    /** Polar energy map. angle 0 = hard L, centre = front, last = hard R. */
    std::array<float, kPolarAngleBins * kPolarRadiusBins> polarField {};

    /** Weighted stereo centroid [-1, 1]. Tracks bus pan. */
    float panCentroid = 0.0f;

    /** L/R correlation [-1, 1]. +1 mono, 0 wide, -1 out of phase. */
    float correlation = 1.0f;

    float leftEnergy = 0.0f;
    float centreEnergy = 0.0f;
    float rightEnergy = 0.0f;
    float energy = 0.0f;
    float bandEnergy = 0.0f;
    float spectralFocus = 0.0f;
    float diffuseness = 0.0f;
    float crest = 0.5f;
    float punch = 0.0f;
    float density = 0.5f;

    uint64_t samplePosition = 0;
    bool active = false;
};

struct SourceSlot
{
    std::atomic<bool> occupied { false };
    std::atomic<uint32_t> generation { 0 };
    SourceSnapshot snapshot {};
};

} // namespace sv
