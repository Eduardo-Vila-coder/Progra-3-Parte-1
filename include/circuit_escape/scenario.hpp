#pragma once
#include <cstddef>
#include <fstream>
#include <stdexcept>
#include <string>
#include <vector>
#include "cells.hpp"
#include "grid.hpp"
#include "position.hpp"

// --- ESCENARIOS DE DEMOSTRACIÓN (enunciado 5.1, 10.2 y 13) ---
// Un escenario es un mapa de texto con la convención ASCII del enunciado (5.8):
//   .  espacio libre        #  muro             ~  terreno de costo elevado
//   R  recurso              B  batería          T  trampa
//   S  salida               @  inicio del agente (la celda es un espacio libre)
// Cada línea es una fila y cada carácter una columna.

// Carpeta de los mapas. CMake la define con la ruta absoluta de assets/maps
// para que funcione sin importar desde qué carpeta se ejecute el programa o las pruebas.
#ifndef CIRCUIT_ESCAPE_MAPS_DIR
#define CIRCUIT_ESCAPE_MAPS_DIR "assets/maps"
#endif

// Escenario 1: reproducción exacta del ejemplo de 20 x 30 del enunciado (página 11).
// NOTA: se puede completar en easy y standard, pero NO en hard. La ruta más corta necesita
// 44 movimientos (distancia Manhattan de (1,1) a (18,28)) y en hard solo hay 40 de energía
// más 2 de la única batería (42 en total). Se mantiene idéntico al enunciado a propósito.
inline const std::string scenario01File = "scenario_01.txt";

// Escenario 2: diseño propio con dos rutas (paso norte con batería y paso sur con terreno elevado).
// Se puede completar en easy, standard y hard.
inline const std::string scenario02File = "scenario_02.txt";

// Ruta completa de un archivo dentro de la carpeta de mapas
inline std::string scenarioPath(const std::string& fileName) {
    return std::string(CIRCUIT_ESCAPE_MAPS_DIR) + "/" + fileName;
}

// Resultado de cargar un escenario: el tablero y la posición inicial del agente
template <std::size_t Rows, std::size_t Columns>
struct Scenario {
    Grid<Cell, Rows, Columns> grid;
    Position start;
};

// Convierte un símbolo del mapa en su celda
inline Cell cellFromSymbol(char symbol) {
    switch (symbol) {
    case '.':
    case '@': return Empty{};
    case '#': return Wall{};
    case '~': return RoughTerrain{};
    case 'R': return ResourceCell<int>{10};
    case 'B': return Battery{};
    case 'T': return Trap{};
    case 'S': return Exit{};
    }
    throw std::invalid_argument(std::string("Escenario: simbolo desconocido '") + symbol + "'");
}

// Construye un escenario a partir de una colección de cadenas (una por fila).
// Valida las dimensiones, los símbolos y que haya exactamente un inicio y una salida.
template <std::size_t Rows, std::size_t Columns>
Scenario<Rows, Columns> scenarioFromLines(const std::vector<std::string>& lines) {
    if (lines.size() != Rows) {
        throw std::invalid_argument("Escenario: se esperaban " + std::to_string(Rows) +
                                    " filas y hay " + std::to_string(lines.size()));
    }

    Scenario<Rows, Columns> scenario{};
    std::size_t starts = 0;
    std::size_t exits = 0;

    for (std::size_t row = 0; row < Rows; ++row) {
        if (lines[row].size() != Columns) {
            throw std::invalid_argument("Escenario: la fila " + std::to_string(row) + " debe tener " +
                                        std::to_string(Columns) + " columnas");
        }
        for (std::size_t column = 0; column < Columns; ++column) {
            const char symbol = lines[row][column];
            scenario.grid.at({row, column}) = cellFromSymbol(symbol);
            if (symbol == '@') {
                scenario.start = Position{row, column};
                ++starts;
            } else if (symbol == 'S') {
                ++exits;
            }
        }
    }

    if (starts != 1) {
        throw std::invalid_argument("Escenario: debe haber exactamente un inicio '@'");
    }
    if (exits != 1) {
        throw std::invalid_argument("Escenario: debe haber exactamente una salida 'S'");
    }
    return scenario;
}

// Lee un escenario desde un archivo de texto. Ignora las líneas vacías y el '\r' de Windows.
template <std::size_t Rows, std::size_t Columns>
Scenario<Rows, Columns> loadScenario(const std::string& path) {
    std::ifstream file(path);
    if (!file) {
        throw std::runtime_error("Escenario: no se pudo abrir " + path);
    }

    std::vector<std::string> lines;
    std::string line;
    while (std::getline(file, line)) {
        if (!line.empty() && line.back() == '\r') {
            line.pop_back(); // fin de línea CRLF (Windows)
        }
        if (!line.empty()) {
            lines.push_back(line);
        }
    }
    return scenarioFromLines<Rows, Columns>(lines);
}
