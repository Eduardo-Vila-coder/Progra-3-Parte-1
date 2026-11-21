// Pruebas del BFS propio (proyecto 2, secciones 5 y 7)
#include <cassert>
#include <cstddef>
#include <iostream>
#include <vector>
#include "../../include/circuit_escape/graph/graph.hpp"
#include "../../include/circuit_escape/pathfinding/bfs.hpp"

using Graph = MapGraph<3, 5>;

std::size_t idx(std::size_t row, std::size_t column) { return toIndex({row, column}, 5); }

// Dos casillas consecutivas del camino deben ser vecinas en el grafo
bool isValidPath(const Graph& graph, const std::vector<std::size_t>& path) {
    for (std::size_t i = 1; i < path.size(); ++i) {
        const auto& next = graph.neighbors(path[i - 1]);
        bool adjacent = false;
        for (std::size_t n : next) adjacent = adjacent || n == path[i];
        if (!adjacent) return false;
    }
    return true;
}

int main() {
    const GameRules<> rules{};

    // 1. Tablero libre: la cantidad de movimientos es la distancia Manhattan
    {
        Grid<Cell, 3, 5> g{};
        const Graph graph{g, rules};
        const SearchResult r = bfs(graph, idx(0, 0), idx(2, 4));
        assert(r.found);
        assert(r.cost == 6);
        assert(r.path.front() == idx(0, 0) && r.path.back() == idx(2, 4));
        assert(r.path.size() == 7);
        assert(isValidPath(graph, r.path));
        assert(!r.examined.empty() && r.examined.front() == idx(0, 0));
    }

    // 2. Desempate fijo: con el orden de MapGraph (derecha, arriba, abajo, izquierda)
    //    de (0,0) a (1,1) primero va a la derecha y luego baja
    {
        Grid<Cell, 3, 5> g{};
        const Graph graph{g, rules};
        const SearchResult r = bfs(graph, idx(0, 0), idx(1, 1));
        assert((r.path == std::vector<std::size_t>{idx(0, 0), idx(0, 1), idx(1, 1)}));
        // Reproducible: la misma búsqueda da el mismo camino
        assert(bfs(graph, idx(0, 0), idx(1, 1)).path == r.path);
    }

    // 3. Respeta muros: rodea la pared y usa la menor cantidad de movimientos
    //    . # . . .
    //    . # . . .
    //    . . . . .
    {
        Grid<Cell, 3, 5> g{};
        g.at({0, 1}) = Wall{};
        g.at({1, 1}) = Wall{};
        const Graph graph{g, rules};
        const SearchResult r = bfs(graph, idx(0, 0), idx(0, 2));
        assert(r.found && r.cost == 6);   // baja 2, avanza 2, sube 2
        assert(isValidPath(graph, r.path));
        for (std::size_t cell : r.path) assert(graph.isOpen(cell));
    }

    // 4. Sin camino: un muro separa las dos mitades
    {
        Grid<Cell, 3, 5> g{};
        g.at({0, 2}) = Wall{};
        g.at({1, 2}) = Wall{};
        g.at({2, 2}) = Wall{};
        const Graph graph{g, rules};
        const SearchResult r = bfs(graph, idx(0, 0), idx(0, 4));
        assert(!r.found && r.path.empty());
    }

    // 5. Origen igual al destino: camino de una casilla y cero movimientos
    {
        Grid<Cell, 3, 5> g{};
        const Graph graph{g, rules};
        const SearchResult r = bfs(graph, idx(1, 2), idx(1, 2));
        assert(r.found && r.cost == 0);
        assert((r.path == std::vector<std::size_t>{idx(1, 2)}));
    }

    // 6. Destino en un muro o fuera de rango: no hay camino
    {
        Grid<Cell, 3, 5> g{};
        g.at({2, 4}) = Wall{};
        const Graph graph{g, rules};
        assert(!bfs(graph, idx(0, 0), idx(2, 4)).found);
        assert(!bfs(graph, idx(0, 0), 999).found);
    }

    // 7. BFS cuenta movimientos, no energía: el terreno elevado no cambia el resultado
    {
        Grid<Cell, 3, 5> g{};
        g.at({0, 1}) = RoughTerrain{};
        g.at({0, 2}) = RoughTerrain{};
        const Graph graph{g, rules};
        const SearchResult r = bfs(graph, idx(0, 0), idx(0, 3));
        assert(r.cost == 3);
    }

    std::cout << "bfs_test: todas las pruebas pasaron\n";
    return 0;
}
