#pragma once
#include "IMemoryAllocator.h"
#include <atomic>
#include <cstdint>
#include <deque>
#include <map>
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>

// Demand-paging allocator (MO2). Physical memory is a fixed set of frames; a process's virtual
// pages are only backed by a frame once faulted in (see Process::bindMemory demandPaged=true).
// Eviction policy is FIFO over load order; evicted pages are written to an in-memory `store` that
// mirrors the on-disk csopesy-backing-store.txt for human inspection.
class PagingAllocator : public IMemoryAllocator {
public:
    PagingAllocator(std::uint64_t totalBytes, std::uint64_t frameBytes);

    // Placeholder synonym for admit() bookkeeping when no Process& is available; base address is
    // meaningless under paging.
    std::optional<std::uint64_t> allocate(std::uint64_t size, const std::string& owner) override;
    void deallocate(const std::string& owner) override;

    bool admit(Process& proc, std::uint64_t size) override;
    bool handleFault(Process& proc, std::uint64_t vpage) override;
    bool isDemandPaged() const override;

    int           getProcessCount()          const override;
    std::uint64_t getExternalFragmentation() const override; // paging has no contiguous holes

    void writeSnapshot(int quantumIndex, const std::string& outDir) const override;

    std::uint64_t usedBytes()  const override;
    std::uint64_t freeBytes()  const override;
    std::uint64_t totalBytes() const override;
    std::uint64_t pagedIn()    const override;
    std::uint64_t pagedOut()   const override;

private:
    struct Frame {
        bool          free    = true;
        Process*      owner   = nullptr;
        std::string   ownerName;
        std::uint64_t vpage   = 0;
    };

    // Rewrites csopesy-backing-store.txt from scratch. Must be called with mtx held.
    void writeBackingStoreFile() const;

    std::uint64_t totalBytesVal;
    std::uint64_t frameBytes;

    std::vector<Frame>          frames;    // size = totalBytes / frameBytes
    std::deque<std::size_t>     loadOrder; // FIFO victim order: frame indices, front = oldest
    std::map<std::pair<std::string, std::uint64_t>, std::vector<std::uint8_t>> store; // (owner,page) -> bytes
    std::unordered_map<std::string, std::uint64_t> admitted; // owner name -> size in bytes

    std::atomic<std::uint64_t> numPagedIn{0};
    std::atomic<std::uint64_t> numPagedOut{0};

    mutable std::mutex mtx;
};
