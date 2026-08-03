#pragma once
#include "IMemoryAllocator.h"
#include <cstdint>
#include <memory>
#include <optional>
#include <string>

// Facade over the two memory-allocation strategies (FlatMemoryAllocator, PagingAllocator).
// Picks one at construction time and forwards every IMemoryAllocator call to it, so callers
// (Console, SchedulerBase, CPUWorker, ...) depend only on IMemoryAllocator and never need to know
// which concrete strategy is active.
class MemoryManager : public IMemoryAllocator {
public:
    // demandPaged selects PagingAllocator(totalBytes, frameBytes) vs. FlatMemoryAllocator(totalBytes).
    MemoryManager(bool demandPaged, std::uint64_t totalBytes, std::uint64_t frameBytes);

    std::optional<std::uint64_t> allocate(std::uint64_t size, const std::string& owner) override;
    void deallocate(const std::string& owner) override;

    bool admit(Process& proc, std::uint64_t size) override;
    bool handleFault(Process& proc, std::uint64_t vpage) override;
    bool isDemandPaged() const override;

    int           getProcessCount()          const override;
    std::uint64_t getExternalFragmentation() const override;

    void writeSnapshot(int quantumIndex, const std::string& outDir) const override;

    std::uint64_t usedBytes()  const override;
    std::uint64_t freeBytes()  const override;
    std::uint64_t totalBytes() const override;
    std::uint64_t pagedIn()    const override;
    std::uint64_t pagedOut()   const override;

private:
    std::unique_ptr<IMemoryAllocator> strategy;
};
