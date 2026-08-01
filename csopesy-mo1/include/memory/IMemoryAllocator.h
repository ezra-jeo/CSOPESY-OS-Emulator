#pragma once
#include <cstdint>
#include <optional>
#include <string>

// Abstract memory allocator interface shared by MemoryManager (flat first-fit, MO1) and the
// demand-paging allocator introduced in a later step. SchedulerBase/Console hold an
// IMemoryAllocator& / unique_ptr<IMemoryAllocator> so the scheduling and console code stays
// unaware of which concrete allocation strategy is in play.
class IMemoryAllocator {
public:
    virtual ~IMemoryAllocator() = default;

    // Returns the base address on success, or nullopt if the request cannot be satisfied.
    virtual std::optional<std::uint64_t> allocate(std::uint64_t size, const std::string& owner) = 0;
    virtual void deallocate(const std::string& owner) = 0;

    virtual int           getProcessCount()          const = 0;
    virtual std::uint64_t getExternalFragmentation() const = 0; // sum of all free block sizes

    // Writes memory_stamp_<quantumIndex>.txt into outDir (see MemoryManager for the flat-model
    // format demand paging must also honour).
    virtual void writeSnapshot(int quantumIndex, const std::string& outDir) const = 0;

    // Demand-paging bookkeeping hooks. The flat first-fit allocator answers these from its
    // existing block list; a paging allocator answers them from its frame table.
    virtual std::uint64_t usedBytes()  const = 0;
    virtual std::uint64_t freeBytes()  const = 0;
    virtual std::uint64_t totalBytes() const = 0;
    virtual std::uint64_t pagedIn()    const = 0; // flat model: always 0, no paging occurs
    virtual std::uint64_t pagedOut()   const = 0; // flat model: always 0, no paging occurs
};
