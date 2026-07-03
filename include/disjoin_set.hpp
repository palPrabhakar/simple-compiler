#pragma once

#include <unordered_map>
#include <utility>

template <typename T> class DisjointSet {
  public:
    void MakeSet(T t) {
        if (parent.contains(t)) {
            return;
        }

        parent.insert({t, t});
        depth.insert({t, 0});
    }

    void UnionSet(T a, T b) {
        a = FindSet(a);
        b = FindSet(b);

        if (a != b) {
            if (depth[a] < depth[b]) {
                std::swap(a, b);
            }

            parent[b] = a;

            if (depth[a] == depth[b])
                ++depth[a];
        }
    }

    template <bool make_new = true> T FindSet(T t) {
        if constexpr (make_new) {
            if (!parent.contains(t)) {
                MakeSet(t);
                return t;
            }
        }

        if (parent[t] == t) {
            return t;
        }

        parent[t] = FindSet<false>(t);
        return parent[t];
    }

  private:
    std::unordered_map<T, T> parent;
    std::unordered_map<T, int> depth;
};
