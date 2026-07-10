#pragma once
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
class MemoryManager {
public:
    explicit MemoryManager(std::uint64_t totalSize);

    // Returns the base address on success, or nullopt if no free block is large enough.
    std::optional<std::uint64_t> allocate(std::uint64_t size, const std::string& owner);
    void deallocate(const std::string& owner);

    int           getProcessCount()          const;
    std::uint64_t getExternalFragmentation() const; // sum of all free block sizes

    // Writes memory_stamp_<quantumIndex>.txt into outDir with the timestamp, process count,
    // external fragmentation, and an ASCII map of occupied blocks from high to low address.
    void writeSnapshot(int quantumIndex, const std::string& outDir) const;

private:
    std::uint64_t totalSize;
    std::vector<MemoryBlock> blocks; // sorted by start address, contiguous coverage of [0,totalSize)
    mutable std::mutex mtx;
};
