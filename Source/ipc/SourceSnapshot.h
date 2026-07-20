#pragma once

#include <array>
#include <atomic>
#include <cstdint>

namespace sv
{

constexpr int kMaxSources = 32;
constexpr int kMaxNameChars = 32;

/** One frame of analysed audio published by a Sender instance. */
struct SourceSnapshot
{
    uint32_t sourceId = 0;
    char name[kMaxNameChars] {};
    uint32_t colourARGB = 0xff4ecdc4;

    /** Stereo pan in [-1, 1] (left -> right). */
    float pan = 0.0f;

    /** Approximate front/back from mid/side balance in [-1, 1]. */
    float depth = 0.0f;

    /** Broadband RMS energy in [0, 1] (soft-capped). */
    float energy = 0.0f;

    /** Energy inside the selected frequency band in [0, 1]. */
    float bandEnergy = 0.0f;

    /** Peak magnitude of the selected FFT bin region in [0, 1]. */
    float spectralFocus = 0.0f;

    uint64_t samplePosition = 0;
    bool active = false;
};

/** Slot for one sender. Writers own their slot; readers copy under hub lock. */
struct SourceSlot
{
    std::atomic<bool> occupied { false };
    std::atomic<uint32_t> generation { 0 };
    SourceSnapshot snapshot {};
};

} // namespace sv
