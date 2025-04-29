#include "index_set.hpp"
#include <algorithm>
#include <ranges>

namespace sc {
IndexSet operator|(const IndexSet &lhs, const IndexSet &rhs) {
    auto min = std::min(lhs.sets.size(), rhs.sets.size());
    auto size = std::max(lhs.size, rhs.size);
    auto rset = IndexSet(size);

    for (auto i : std::views::iota(0UL, min)) {
        rset.sets[i] = lhs.sets[i] | rhs.sets[i];
    }

    for (auto i : std::views::iota(min, lhs.sets.size())) {
        rset.sets[i] = lhs.sets[i];
    }

    for (auto i : std::views::iota(min, rhs.sets.size())) {
        rset.sets[i] = rhs.sets[i];
    }

    return rset;
}

IndexSet operator&(const IndexSet &lhs, const IndexSet &rhs) {
    auto min = std::min(lhs.sets.size(), rhs.sets.size());
    auto size = std::min(lhs.size, rhs.size);
    auto rset = IndexSet(size);

    for (auto i : std::views::iota(0UL, min)) {
        rset.sets[i] = lhs.sets[i] & rhs.sets[i];
    }

    return rset;
}

bool operator==(const IndexSet &lhs, const IndexSet &rhs) {
    return lhs.sets == rhs.sets;
}

bool operator!=(const IndexSet &lhs, const IndexSet &rhs) {
    return !(lhs == rhs);
}

std::vector<size_t> IndexSet::GetBlocks() const {
    std::vector<size_t> indexes;
    for (auto i : std::views::iota(0UL, sets.size())) {
        auto bits = sets[i];
        uint k = 0;
        while (bits) {
            if (bits & (1 << k)) {
                indexes.push_back(i * 32 + k);
                bits &= ~(1 << k);
            }
            ++k;
        }
    }
    return indexes;
}

} // namespace sc
