#pragma once
#include "IMemoryAllocator.h"
#include <cstddef>
#include <cstdint>
#include <mutex>
#include <optional>
#include <string>
#include <vector>

// One contiguous run of memory: either a free hole or a process's allocation.
struct MemoryBlock {
    std::uint64_t start;
    std::uint64_t size;
    bool          free;
    std::string   owner;
};

// Flat first-fit memory allocator (lecture: "flat memory model" + first-fit placement).
// The whole address space [0, totalSize) starts as one free block; allocate() scans blocks
// in address order and takes the first one large enough, splitting off the remainder as a
// new free block. deallocate() frees a process's block and coalesces it with any adjacent
// free neighbours so later allocations see the largest possible holes.
class FlatMemoryAllocator : public IMemoryAllocator {
public:
    explicit FlatMemoryAllocator(std::uint64_t totalSize);

    // Returns the base address on success, or nullopt if no free block is large enough.
    std::optional<std::uint64_t> allocate(std::uint64_t size, const std::string& owner) override;
    void deallocate(const std::string& owner) override;

    // Secures a block via allocate() and binds proc's flat (non-paged) address space.
    bool admit(Process& proc, std::uint64_t size) override;
    // Flat processes are always fully resident, so this is never actually reached.
    bool handleFault(Process& proc, std::uint64_t vpage) override;
    bool isDemandPaged() const override;

    int           getProcessCount()          const override;
    std::uint64_t getExternalFragmentation() const override; // sum of all free block sizes

    // Writes memory_stamp_<quantumIndex>.txt into outDir with the timestamp, process count,
    // external fragmentation, and an ASCII map of occupied blocks from high to low address.
    void writeSnapshot(int quantumIndex, const std::string& outDir) const override;

    // Flat-model answers for the demand-paging bookkeeping hooks: no paging ever happens here.
    std::uint64_t usedBytes()  const override;
    std::uint64_t freeBytes()  const override;
    std::uint64_t totalBytes() const override;
    std::uint64_t pagedIn()    const override;
    std::uint64_t pagedOut()   const override;

private:
    std::uint64_t totalSize;
    std::vector<MemoryBlock> blocks; // sorted by start address, contiguous coverage of [0,totalSize)
    mutable std::mutex mtx;
};
