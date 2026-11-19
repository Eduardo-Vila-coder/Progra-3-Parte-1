#include "circuit_escape/graph/graph.hpp"

DisjointSet::DisjointSet(std::size_t size) {
    for (std::size_t i = 0; i < size; i++) {
        parent_.push_back(i);
        size_.push_back(1);
    }
}

std::size_t DisjointSet::find(std::size_t cell) {
    if (cell == parent_[cell]) {
        return cell;
    }

    return find(parent_[cell]);
}

bool DisjointSet::unite(std::size_t a, std::size_t b) {
    auto root_a = find(a);
    auto root_b = find(b);

    if (root_a == root_b) {
        return false;
    }

    if (size_[find(a)] > size_[find(b)]) {
        parent_[root_b] = root_a;
        size_[root_a] += size_[root_b];     // root_a es la nueva root del set
    } else {
        parent_[root_a] = root_b;
        size_[root_b] += size_[root_a];     // root_b es la nueva root del set
    }

    return true;
}

bool DisjointSet::connected(std::size_t a, std::size_t b) {
    return find(a) == find(b);
}