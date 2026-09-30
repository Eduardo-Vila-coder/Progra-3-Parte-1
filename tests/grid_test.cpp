// Pruebas de Grid<Cell, Rows, Columns> (enunciado 5.1, 5.7, 6.2 y 8):
// acceso válido e inválido, bordes y esquinas, versiones const, iteradores y orden de recorrido.

#include <algorithm>
#include <cassert>
#include <iostream>
#include <iterator>
#include <stdexcept>
#include <type_traits>
#include <variant>

#include "../include/circuit_escape/cells.hpp"
#include "../include/circuit_escape/grid.hpp"

// Las dimensiones forman parte del tipo y se conocen en compilación
static_assert(Grid<int, 3, 4>::rows() == 3);
static_assert(Grid<int, 3, 4>::columns() == 4);
static_assert(Grid<Cell, 20, 30>::rows() * Grid<Cell, 20, 30>::columns() == 600);
// iterator permite modificar; const_iterator no
static_assert(!std::is_same_v<Grid<int, 3, 4>::iterator, Grid<int, 3, 4>::const_iterator>);
static_assert(std::is_same_v<decltype(*std::declval<Grid<int, 3, 4>::iterator>()), int&>);
static_assert(std::is_same_v<decltype(*std::declval<Grid<int, 3, 4>::const_iterator>()), const int&>);
// Grid<int, 0, 4> no compila: static_assert(Rows > 0 && Columns > 0) dentro de Grid.

namespace {

bool throwsOutOfRange(const Grid<int, 3, 4>& grid, Position position) {
    try {
        (void)grid.at(position);
    } catch (const std::out_of_range&) {
        return true;
    }
    return false;
}

}  // namespace

int main() {
    // Prueba 1: contains acepta las cuatro esquinas y rechaza lo que está fuera
    {
        const Grid<int, 3, 4> grid{};
        assert(grid.contains({0, 0}));
        assert(grid.contains({0, 3}));
        assert(grid.contains({2, 0}));
        assert(grid.contains({2, 3}));
        assert(!grid.contains({3, 0}));  // una fila de más
        assert(!grid.contains({0, 4}));  // una columna de más
        assert(!grid.contains({3, 4}));
    }

    // Prueba 2: acceso válido (lectura y escritura) en esquinas y en el centro
    {
        Grid<int, 3, 4> grid{};
        grid.at({0, 0}) = 1;
        grid.at({0, 3}) = 2;
        grid.at({2, 0}) = 3;
        grid.at({2, 3}) = 4;
        grid.at({1, 2}) = 5;
        assert(grid.at({0, 0}) == 1);
        assert(grid.at({0, 3}) == 2);
        assert(grid.at({2, 0}) == 3);
        assert(grid.at({2, 3}) == 4);
        assert(grid.at({1, 2}) == 5);
        assert(grid.at({1, 1}) == 0);  // las demás celdas quedan con su valor por defecto
    }

    // Prueba 3: acceso inválido lanza std::out_of_range (fila, columna o ambas fuera)
    {
        const Grid<int, 3, 4> grid{};
        assert(throwsOutOfRange(grid, {3, 0}));
        assert(throwsOutOfRange(grid, {0, 4}));
        assert(throwsOutOfRange(grid, {100, 100}));
        assert(!throwsOutOfRange(grid, {2, 3}));

        Grid<int, 3, 4> mutableGrid{};
        bool threw = false;
        try {
            mutableGrid.at({3, 3}) = 1;  // versión no const
        } catch (const std::out_of_range&) {
            threw = true;
        }
        assert(threw);
    }

    // Prueba 4: la versión const devuelve una referencia const al mismo elemento
    {
        Grid<int, 3, 4> grid{};
        grid.at({1, 1}) = 7;
        const Grid<int, 3, 4>& view = grid;
        static_assert(std::is_same_v<decltype(view.at({1, 1})), const int&>);
        assert(view.at({1, 1}) == 7);
        assert(&view.at({1, 1}) == &grid.at({1, 1}));
    }

    // Prueba 5: los iteradores recorren las 12 celdas por filas (izquierda a derecha, arriba a abajo)
    {
        Grid<int, 3, 4> grid{};
        for (std::size_t row = 0; row < 3; ++row) {
            for (std::size_t column = 0; column < 4; ++column) {
                grid.at({row, column}) = static_cast<int>(row * 10 + column);
            }
        }
        assert(std::distance(grid.begin(), grid.end()) == 12);
        const int expected[] = {0, 1, 2, 3, 10, 11, 12, 13, 20, 21, 22, 23};
        assert(std::equal(grid.cbegin(), grid.cend(), std::begin(expected), std::end(expected)));
    }

    // Prueba 6: el iterador no const permite modificar las celdas
    {
        Grid<int, 3, 4> grid{};
        std::fill(grid.begin(), grid.end(), 9);
        for (int& value : grid) {
            value += 1;
        }
        assert(std::all_of(grid.cbegin(), grid.cend(), [](int value) { return value == 10; }));
    }

    // Prueba 7: los algoritmos de <algorithm> funcionan sobre un Grid de celdas
    {
        Grid<Cell, 3, 4> grid{};
        grid.at({0, 1}) = Wall{};
        grid.at({2, 2}) = Wall{};
        grid.at({1, 3}) = Exit{};
        const auto walls = std::count_if(grid.begin(), grid.end(), [](const Cell& cell) {
            return std::holds_alternative<Wall>(cell);
        });
        assert(walls == 2);
        const auto exit = std::find_if(grid.cbegin(), grid.cend(), [](const Cell& cell) {
            return std::holds_alternative<Exit>(cell);
        });
        assert(std::distance(grid.cbegin(), exit) == 1 * 4 + 3);  // índice = fila * columnas + columna
    }

    // Prueba 8: funciona con otras dimensiones conocidas en compilación (1 x 1 y 20 x 30)
    {
        Grid<int, 1, 1> tiny{};
        tiny.at({0, 0}) = 5;
        assert(tiny.at({0, 0}) == 5);
        assert(!tiny.contains({0, 1}));

        const Grid<Cell, 20, 30> big{};
        assert(std::distance(big.begin(), big.end()) == 600);
        assert(big.contains({19, 29}));
        assert(!big.contains({20, 0}));
    }

    std::cout << "grid_test: todas las pruebas pasaron\n";
    return 0;
}
