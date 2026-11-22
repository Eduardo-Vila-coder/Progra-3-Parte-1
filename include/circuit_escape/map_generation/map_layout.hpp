#pragma once

#include <algorithm>
#include <cstddef>
#include <stdexcept>
#include <vector>

#include "circuit_escape/cells.hpp"
#include "circuit_escape/map_generation/map_builder.hpp"
#include "circuit_escape/position.hpp"

// Colocación de los elementos jugables sobre un mapa generado por Kruskal.
// MapBuilder solo produce salas (Empty) y muros (Wall); aquí se eligen posiciones
// deterministas para el inicio, la salida, los interruptores A/B y el vigilante.
namespace map_layout {

// Última fila/columna que es sala (índice par). Para 20x30 devuelve 18 y 28.
[[nodiscard]] constexpr std::size_t lastRoomIndex(std::size_t extent) noexcept
{
    return extent == 0 ? 0 : ((extent - 1) / 2) * 2;
}

template <std::size_t Rows, std::size_t Columns>
[[nodiscard]] std::vector<Position> rooms(const Grid<Cell, Rows, Columns>& grid)
{
    std::vector<Position> result;
    for (std::size_t row = 0; row < Rows; row += 2) {
        for (std::size_t column = 0; column < Columns; column += 2) {
            if (isTraversable(grid.at({row, column}))) {
                result.push_back(Position{row, column});
            }
        }
    }
    return result;
}

struct MapEntities
{
    Position start{};
    Position goal{};
    Position switchA{};
    Position switchB{};
    Position guard{};
};

// Elige salas distintas en orden fijo y las escribe en el tablero. El inicio y el
// vigilante siguen siendo Empty en la celda; el escritor de texto los marca con '@' y 'V'.
template <std::size_t Rows, std::size_t Columns>
MapEntities placeEntities(BuiltMap<Rows, Columns>& map)
{
    const std::vector<Position> available = rooms(map.grid);
    if (available.size() < 5) {
        throw std::invalid_argument("El mapa no tiene salas suficientes para los elementos");
    }

    MapEntities entities{
        available.front(),
        available.back(),
        available[1],
        available[2],
        Position{},
    };

    const auto used = [&entities](const Position& position) {
        return position == entities.start || position == entities.goal ||
               position == entities.switchA || position == entities.switchB;
    };
    const auto guard = std::find_if(available.begin(), available.end(),
                                    [&used](const Position& position) { return !used(position); });
    entities.guard = *guard;

    map.grid.at(entities.goal) = Exit{};
    map.grid.at(entities.switchA) = Switch{SwitchId::A};
    map.grid.at(entities.switchB) = Switch{SwitchId::B};

    return entities;
}

} // namespace map_layout
