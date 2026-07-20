#pragma once

#include <array>
#include <atomic>
#include <cstdint>

namespace sv
{

constexpr int kMaxSources = 32;
constexpr int kMaxNameChars = 32;

struct SourceSnapshot
{
    uint32_t sourceId = 0;
    char name[kMaxNameChars] {};
    uint32_t colourARGB = 0xff4ecdc4;

    float leftEnergy = 0.0f;
    float rightEnergy = 0.0f;
    float midEnergy = 0.0f;
    float sideEnergy = 0.0f;
    float energy = 0.0f;
    float bandEnergy = 0.0f;
    float spectralFocus = 0.0f;

    /** Peak/RMS style punch. High = transient/open dynamics; low = compressed. */
    float crest = 0.5f;

    /** Short/long energy ratio — visible as burstiness. */
    float punch = 0.0f;

    /** Fill factor for clouds: compressed material tends toward denser fog. */
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
