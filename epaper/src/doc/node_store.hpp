
#pragma once
#include "node.hpp"
#include <atomic>
#include <cstddef>
#include <cstdint>
#include <sys/types.h>
#include <vector>
#include <unordered_map>
#include <cstdio>
#include <cstring>

class SlotTable;
class Slot;




enum class SlotState { Free, Live, Retired };


/**
 * fixed size; fixed address for the life of the store
 */
struct Slot {
    // How many times this slot has been freed
    std::atomic<uint32_t> gen{0};

    // Free | Live | Retired
    std::atomic<SlotState> state{SlotState::Free};
    
    // The node itself, constructed in place
    NodeStorage node;
};

struct Retired {
    uint32_t slot;
    uint64_t epoch;
};

const uint32_t kChunkSize = 1024;

/**
 * Chunk = 1024 slots = [0, 1, 2 ... 1023]
 * Chunk* points to that 1-d array
 * Chunk** points to list of Chunk*, each of those is a separate 1-d block.
 */

struct Chunk {
    Slot slots[kChunkSize];
};


class SlotTable {

private:
    /**
     * "Chunk*" is pointer to 1-d array of 1024 slots
     * "Chunk**" is what we needed here. Growing the directory by adding new Chunk*
     */
    std::atomic<Chunk**> directory{nullptr};
    std::atomic<uint32_t> chunkCount = 0;

    // Nothing synchronize on chunkCapacity, no atomic needed like chunkCount
    uint32_t chunkCapacity;

    // No atomic needed here
    uint32_t nextUnused = 0;
    std::vector<uint32_t> freeList;
    std::vector<Retired> retired;
    

    // When we grow the directory, old directories are collected here
    std::vector<Chunk**> retiredDirectories;

private:
    void grow() {
        uint32_t n = chunkCount.load(std::memory_order_acquire);
        Chunk** dir = directory.load(std::memory_order_acquire);
        
        if (n >= chunkCapacity) {
            // Ensure "directory" capacity

            uint32_t newCapacity = chunkCapacity * 2;
            Chunk** newDir = new Chunk*[newCapacity]();

            std::memcpy(newDir, dir, n * sizeof(Chunk*));

            chunkCapacity = newCapacity;
            directory.store(newDir, std::memory_order_release);

            retiredDirectories.push_back(dir);  // pointer array only; chunks stay live
            dir = newDir;
        }

        dir[n] = new Chunk;
        chunkCount.store(n + 1, std::memory_order_release);
    }

    Slot& at(uint32_t slotIndex)
    {
        Chunk** dir = directory.load(std::memory_order_relaxed); // writer
        return dir[slotIndex / kChunkSize]->slots[slotIndex % kChunkSize];
    }

    bool ready(Retired r) {
        // Step 5 replaces it with the epoch check.
        return true;
    }
   
public:

    explicit SlotTable(int initialChunkCapacity) {
        Chunk** dir = new Chunk*[initialChunkCapacity]();
        dir[0] = new Chunk;

        chunkCapacity = initialChunkCapacity;
        directory.store(dir, std::memory_order_release);
        chunkCount.store(1, std::memory_order_release);
    }

    // SlotTable owns raw pointers and is not copyable 
    SlotTable(const SlotTable&) = delete;

    // SlotTable owns raw pointers and is not copyable 
    SlotTable& operator=(const SlotTable&) = delete;

    ~SlotTable()
    {
        Chunk** dir = directory.load(std::memory_order_relaxed);
        const uint32_t n = chunkCount.load(std::memory_order_relaxed);

        if (dir) {
            for (uint32_t i = 0; i < n; ++i) {
                delete dir[i];   // real Slot blocks — once
            }
            delete[] dir;
        }

        for (Chunk** old : retiredDirectories) {
            delete[] old;        // old Chunk* arrays only — do not delete Chunks again
        }
        retiredDirectories.clear();
    }


    NodeStorage* resolve(Handle h) {
        // atomic, not uint32_t
        const uint32_t n = chunkCount.load(std::memory_order_acquire);

        if (h.slot >= n * kChunkSize) {
            std::fprintf(stderr, "[doc] resolve: slot %u out of range (chunks=%u, gen=%u)\n", h.slot, n, h.gen);
            return nullptr;
        }
        

        // directory is an std::atomic<Chunk**>, so it is not an array you can index.
        Chunk** dir = directory.load(std::memory_order_acquire);
        Chunk* chunk = dir[h.slot / kChunkSize];

        // Why "Slot&" ? We cannot copy the slot to heap, and have to leave the slot in its place.
        // Copying would be a different struct in memory, with temporary-different s.node too, 
        // all living in stack frame, dangerous.
        Slot& s = chunk->slots[h.slot % kChunkSize];

        // Same here, load data from atomic pointer by "memory_order_acquire"
        if (s.gen.load(std::memory_order_acquire) != h.gen) return nullptr;                 // Gone
        if (s.state.load(std::memory_order_acquire) != SlotState::Live) return nullptr;     // Gone

        // Safely refer to s.node in the directory, not something temporarily lives in stack frame.
        return &s.node;
    }

    /**
     * Pick a free slot to use. "NodeStore" then use the handle and store node data into the slot.
     * No race-condition protection here.
     */
    Handle claim() {
        uint32_t s;
        if (!freeList.empty()) {
            s = freeList.back();
            freeList.pop_back();
        } else {
            s = nextUnused++;
            if (s == chunkCount.load(std::memory_order_relaxed) * kChunkSize) {
                grow();
            }
        }

        Slot& slot = at(s);
        const uint32_t g = slot.gen.load(std::memory_order_relaxed);

        // Don't change slot.state here, "NodeStore" will do that later
        return Handle{s, g};
    }

    void free() {
        std::vector<Retired> keep;

        for (const Retired& r: retired) {

            if (!ready(r)) {
                keep.push_back(r);
                continue;
            }

            Slot& slot = at(r.slot);
            NodeStorage& node = slot.node;

            const NodeType type = node.type.load(std::memory_order_relaxed);
            destroyPayload(type, node.payload.load(std::memory_order_relaxed));
            node.payload.store(nullptr, std::memory_order_relaxed);

            const Children* c = node.children.load(std::memory_order_relaxed);
            if (c) {
                delete c;
                node.children.store(nullptr, std::memory_order_relaxed);
            }

            const uint32_t g = slot.gen.load(std::memory_order_relaxed);
            if (g == UINT32_MAX) {
                slot.state.store(SlotState::Free, std::memory_order_release);
            } else {
                slot.gen.store(g + 1, std::memory_order_release);
                slot.state.store(SlotState::Free, std::memory_order_release);
                freeList.push_back(r.slot);
            }

        }

        retired = keep;
    }

};

// Forward declaration is not enough
class NodeStore {

public:

private:
    SlotTable slots = SlotTable(4);
    std::unordered_map<NodeId, Handle> ids{};
    uint64_t epoches = 0;
    std::unordered_map<NodeId, std::vector<NodeId>> dependents{};
};