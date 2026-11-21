#pragma once
#include <concepts>
#include <cstddef>
#include <limits>
#include <queue>
#include <vector>
#include "circuit_escape/pathfinding/search_result.hpp"

// Lo mínimo que BFS necesita de un grafo: cantidad de casillas, si una casilla es transitable
// y sus vecinos en un orden fijo. MapGraph (graph/graph.hpp) cumple este contrato.
template <typename Graph>
concept SearchGraph = requires(const Graph& graph, std::size_t cell) {
    { graph.size() } -> std::convertible_to<std::size_t>;
    { graph.isOpen(cell) } -> std::convertible_to<bool>;
    { graph.neighbors(cell) } -> std::convertible_to<const std::vector<std::size_t>&>;
};

// --- BFS PROPIO (proyecto 2, secciones 3.1 y 5) ---
// Encuentra un camino con la MENOR cantidad de movimientos (todos los pasos valen uno).
// Se marca cada casilla al encolarla, así ninguna entra dos veces a la cola.
// Desempate fijo: los vecinos se revisan en el orden que entrega el grafo
// (MapGraph: derecha, arriba, abajo, izquierda), por eso el resultado es reproducible.
// Tiempo O(V + E), espacio O(V).
template <SearchGraph Graph>
[[nodiscard]] SearchResult bfs(const Graph& graph, std::size_t source, std::size_t target)
{
    SearchResult result{};
    const std::size_t total = graph.size();
    if (source >= total || target >= total || !graph.isOpen(source) || !graph.isOpen(target)) {
        return result; // origen o destino inválidos: no hay camino
    }

    constexpr std::size_t none = std::numeric_limits<std::size_t>::max();
    std::vector<std::size_t> previous(total, none); // casilla anterior: permite reconstruir el camino
    std::vector<bool> discovered(total, false);
    std::queue<std::size_t> pending;

    pending.push(source);
    discovered[source] = true;

    while (!pending.empty()) {
        const std::size_t current = pending.front();
        pending.pop();
        result.examined.push_back(current);
        if (current == target) {
            break; // la primera vez que sale el destino ya tiene la menor cantidad de movimientos
        }
        for (const std::size_t next : graph.neighbors(current)) {
            if (!discovered[next]) {
                discovered[next] = true;
                previous[next] = current;
                pending.push(next);
            }
        }
    }

    if (!discovered[target]) {
        return result; // destino inalcanzable
    }

    // Reconstrucción: desde el destino, siguiendo las casillas anteriores hasta el origen
    for (std::size_t cell = target; cell != none; cell = previous[cell]) {
        result.path.push_back(cell);
    }
    std::vector<std::size_t> forward(result.path.rbegin(), result.path.rend());
    result.path = std::move(forward);
    result.found = true;
    result.cost = static_cast<int>(result.path.size()) - 1;
    return result;
}
