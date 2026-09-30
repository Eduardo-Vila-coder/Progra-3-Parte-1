#pragma once
#include <utility>
#include <vector>

// Agrupa varias lambdas en un solo objeto con varios operator() (para std::visit)
template <typename... Callables>
struct Overloaded : Callables... {
    using Callables::operator()...;
};

template <typename... Callables>
Overloaded(Callables...) -> Overloaded<Callables...>;

// Agrega varios eventos al vector, en el mismo orden en que se pasan (fold expression)
template <typename T, typename... Values>
void appendEvents(std::vector<T>& destination, Values&&... values) {
    (destination.push_back(std::forward<Values>(values)), ...);
}
