#pragma once

#include <array>
#include <atomic>
#include <cstdint>

namespace sv
{

constexpr int kMaxSources = 32;
constexpr int kMaxNameChars = 32;
constexpr int kAngularBins = 72; // continuous L <-> R soundstage (-90 .. +90 deg)

/**
 * Sender frame: a continuous stereo-field energy map (what headphones hear),
 * not three discrete L/M/R blobs.
 */
struct SourceSnapshot
{
    uint32_t sourceId = 0;
    char name[kMaxNameChars] {};
    uint32_t colourARGB = 0xff4ecdc4;

    /** Normalised energy across the stereo stage. Index 0 = hard left, last = hard right. */
    std::array<float, kAngularBins> field {};

    float leftEnergy = 0.0f;   // integral of left third (legend)
    float centreEnergy = 0.0f; // integral of centre third
    float rightEnergy = 0.0f;  // integral of right third
    float energy = 0.0f;
    float bandEnergy = 0.0f;
    float spectralFocus = 0.0f;

    /** 0 = focused/dry-ish, 1 = decorrelated/diffuse (reverb fills space). */
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
