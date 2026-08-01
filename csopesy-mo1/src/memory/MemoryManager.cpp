#include "MemoryManager.h"
#include "Process.h"
#include <algorithm>
#include <ctime>
#include <filesystem>
#include <fstream>

MemoryManager::MemoryManager(std::uint64_t totalSize) : totalSize(totalSize) {
    blocks.push_back({0, totalSize, true, ""});
}

std::optional<std::uint64_t> MemoryManager::allocate(std::uint64_t size, const std::string& owner) {
    std::lock_guard<std::mutex> lock(mtx);
    for (std::size_t i = 0; i < blocks.size(); ++i) {
        if (!blocks[i].free || blocks[i].size < size) continue;

        std::uint64_t start = blocks[i].start;
        if (blocks[i].size == size) {
            blocks[i].free = false;
            blocks[i].owner = owner;
        } else {
            MemoryBlock allocated{start, size, false, owner};
            blocks[i].start += size;
            blocks[i].size  -= size;
            blocks.insert(blocks.begin() + i, allocated);
        }
        return start;
    }
    return std::nullopt;
}

void MemoryManager::deallocate(const std::string& owner) {
    std::lock_guard<std::mutex> lock(mtx);
    for (auto& b : blocks) {
        if (!b.free && b.owner == owner) {
            b.free = true;
            b.owner.clear();
            break;
        }
    }

    // Coalesce adjacent free blocks so future allocations see the largest possible holes.
    for (std::size_t i = 0; i + 1 < blocks.size();) {
        if (blocks[i].free && blocks[i + 1].free) {
            blocks[i].size += blocks[i + 1].size;
            blocks.erase(blocks.begin() + i + 1);
        } else {
            ++i;
        }
    }
}

bool MemoryManager::admit(Process& proc, std::uint64_t size) {
    auto base = allocate(size, proc.getName());
    if (!base) return false;
    proc.setMemory(*base, size);
    proc.bindMemory(size, size, /*demandPaged=*/false);
    return true;
}

bool MemoryManager::handleFault(Process&, std::uint64_t) {
    return true;
}

bool MemoryManager::isDemandPaged() const {
    return false;
}

int MemoryManager::getProcessCount() const {
    std::lock_guard<std::mutex> lock(mtx);
    int count = 0;
    for (auto& b : blocks) if (!b.free) ++count;
    return count;
}

std::uint64_t MemoryManager::getExternalFragmentation() const {
    std::lock_guard<std::mutex> lock(mtx);
    std::uint64_t frag = 0;
    for (auto& b : blocks) if (b.free) frag += b.size;
    return frag;
}

void MemoryManager::writeSnapshot(int quantumIndex, const std::string& outDir) const {
    std::vector<MemoryBlock> occupied;
    std::uint64_t frag = 0;
    {
        std::lock_guard<std::mutex> lock(mtx);
        for (auto& b : blocks) {
            if (b.free) frag += b.size;
            else        occupied.push_back(b);
        }
    }

    // Highest address first, matching the top-down "----end----" to "----start-----" layout.
    std::sort(occupied.begin(), occupied.end(),
              [](const MemoryBlock& a, const MemoryBlock& b) { return a.start > b.start; });

    std::filesystem::create_directories(outDir);
    std::ofstream out(outDir + std::string("/memory_stamp_") + std::to_string(quantumIndex) + ".txt");

    std::time_t t = std::time(nullptr);
    char buf[32];
    std::strftime(buf, sizeof(buf), "(%m/%d/%Y %I:%M:%S%p)", std::localtime(&t));

    out << "Timestamp: " << buf << "\n";
    out << "Number of processes in memory: " << occupied.size() << "\n";
    out << "Total external fragmentation in KB: " << frag << "\n\n";

    out << "----end---- = " << totalSize << "\n\n";
    for (auto& b : occupied) {
        out << (b.start + b.size) << "\n";
        out << b.owner << "\n";
        out << b.start << "\n\n";
    }
    out << "----start----- = 0\n";
}

std::uint64_t MemoryManager::usedBytes() const {
    return totalSize - getExternalFragmentation();
}

std::uint64_t MemoryManager::freeBytes() const {
    return getExternalFragmentation();
}

std::uint64_t MemoryManager::totalBytes() const {
    return totalSize;
}

std::uint64_t MemoryManager::pagedIn() const {
    return 0;
}

std::uint64_t MemoryManager::pagedOut() const {
    return 0;
}
