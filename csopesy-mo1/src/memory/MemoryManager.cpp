#include "MemoryManager.h"
#include "FlatMemoryAllocator.h"
#include "PagingAllocator.h"

MemoryManager::MemoryManager(bool demandPaged, std::uint64_t totalBytes, std::uint64_t frameBytes) {
    if (demandPaged)
        strategy = std::make_unique<PagingAllocator>(totalBytes, frameBytes);
    else
        strategy = std::make_unique<FlatMemoryAllocator>(totalBytes);
}

std::optional<std::uint64_t> MemoryManager::allocate(std::uint64_t size, const std::string& owner) {
    return strategy->allocate(size, owner);
}

void MemoryManager::deallocate(const std::string& owner) {
    strategy->deallocate(owner);
}

bool MemoryManager::admit(Process& proc, std::uint64_t size) {
    return strategy->admit(proc, size);
}

bool MemoryManager::handleFault(Process& proc, std::uint64_t vpage) {
    return strategy->handleFault(proc, vpage);
}

bool MemoryManager::isDemandPaged() const {
    return strategy->isDemandPaged();
}

int MemoryManager::getProcessCount() const {
    return strategy->getProcessCount();
}

std::uint64_t MemoryManager::getExternalFragmentation() const {
    return strategy->getExternalFragmentation();
}

void MemoryManager::writeSnapshot(int quantumIndex, const std::string& outDir) const {
    strategy->writeSnapshot(quantumIndex, outDir);
}

std::uint64_t MemoryManager::usedBytes() const {
    return strategy->usedBytes();
}

std::uint64_t MemoryManager::freeBytes() const {
    return strategy->freeBytes();
}

std::uint64_t MemoryManager::totalBytes() const {
    return strategy->totalBytes();
}

std::uint64_t MemoryManager::pagedIn() const {
    return strategy->pagedIn();
}

std::uint64_t MemoryManager::pagedOut() const {
    return strategy->pagedOut();
}
