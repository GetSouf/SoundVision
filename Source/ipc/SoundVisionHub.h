#pragma once

#include "SourceSnapshot.h"
#include <array>
#include <mutex>
#include <optional>
#include <vector>

namespace sv
{

/**
 * Process-wide hub shared by all SoundVision instances in the host.
 * Senders publish snapshots; Receiver(s) pull an aggregated view for UI/DSP.
 */
class SoundVisionHub
{
public:
    SoundVisionHub() = default;

    SoundVisionHub(const SoundVisionHub&) = delete;
    SoundVisionHub& operator= (const SoundVisionHub&) = delete;

    /** Reserve a free slot. Returns nullopt if the hub is full. */
    std::optional<int> registerSource (uint32_t preferredId);

    void unregisterSource (int slotIndex);

    void publish (int slotIndex, const SourceSnapshot& snapshot);

    /** Copy all currently occupied snapshots (message/UI thread friendly). */
    std::vector<SourceSnapshot> collectActiveSources() const;

    int getOccupiedCount() const;

private:
    mutable std::mutex mutex;
    std::array<SourceSlot, kMaxSources> slots {};
};

} // namespace sv
