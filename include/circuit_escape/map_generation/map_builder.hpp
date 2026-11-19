#pragma once
#include <cstddef>
#include <vector>
#include <random>
#include <algorithm>
#include "circuit_escape/grid.hpp"
#include "circuit_escape/cells.hpp"
#include "circuit_escape/graph/graph.hpp"

// Un muro que se puede quitar porque está entre dos salas
struct Opening
{
    std::size_t wall;   // número de la casilla del muro
    std::size_t cellA;  // sala de un lado
    std::size_t cellB;  // sala del otro lado
    int weight;         // valor al azar (NO es energía)
};

// Lo que devuelve Kruskal
struct KruskalResult
{
    std::vector<Opening> tree;     // aperturas elegidas (el árbol)
    std::vector<Opening> rejected; // las que habrían formado un ciclo
};

// Kruskal puro: no sabe nada del tablero, solo de aperturas y casillas
inline KruskalResult kruskal(std::size_t cellCount, std::vector<Opening> openings) {
    std::sort(openings.begin(), openings.end(),
        [](Opening a, Opening b) {
            if (a.weight == b.weight) {
                return a.wall < b.wall;
            }

            return a.weight < b.weight;
        }
        );

    DisjointSet sets{cellCount};
    KruskalResult result{};

    for (const auto& opening : openings) {
        if (sets.unite(opening.cellA, opening.cellB)) {
            result.tree.push_back(opening);
        } else {
            result.rejected.push_back(opening);
        }
    }

    return result;
}

// Un mapa ya construido
template <std::size_t Rows, std::size_t Columns>
struct BuiltMap
{
    Grid<Cell, Rows, Columns> grid;
    std::size_t id;                     // número del mapa (1, 2, 3...)
    unsigned seed;                      // semilla usada
    std::vector<Opening> treeOpenings;  // aperturas del árbol
    std::vector<Opening> extraOpenings; // aperturas adicionales
};

template <std::size_t Rows, std::size_t Columns>
class MapBuilder
{
public:
    // Construye un mapa completo
    [[nodiscard]] static BuiltMap<Rows, Columns> build(std::size_t id, unsigned seed, std::size_t extraCount) {
        std::mt19937 random{seed};

        MapGrid grid = prepareGrid();
        std::vector<Opening> aperturas = listOpenings(random);
        KruskalResult kruskalResult = kruskal(Rows * Columns, aperturas);
        openWalls(grid, kruskalResult.tree);
        std::vector<Opening> chosen = pickExtras(kruskalResult.rejected, extraCount, random);
        openWalls(grid, chosen);

        return {grid, id, seed, kruskalResult.tree, chosen};
    }


private:
    using MapGrid = Grid<Cell, Rows, Columns>;

    // Tablero inicial: salas libres, todo lo demás muro
    static MapGrid prepareGrid() {
        MapGrid map_grid{};

        for (std::size_t i = 0; i < Rows; ++i) {
            for (std::size_t j = 0; j < Columns; ++j) {
                if (!(i%2 == 0 && j%2 == 0)) {
                    map_grid.at({i, j}) = Wall{};
                }
            }
        }

        return map_grid;
    }

    // Lista de muros entre dos salas, cada uno con su valor al azar
    static std::vector<Opening> listOpenings(std::mt19937 &random) {
        std::uniform_int_distribution<int> distribution{1, 100};
        std::vector<Opening> aperturas;

        for (std::size_t i = 0; i < Rows; i++) {
            for (std::size_t j = 0; j < Columns; j++) {
                if (i%2 == 0 && j%2 == 1 && j != Columns-1) {
                    aperturas.emplace_back(toIndex({i, j}, Columns),
                        toIndex({i, j-1}, Columns),
                        toIndex({i, j+1}, Columns),
                        distribution(random));
                } else if (i%2 == 1 && j%2 == 0 && i != Rows-1) {
                    aperturas.emplace_back(toIndex({i, j}, Columns),
                        toIndex({i-1, j}, Columns),
                        toIndex({i+1, j}, Columns),
                        distribution(random));
                }
            }
        }

        return aperturas;
    }

    // Quita del tablero los muros indicados
    static void openWalls(MapGrid &grid, const std::vector<Opening> &openings) {
        for (const auto& opening : openings) {
            grid.at(toPosition(opening.wall, Columns)) = Empty{};
        }
    }

    // Elige al azar 'count' aperturas extra de entre las rechazadas
    static std::vector<Opening> pickExtras(std::vector<Opening> rejected, std::size_t count, std::mt19937 &random) {
        std::shuffle(rejected.begin(), rejected.end(), random);
        std::vector<Opening> chosen{};

        const std::size_t total = std::min(count, rejected.size());   // nunca más de las que hay
        for (std::size_t i = 0; i < total; ++i) {
            chosen.push_back(rejected[i]);
        }

        return chosen;
    }
};