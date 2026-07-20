#pragma once

#include <array>
#include <atomic>
#include <cstdint>

namespace sv
{

constexpr int kMaxSources = 32;
constexpr int kMaxNameChars = 32;

/**
 * One analysed frame from a Sender.
 * Imaging is carried as Left / Right / Mid / Side energies so a wide
 * (side-only) track lights both ears without inventing a second source.
 */
struct SourceSnapshot
{
    uint32_t sourceId = 0;
    char name[kMaxNameChars] {};
    uint32_t colourARGB = 0xff4ecdc4;

    float leftEnergy = 0.0f;
    float rightEnergy = 0.0f;
    float midEnergy = 0.0f;
    float sideEnergy = 0.0f;

    /** Soft-capped broadband magnitude. */
    float energy = 0.0f;

    /** Soft-capped selected-band magnitude. */
    float bandEnergy = 0.0f;

    /** FFT focus inside the selected band. */
    float spectralFocus = 0.0f;

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
