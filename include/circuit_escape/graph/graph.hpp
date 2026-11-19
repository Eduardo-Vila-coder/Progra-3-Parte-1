#pragma once
#include <cstddef>
#include <vector>
#include <array>
#include <variant>
#include <utility>
#include "circuit_escape/grid.hpp"
#include "circuit_escape/cells.hpp"
#include "circuit_escape/game_rules.hpp"
#include "circuit_escape/position.hpp"

// Convierte una posición (fila, columna) en el número único de esa casilla
[[nodiscard]] constexpr std::size_t toIndex(Position position, std::size_t columns) noexcept
{
    return position.row * columns + position.column;
}

// Hace lo contrario: a partir del número de casilla, recupera su posición
[[nodiscard]] constexpr Position toPosition(std::size_t index, std::size_t columns) noexcept
{
    return Position{index / columns, index % columns};
}

// ---------- Pieza 1: DisjointSet ----------
// Las funciones se implementan en src/graph/graph.cpp
class DisjointSet
{
public:
    // Crea 'size' grupos, uno por casilla
    explicit DisjointSet(std::size_t size);

    // Devuelve el "jefe" del grupo de la casilla
    [[nodiscard]] std::size_t find(std::size_t cell);

    // Une los grupos de a y b. Devuelve true si estaban separados y se unieron
    bool unite(std::size_t a, std::size_t b);

    // Dice si a y b están en el mismo grupo
    [[nodiscard]] bool connected(std::size_t a, std::size_t b);

private:
    std::vector<std::size_t> parent_; // jefe de cada casilla
    std::vector<std::size_t> size_;   // tamaño de cada grupo
};

// ---------- Pieza 2: grafo del mapa ----------
// Al ser plantilla, las funciones se escriben completas aquí mismo (no van en .cpp)
template <std::size_t Rows, std::size_t Columns>
class MapGraph
{
public:
    // Arma el grafo a partir del tablero ya construido y las reglas de costos
    template <Difficulty D>
    MapGraph(const Grid<Cell, Rows, Columns> &grid, const GameRules<D> &rules) {
        // Accedemos a cada celda de grid en orden
        for (std::size_t row = 0; row < Rows; ++row) {
            for (std::size_t column = 0; column < Columns; ++column) {
                if (std::holds_alternative<Wall>(grid.at(Position{row, column}))) {
                    open_[row * Columns + column] = false;
                    costs_[row * Columns + column] = 0;
                    continue;
                }

                open_[row * Columns + column] = true;

                costs_[row * Columns + column] = rules.energyCostGeneral;

                if (std::holds_alternative<Trap>(grid.at(Position{row, column}))) {
                    costs_[row * Columns + column] += rules.energyPenalty;
                } else if (std::holds_alternative<RoughTerrain>(grid.at(Position{row, column}))) {
                    costs_[row * Columns + column] = rules.energyCostRoughTerrain;
                }

                // Declaramos la posicion relativa de cada vecino de la celda
                std::vector<std::pair<int, int>> adjacent {
                            {0, 1} ,
                    {-1, 0},            {1, 0},
                            {0, -1}};

                for (const auto& [x, y] : adjacent) {
                    if (int(row) + x < 0 || int(row) + x >= Rows || int(column) + y < 0 || int(column) + y >= Columns
                        || std::holds_alternative<Wall>(grid.at(Position{row + x, column + y}))) {
                        continue;
                    }
                    neighbors_[row * Columns + column].push_back((row + x) * Columns + column + y);
                }
            }
        }
    }

    // Casillas a las que se puede pasar desde 'cell', siempre en el mismo orden
    [[nodiscard]] const std::vector<std::size_t> &neighbors(std::size_t cell) const
    {
        return neighbors_.at(cell);
    }

    // Energía que cuesta entrar a 'cell'
    [[nodiscard]] int cost(std::size_t cell) const
    {
        return costs_.at(cell);
    }

    // Dice si 'cell' no es muro
    [[nodiscard]] bool isOpen(std::size_t cell) const
    {
        return open_.at(cell);
    }

    // Total de casillas (filas × columnas)
    [[nodiscard]] static constexpr std::size_t size() noexcept { return Rows * Columns; }

private:
    std::array<std::vector<std::size_t>, Rows * Columns> neighbors_{}; // vecinos de cada casilla
    std::array<int, Rows * Columns> costs_{};                          // costo de entrar a cada casilla
    std::array<bool, Rows * Columns> open_{};                          // true si no es muro
};























