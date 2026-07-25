#include "index_set.hpp"
#include <algorithm>
#include <ranges>
#include <iostream>

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

    for (auto i : std::views::iota(0ul, min)) {
        rset.sets[i] = lhs.sets[i] & rhs.sets[i];
    }

    return rset;
}


IndexSet IndexSet::operator!() {
    auto rset = IndexSet(this->size);
    for (auto i : std::views::iota(0ul, this->sets.size())) {
        rset.sets[i] = ~this->sets[i];
    }
    return rset;
}

bool operator==(const IndexSet &lhs, const IndexSet &rhs) {
    return lhs.sets == rhs.sets;
}

bool operator!=(const IndexSet &lhs, const IndexSet &rhs) {
    return !(lhs == rhs);
}

std::vector<size_t> IndexSet::GetValues() const {
    std::vector<size_t> indexes;
    for (auto i : std::views::iota(0ul, sets.size())) {
        auto bits = sets[i];
        uint k = 0;
        while (bits) {
            if (bits & (1u << k)) {
                indexes.push_back(i * 32 + k);
                bits &= ~(1u << k);
            }
            ++k;
        }
    }
    return indexes;
}

} // namespace sc
