#include "PagingAllocator.h"
#include "Process.h"
#include <algorithm>
#include <fstream>
#include <iomanip>

PagingAllocator::PagingAllocator(std::uint64_t totalBytes, std::uint64_t frameBytes)
    : totalBytesVal(totalBytes), frameBytes(frameBytes) {
    std::uint64_t numFrames = frameBytes ? (totalBytes / frameBytes) : 0;
    frames.resize(numFrames);

    // Create csopesy-backing-store.txt immediately so it's present/readable for the whole
    // emulator run, even before any page fault has occurred to populate it.
    std::lock_guard<std::mutex> lock(mtx);
    writeBackingStoreFile();
}

std::optional<std::uint64_t> PagingAllocator::allocate(std::uint64_t size, const std::string& owner) {
    std::lock_guard<std::mutex> lock(mtx);
    admitted[owner] = size;
    return std::optional<std::uint64_t>(0);
}

void PagingAllocator::deallocate(const std::string& owner) {
    std::lock_guard<std::mutex> lock(mtx);
    for (std::size_t i = 0; i < frames.size(); ++i) {
        if (!frames[i].free && frames[i].ownerName == owner) {
            frames[i].free = true;
            frames[i].owner = nullptr;
            frames[i].ownerName.clear();
            loadOrder.erase(std::remove(loadOrder.begin(), loadOrder.end(), i), loadOrder.end());
        }
    }

    bool storeChanged = false;
    for (auto it = store.begin(); it != store.end();) {
        if (it->first.first == owner) {
            it = store.erase(it);
            storeChanged = true;
        } else {
            ++it;
        }
    }

    admitted.erase(owner);

    if (storeChanged) writeBackingStoreFile();
}

bool PagingAllocator::admit(Process& proc, std::uint64_t size) {
    std::lock_guard<std::mutex> lock(mtx);
    admitted[proc.getName()] = size;
    proc.setMemory(0, size);
    proc.bindMemory(size, frameBytes, /*demandPaged=*/true);
    return true;
}

bool PagingAllocator::handleFault(Process& proc, std::uint64_t vpage) {
    std::lock_guard<std::mutex> lock(mtx);

    if (proc.isPageResident(vpage)) return true;

    std::size_t freeIdx = frames.size(); // sentinel: none found yet
    for (std::size_t i = 0; i < frames.size(); ++i) {
        if (frames[i].free) { freeIdx = i; break; }
    }

    if (freeIdx == frames.size()) {
        std::size_t victimIdx = loadOrder.front();
        loadOrder.pop_front();
        Frame& f = frames[victimIdx];
        if (f.owner) {
            auto bytes = f.owner->extractPageBytes(f.vpage);
            store[{f.ownerName, f.vpage}] = std::move(bytes);
            f.owner->invalidatePage(f.vpage);
            ++numPagedOut;
        }
        f.free = true;
        f.owner = nullptr;
        f.ownerName.clear();
        freeIdx = victimIdx;
    }

    std::vector<std::uint8_t> bytes;
    bool eraseStoreEntry = false;
    auto key = std::make_pair(proc.getName(), vpage);
    auto it = store.find(key);
    if (it != store.end()) {
        bytes = it->second;
        eraseStoreEntry = true;
    } else {
        bytes.assign(frameBytes, 0);
    }

    proc.installPageBytes(vpage, bytes);

    Frame& f = frames[freeIdx];
    f.free = false;
    f.owner = &proc;
    f.ownerName = proc.getName();
    f.vpage = vpage;
    loadOrder.push_back(freeIdx);
    ++numPagedIn;

    if (eraseStoreEntry) store.erase(it);
    writeBackingStoreFile();

    return true;
}

bool PagingAllocator::isDemandPaged() const {
    return true;
}

int PagingAllocator::getProcessCount() const {
    std::lock_guard<std::mutex> lock(mtx);
    return static_cast<int>(admitted.size());
}

std::uint64_t PagingAllocator::getExternalFragmentation() const {
    return 0; // paging has no contiguous free-block holes to fragment
}

void PagingAllocator::writeSnapshot(int, const std::string&) const {
    // Demand paging exposes live state via process-smi/vmstat (a later step) instead of the flat
    // model's per-quantum memory_stamp_*.txt files.
}

std::uint64_t PagingAllocator::usedBytes() const {
    std::lock_guard<std::mutex> lock(mtx);
    std::uint64_t used = 0;
    for (auto& f : frames) if (!f.free) used += frameBytes;
    return used;
}

std::uint64_t PagingAllocator::freeBytes() const {
    return totalBytes() - usedBytes();
}

std::uint64_t PagingAllocator::totalBytes() const {
    return totalBytesVal;
}

std::uint64_t PagingAllocator::pagedIn() const {
    return numPagedIn.load();
}

std::uint64_t PagingAllocator::pagedOut() const {
    return numPagedOut.load();
}

void PagingAllocator::writeBackingStoreFile() const {
    std::ofstream out("csopesy-backing-store.txt", std::ios::trunc);
    out << "CSOPESY demand-paging backing store (owner, page, little-endian uint16 words)\n";
    out << "entries: " << store.size() << "\n\n";
    for (auto& [key, bytes] : store) {
        out << "owner=" << key.first << " page=" << key.second
            << " bytes=" << bytes.size() << "\n";
        for (std::size_t addr = 0; addr < bytes.size(); addr += 2) {
            std::uint16_t word;
            if (addr + 1 < bytes.size())
                word = static_cast<std::uint16_t>(bytes[addr] | (bytes[addr + 1] << 8));
            else
                word = bytes[addr];
            out << "  [" << addr << "] " << word << "\n";
        }
        out << "\n";
    }
}
