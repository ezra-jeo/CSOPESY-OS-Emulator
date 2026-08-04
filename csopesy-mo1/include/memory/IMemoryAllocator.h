#pragma once
#include <cstdint>
#include <string>

class Process;

// Abstract memory allocator interface implemented by MemoryManager, the demand-paging memory
// manager (MO2). SchedulerBase/Console hold an IMemoryAllocator& / unique_ptr<IMemoryAllocator>
// so the scheduling and console code stays decoupled from the concrete allocator.
class IMemoryAllocator {
public:
    virtual ~IMemoryAllocator() = default;

    virtual void deallocate(const std::string& owner) = 0;

    // Prepares `proc` to run under demand paging: secures address space and calls
    // proc.setMemory(...)/proc.bindMemory(...). Demand paging never fails here — admitting a
    // process needs no physical frames up front.
    virtual bool admit(Process& proc, std::uint64_t size) = 0;

    // Services a page fault for proc's virtual page `vpage`. Brings the page into a free frame,
    // evicting a victim to the backing store first if none is free.
    virtual bool handleFault(Process& proc, std::uint64_t vpage) = 0;

    // Demand-paging bookkeeping hooks, answered from the frame table.
    virtual std::uint64_t usedBytes()  const = 0;
    virtual std::uint64_t freeBytes()  const = 0;
    virtual std::uint64_t totalBytes() const = 0;
    virtual std::uint64_t pagedIn()    const = 0;
    virtual std::uint64_t pagedOut()   const = 0;
};
