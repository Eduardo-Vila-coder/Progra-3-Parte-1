#pragma once

#include <filesystem>
#include <fstream>
#include <stdexcept>
#include <string>

#include "circuit_escape/cells.hpp"
#include "circuit_escape/map_generation/map_layout.hpp"
#include "circuit_escape/overloaded.hpp"
#include "circuit_escape/position.hpp"

// Escritor de escenarios para los mapas producidos por MapBatchRunner.
// La primera parte del archivo son comentarios ';' con los datos del enunciado
// (id, semilla, nivel, costo mínimo y posiciones); el resto es el tablero ASCII que
// ya entiende scenario.hpp. Los comentarios se ignoran al cargar.
namespace map_writer {

[[nodiscard]] inline char symbolFor(const Cell& cell)
{
    return std::visit(
        Overloaded{
            [](const Empty&) { return '.'; },
            [](const Wall&) { return '#'; },
            [](const RoughTerrain&) { return '~'; },
            [](const ResourceCell<int>& resource) { return resource.collected ? '.' : 'R'; },
            [](const Battery& battery) { return battery.consumed ? '.' : 'B'; },
            [](const Trap&) { return 'T'; },
            [](const Exit&) { return 'S'; },
            [](const Switch& switchCell) {
                return switchCell.id == SwitchId::A ? 'a' : 'b';
            },
        },
        cell);
}

template <std::size_t Rows, std::size_t Columns>
void saveScenario(const std::filesystem::path& path,
                  const BuiltMap<Rows, Columns>& map,
                  const map_layout::MapEntities& entities,
                  const std::string& level,
                  int minimumCost)
{
    std::ofstream file(path);
    if (!file) {
        throw std::runtime_error("No se pudo escribir el escenario: " + path.string());
    }

    file << "; generado_por=MapBatchRunner\n";
    file << "; id=" << map.id << '\n';
    file << "; semilla=" << map.seed << '\n';
    file << "; nivel=" << level << '\n';
    file << "; filas=" << Rows << '\n';
    file << "; columnas=" << Columns << '\n';
    file << "; costo_minimo=" << minimumCost << '\n';
    file << "; inicio=" << entities.start.row << ',' << entities.start.column << '\n';
    file << "; salida=" << entities.goal.row << ',' << entities.goal.column << '\n';
    file << "; interruptor_A=" << entities.switchA.row << ',' << entities.switchA.column << '\n';
    file << "; interruptor_B=" << entities.switchB.row << ',' << entities.switchB.column << '\n';
    file << "; vigilante=" << entities.guard.row << ',' << entities.guard.column << '\n';

    for (std::size_t row = 0; row < Rows; ++row) {
        for (std::size_t column = 0; column < Columns; ++column) {
            const Position position{row, column};
            char symbol = symbolFor(map.grid.at(position));
            if (position == entities.start) {
                symbol = '@';
            } else if (position == entities.goal) {
                symbol = 'S';
            } else if (position == entities.switchA) {
                symbol = 'a';
            } else if (position == entities.switchB) {
                symbol = 'b';
            } else if (position == entities.guard) {
                symbol = 'V';
            }
            file << symbol;
        }
        file << '\n';
    }

    if (!file) {
        throw std::runtime_error("Error al escribir el escenario: " + path.string());
    }
}

} // namespace map_writer
