
#pragma once
#include "node.hpp"
#include <algorithm>
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
 * "std::atomic" has no copy-constructor, hence "Slot" has no copy-constructor.
 * When assigning to a "Slot" variable, use reference - no copy
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

    /** Internal access to slot, without resolving to "NodeStorage" */
    Slot& at(uint32_t slotIndex)
    {
        Chunk** dir = directory.load(std::memory_order_relaxed); // writer
        return dir[slotIndex / kChunkSize]->slots[slotIndex % kChunkSize];
    }

    /** Writer-only: push onto the private retired list. Does not change slot state. */
    void enqueueRetired(uint32_t slot, uint64_t epoch)
    {
        retired.push_back(Retired{slot, epoch});
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
    NodeStore() = default;

    ~NodeStore()
    {
        for (const RetiredChildren& r : retiredChildren)
            delete r.ptr;
        retiredChildren.clear();
    }

    NodeStore(const NodeStore&) = delete;
    NodeStore& operator=(const NodeStore&) = delete;

    Handle allocate(NodeId id, NodeType type)
    {
        Handle h = slots.claim();
        Slot& slot = slots.at(h.slot);
        NodeStorage& node = slot.node;

        // relaxed: same-thread initialization of one node. Each store is
        // sequenced-before the releases below, so "resolve"'s acquire of Live
        // makes every one of them visible together.
        //
        // Nothing acquire-loads these fields; they stay private until that handshake:
        //
        // Other cores are also allowed to observe those relaxed stores in any order, 
        // and even before Live is visible. That is acceptable because "resolve" does not 
        // read them until it has seen Live.
        node.id.store(id, std::memory_order_relaxed);
        node.type.store(type, std::memory_order_relaxed);
        node.parent.store(kNoParent, std::memory_order_relaxed);

        node.sx.store(1.0, std::memory_order_relaxed);
        node.sy.store(1.0, std::memory_order_relaxed);
        node.rotation.store(0.0, std::memory_order_relaxed);
        node.tx.store(0.0, std::memory_order_relaxed);
        node.ty.store(0.0, std::memory_order_relaxed);

        node.minX.store(0.0, std::memory_order_relaxed);
        node.minY.store(0.0, std::memory_order_relaxed);
        node.maxX.store(0.0, std::memory_order_relaxed);
        node.maxY.store(0.0, std::memory_order_relaxed);

        node.version.store(0, std::memory_order_relaxed);
        node.children.store(nullptr, std::memory_order_relaxed);

        // release: createPayload has finished constructing the heap object.
        // This store publishes that object through the pointer. An acquire
        // load of payload that sees this pointer also sees those constructor
        // writes. resolve does not take this handshake; it waits on state.
        //
        // A relaxed store of the pointer would publish the address alone; 
        // the reader could follow it into an object whose constructor writes were not yet visible.
        node.payload.store(createPayload(type), std::memory_order_release);

        // release: the door "resolve" waits on. 
        // 
        // A release synchronizes only with an acquire of the same atomic, 
        // and resolve acquire-loads state. 
        // 
        // Everything sequenced before this store — the relaxed fields, the
        // payload pointer, and the payload constructor writes — becomes
        // visible once the reader observes Live. Matching gen is still required.
        slot.state.store(SlotState::Live, std::memory_order_release);
        ids[id] = h;
        return h;
    }

    void retire(Handle h, bool reclaim = true)
    {
        Slot& slot = slots.at(h.slot);

        // Stale handle (outdated): slot already reused or never this occupancy
        if (slot.gen.load(std::memory_order_relaxed) != h.gen)
            return;

        // Only retire a Live node. 
        // - A Free node is to be used.
        // - A Retired node has nothing to do here.
        if (slot.state.load(std::memory_order_relaxed) != SlotState::Live)
            return;

        const Children* children = slot.node.children.load(std::memory_order_relaxed);
        if (children != nullptr) {
            for (const ChildLink& childLink : children->links) {
                retire(childLink.node, false);
            }
        }

        slot.state.store(SlotState::Retired, std::memory_order_release);

        const NodeId nodeId = slot.node.id.load(std::memory_order_relaxed);
        ids.erase(nodeId);

        // dependents → lastPose later (connector step)

        slots.enqueueRetired(h.slot, epoches);

        if (reclaim) {
            // Do not call free() on every recursive level
            // Because SlotTable::free already does the walk
            slots.free(); // immediate grace until step 5
        }
    }

    // link(P, h, orderKey, placement)
    bool link(Handle P, Handle h, uint64_t orderKey, Placement placement)
    {
        NodeStorage* nodeH = slots.resolve(h);
        NodeStorage* nodeP = slots.resolve(P);

        if (nodeP == nullptr || nodeH == nullptr)
            return false;

        const Children* oldChildren = nodeP->children.load(std::memory_order_relaxed);
        std::vector<ChildLink> newLinks =
            oldChildren ? oldChildren->links : std::vector<ChildLink>{};

        auto it = std::lower_bound(
            newLinks.begin(), newLinks.end(), orderKey,
            [](const ChildLink& a, uint64_t k) { return a.orderKey < k; });

        // memory_order: parent before any reader can reach h through the new list
        nodeH->parent.store(P, std::memory_order_relaxed);

        // Note: Allow duplicated child to avoid full links scan
        // Note: Allow duplicate orderKey (keys must be strictly increasing)

        newLinks.insert(it, ChildLink{h, orderKey, placement});

        Children* newChildren = new Children{std::move(newLinks)};
        // TODO: add index for h in r-tree (step 4)

        // memory_order: publish; readers can now reach h
        nodeP->children.store(newChildren, std::memory_order_release);

        // Retire old Children at the current epoch (step 5: wait before delete)
        if (oldChildren) {
            enqueueRetiredChildren(oldChildren);
        }
        reclaimChildren();

        // TODO: update P’s paint extent, and its entry upward (commit protocol)

        return true;
    }

    bool unlink(Handle P, Handle h) {
        NodeStorage* nodeP = slots.resolve(P);

        if (nodeP == nullptr) { return false; }

        const Children *oldChildren = nodeP->children.load(std::memory_order_relaxed);
        if (oldChildren == nullptr) { return false; }

        std::vector<ChildLink> newLinks{oldChildren->links};
        auto it = std::find_if(newLinks.begin(), newLinks.end(), [h](const ChildLink& c) {
            return h == c.node;
        });

        if (it == newLinks.end()) {
            return false;
        }

        newLinks.erase(it);
    

        // publish Children without h
        Children* newChildren = new Children{std::move(newLinks)};
        nodeP->children.store(newChildren, std::memory_order_release);

        // retire old Children
        enqueueRetiredChildren(oldChildren);
        reclaimChildren();

        // TODO: paint extent upward
        return true;
    }

private:
    struct RetiredChildren {
        const Children* ptr;
        uint64_t epoch;
    };

    /** Step 5 replaces this with e ≤ globalEpoch − 2. */
    bool childrenReady(uint64_t /*epoch*/) const { return true; }

    void enqueueRetiredChildren(const Children* c)
    {
        retiredChildren.push_back(RetiredChildren{c, epoches});
    }

    void reclaimChildren()
    {
        std::vector<RetiredChildren> keep;
        for (const RetiredChildren& r : retiredChildren) {
            if (!childrenReady(r.epoch)) {
                keep.push_back(r);
                continue;
            }
            delete r.ptr;
        }
        retiredChildren = std::move(keep);
    }

    SlotTable slots = SlotTable(4);
    std::unordered_map<NodeId, Handle> ids{};
    uint64_t epoches = 0;
    std::unordered_map<NodeId, std::vector<NodeId>> dependents{};
    std::vector<RetiredChildren> retiredChildren{};
};