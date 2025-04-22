#pragma once

#include <cassert>
#include <cmath>
#include <cstdint>
#include <vector>

namespace sc {

inline size_t Idx32(size_t num) { return num >> 5; }

// Fast set union and intersection using bitwise operations.
// Map each node in the CFG to a number between 0 - |N| - 1,
// where |N| is the number of nodes in the CFG.
// Now, a set of nodes can be represented using a bitset,
// where 1 => element is present and 0 => not present.
class IndexSet {
  public:
    IndexSet(size_t sz, bool all_set = false)
        : size(sz), sets(Idx32(sz) + 1, (all_set ? 0xFFFFFFFF : 0)) {}

    void Set(size_t idx) {
        assert(idx < size);
        auto i = Idx32(idx);
        sets[i] |= 1U << (idx - 32 * i);
    }

    void Reset(size_t idx) {
        assert(idx < size);
        auto i = Idx32(idx);
        sets[i] &= ~(1U << (idx - 32 * i));
    }

    bool Get(size_t idx) const {
        assert(idx < size);
        auto i = Idx32(idx);
        return sets[i] & (1U << (idx - 32 * i));
    }

    size_t GetSize() const { return size; }

    const std::vector<uint32_t> &GetData() const { return sets; }

  private:
    size_t size;
    std::vector<uint32_t> sets;

    // Union
    friend IndexSet operator|(const IndexSet &lhs, const IndexSet &rhs);
    // Intersection
    friend IndexSet operator&(const IndexSet &lhs, const IndexSet &rhs);

    friend bool operator==(const IndexSet &lhs, const IndexSet &rhs);
    friend bool operator!=(const IndexSet &lhs, const IndexSet &rhs);
};
} // namespace sc
