// Pruebas del Dijkstra entregado por Mathius (proyecto 2, secciones 5 y 7).
// La API real es Disjtrak::buscar + Config_algoritmo; no existe todavía el
// MapValidator ni el tipo SearchResult que se plantearon en docs/tests_plan.md.
#include <cassert>
#include <iostream>
#include <optional>
#include <string>
#include <variant>
#include <vector>

#include "circuit_escape/grid.hpp"
#include "circuit_escape/pathfinding/config_algoritmo.hpp"
#include "circuit_escape/pathfinding/logica_algoritmo.hpp"
#include "circuit_escape/scenario.hpp"

namespace {

struct TestNode {
    int peso_{1};
    bool pisable{true};
    std::size_t row_{};
    std::size_t column_{};

    std::size_t id(std::size_t columns) const noexcept { return row_ * columns + column_; }
};

// Adaptador mínimo para probar Disjtrak sin depender del tipo privado de Config_algoritmo.
class TestNodes {
public:
    explicit TestNodes(const Grid<Cell, 3, 5>& grid) : grid_(grid) {}

    std::size_t filas() const noexcept { return 3; }
    std::size_t columnas() const noexcept { return 5; }

    std::optional<TestNode> construir(std::size_t row, std::size_t column) const {
        if (row >= 3 || column >= 5) {
            return std::nullopt;
        }
        const bool wall = std::holds_alternative<Wall>(grid_.at({row, column}));
        return TestNode{1, !wall, row, column};
    }

private:
    const Grid<Cell, 3, 5>& grid_;
};

bool contains(const std::string& text, const std::string& fragment) {
    return text.find(fragment) != std::string::npos;
}

}  // namespace

int main() {
    Config_algoritmo config;

    // 1. Dijkstra devuelve el menor costo y reconstruye el camino de extremo a extremo.
    {
        Grid<Cell, 3, 5> grid{};
        TestNodes nodes{grid};
        std::vector<std::pair<int, int>> path;
        const int cost = Disjtrak{}.buscar(nodes, *nodes.construir(1, 0),
                                           *nodes.construir(1, 4), path);
        assert(cost == 4);                       // distancia Manhattan: todas las celdas cuestan 1
        assert(path.size() == 5);
        assert((path.front() == std::pair<int, int>{1, 0}));
        assert((path.back() == std::pair<int, int>{1, 4}));
    }

    // 2. Un muro que separa origen y destino no debe producir un camino falso.
    {
        Grid<Cell, 3, 5> grid{};
        grid.at({0, 2}) = Wall{};
        grid.at({1, 2}) = Wall{};
        grid.at({2, 2}) = Wall{};
        TestNodes nodes{grid};
        std::vector<std::pair<int, int>> path;
        assert(Disjtrak{}.buscar(nodes, *nodes.construir(1, 0),
                                 *nodes.construir(1, 4), path) == -1);
        assert(path.empty());
    }

    // 3. Config_algoritmo calcula el costo con las reglas de la dificultad.
    //    En standard el terreno elevado cuesta 2: 1 + 2 + 1 + 1 = 5.
    {
        Grid<Cell, 3, 5> grid{};
        grid.at({1, 2}) = RoughTerrain{};
        const std::string shortEnergy = config.calculo(grid, {1, 0}, {1, 4}, 180, 5);
        assert(contains(shortEnergy, "costo de: 5"));
        assert(contains(shortEnergy, "no alcanza energia"));

        const std::string enoughEnergy = config.calculo(grid, {1, 0}, {1, 4}, 180, 6);
        assert(enoughEnergy.find("El camino a seguir es : ") == 0);
        assert(contains(enoughEnergy, "("));
    }

    // 4. Casos límite de la interfaz de cálculo: destino inalcanzable, fuera del
    //    tablero, origen igual al destino y energía cero (llegar a cero no gana).
    {
        Grid<Cell, 3, 5> grid{};
        grid.at({0, 2}) = Wall{};
        grid.at({1, 2}) = Wall{};
        grid.at({2, 2}) = Wall{};
        assert(config.calculo(grid, {1, 0}, {1, 4}, 180, 100) ==
               "No se puede llegar a la salida");

        assert(config.calculo(grid, {1, 0}, {3, 0}, 180, 100) ==
               "Posicion fuera del tablero");

        Grid<Cell, 3, 5> free{};
        assert(config.calculo(free, {1, 0}, {1, 0}, 180, 100) == "Ya estas en la salida");
        assert(contains(config.calculo(free, {1, 0}, {1, 4}, 180, 0), "no alcanza energia"));
    }

    // 5. La validación de mapas usa el mismo algoritmo sobre un escenario real.
    {
        assert((config.validacion_mapas<20, 30>(scenarioPath(scenario01File)) == "Mapa valido"));
        assert(contains(config.validacion_mapas<20, 30>("no_existe.txt"), "Mapa invalido:"));
    }

    std::cout << "dijkstra_test: todas las pruebas pasaron\n";
    return 0;
}
