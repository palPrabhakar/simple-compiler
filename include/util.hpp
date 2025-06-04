#pragma once

#include <algorithm>
#include <vector>

template <typename T>
void RemoveElements(std::vector<T> &vec, std::vector<size_t> indexes) {
    std::sort(indexes.begin(), indexes.end());

    size_t cur = 0; // position to copy to
    size_t idx = 0; // pos in indexes

    for (size_t i = 0; i < vec.size(); ++i) {
        if (idx < indexes.size() && i == indexes[idx]) {
            ++idx; // skip this element
        } else {
            vec[cur++] = std::move(vec[i]);
        }
    }
    vec.erase(vec.begin() + static_cast<long>(cur), vec.end());
}
