#include "ipc/SoundVisionHub.h"

namespace sv
{

namespace
{
bool isValidSlot (int slotIndex) noexcept
{
    return slotIndex >= 0 && slotIndex < kMaxSources;
}
} // namespace

std::optional<int> SoundVisionHub::registerSource (uint32_t preferredId)
{
    const std::lock_guard lock (mutex);

    for (int i = 0; i < kMaxSources; ++i)
    {
        if (! slots[(size_t) i].occupied.load (std::memory_order_acquire))
        {
            auto& slot = slots[(size_t) i];
            slot.snapshot = {};
            slot.snapshot.sourceId = preferredId != 0 ? preferredId
                                                       : (uint32_t) (i + 1);
            slot.snapshot.active = true;
            slot.occupied.store (true, std::memory_order_release);
            slot.generation.fetch_add (1, std::memory_order_acq_rel);
            return i;
        }
    }

    return std::nullopt;
}

void SoundVisionHub::unregisterSource (int slotIndex)
{
    if (! isValidSlot (slotIndex))
        return;

    const std::lock_guard lock (mutex);
    auto& slot = slots[(size_t) slotIndex];
    slot.snapshot = {};
    slot.occupied.store (false, std::memory_order_release);
    slot.generation.fetch_add (1, std::memory_order_acq_rel);
}

void SoundVisionHub::publish (int slotIndex, const SourceSnapshot& snapshot)
{
    if (! isValidSlot (slotIndex))
        return;

    const std::lock_guard lock (mutex);
    auto& slot = slots[(size_t) slotIndex];

    if (! slot.occupied.load (std::memory_order_relaxed))
        return;

    slot.snapshot = snapshot;
    slot.snapshot.active = true;
    slot.generation.fetch_add (1, std::memory_order_relaxed);
}

std::vector<SourceSnapshot> SoundVisionHub::collectActiveSources() const
{
    const std::lock_guard lock (mutex);
    std::vector<SourceSnapshot> result;
    result.reserve ((size_t) kMaxSources);

    for (const auto& slot : slots)
    {
        if (slot.occupied.load (std::memory_order_relaxed) && slot.snapshot.active)
            result.push_back (slot.snapshot);
    }

    return result;
}

int SoundVisionHub::getOccupiedCount() const
{
    const std::lock_guard lock (mutex);
    int count = 0;

    for (const auto& slot : slots)
        if (slot.occupied.load (std::memory_order_relaxed))
            ++count;

    return count;
}

} // namespace sv
