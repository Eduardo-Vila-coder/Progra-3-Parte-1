#include <cassert>    // assert
#include <iostream>   // mensaje final
#include <variant>    // holds_alternative
#include <vector>
#include "circuit_escape/graph/graph.hpp"
#include "circuit_escape/map_generation/map_builder.hpp"
#include "circuit_escape/map_generation/map_selector.hpp"

int main() {
    // Prueba 1: DisjointSet une y detecta grupos
    {
        DisjointSet sets{6};
        assert(!sets.connected(0, 1));
        assert(sets.unite(0, 1));      // estaban separadas → true
        assert(!sets.unite(0, 1));     // ya juntas → false
        assert(sets.connected(1, 0));

        assert(sets.unite(1, 4));
        assert(sets.connected(0, 4));
        assert(!sets.connected(0, 5));
    }

    // Prueba 2: MapGraph
    {
        //  0: .   1: R   2: #
        //  3: .   4: .   5: T
        Grid<Cell, 2, 3> grid{};            // todas las casillas empiezan en Empty
        grid.at({0, 1}) = RoughTerrain{};   // terreno elevado
        grid.at({0, 2}) = Wall{};           // muro
        grid.at({1, 2}) = Trap{};           // trampa

        GameRules<> rules{};                // general 1, terreno 2, penalización de trampa 2
        MapGraph<2, 3> graph{grid, rules};

        assert(!graph.isOpen(2));
        assert(graph.isOpen(0) && graph.isOpen(1) && graph.isOpen(3) &&
               graph.isOpen(4) && graph.isOpen(5));

        assert(graph.cost(0) == 1);         // casilla normal
        assert(graph.cost(1) == 2);         // terreno elevado
        assert(graph.cost(5) == 3);         // trampa: 1 + 2

        assert(graph.neighbors(0) == std::vector<std::size_t>({1, 3}));
        assert(graph.neighbors(1) == std::vector<std::size_t>({4, 0}));
        assert(graph.neighbors(2).empty()); // muro: sin vecinos
        assert(graph.neighbors(3) == std::vector<std::size_t>({4, 0}));
        assert(graph.neighbors(4) == std::vector<std::size_t>({5, 1, 3}));
        assert(graph.neighbors(5) == std::vector<std::size_t>({4}));
    }

    // Prueba 3: kruskal con valores distintos
    {
        //  0  1  2
        //  3  4  5
        //  6  7  8
        std::vector<Opening> openings{
            Opening{1, 0, 2, 7},   // muro 1, une 0 y 2, valor 7
            Opening{3, 0, 6, 3},   // muro 3, une 0 y 6, valor 3
            Opening{5, 2, 8, 9},   // muro 5, une 2 y 8, valor 9
            Opening{7, 6, 8, 5},   // muro 7, une 6 y 8, valor 5
        };

        KruskalResult result = kruskal(9, openings);

        assert(result.tree.size() == 3);
        assert(result.tree[0].wall == 3);
        assert(result.tree[1].wall == 7);
        assert(result.tree[2].wall == 1);

        assert(result.rejected.size() == 1);
        assert(result.rejected[0].wall == 5);
    }

    // Prueba 4: kruskal desempata por número de muro
    {
        std::vector<Opening> tied{
            Opening{7, 6, 8, 5},   // todas con valor 5,
            Opening{5, 2, 8, 5},   // puestas de muro mayor a menor
            Opening{3, 0, 6, 5},
            Opening{1, 0, 2, 5},
        };

        KruskalResult result = kruskal(9, tied);

        assert(result.tree.size() == 3);
        assert(result.tree[0].wall == 1);
        assert(result.tree[1].wall == 3);
        assert(result.tree[2].wall == 5);

        assert(result.rejected.size() == 1);
        assert(result.rejected[0].wall == 7);   // sin desempate sería el muro 1
    }

    // Prueba 5: MapBuilder, cantidades y datos del mapa
    {
        BuiltMap<5, 5> map = MapBuilder<5, 5>::build(1, 42, 0);
        assert(map.treeOpenings.size() == 8);   // 9 salas - 1
        assert(map.extraOpenings.empty());
        assert(map.id == 1);
        assert(map.seed == 42);

        BuiltMap<5, 5> map2 = MapBuilder<5, 5>::build(1, 42, 2);
        assert(map2.extraOpenings.size() == 2);

        BuiltMap<5, 5> map3 = MapBuilder<5, 5>::build(1, 42, 100);
        assert(map3.extraOpenings.size() == 4); // 12 muros - 8 del árbol

        // Ninguna apertura extra repite una del árbol
        for (const auto &extra : map3.extraOpenings) {
            for (const auto &treeOpening : map3.treeOpenings) {
                assert(extra.wall != treeOpening.wall);
            }
        }
    }

    // Prueba 6: la misma semilla da el mismo mapa
    {
        BuiltMap<5, 5> a = MapBuilder<5, 5>::build(1, 42, 2);
        BuiltMap<5, 5> b = MapBuilder<5, 5>::build(1, 42, 2);

        for (std::size_t row = 0; row < 5; ++row) {
            for (std::size_t column = 0; column < 5; ++column) {
                bool wallA = std::holds_alternative<Wall>(a.grid.at({row, column}));
                bool wallB = std::holds_alternative<Wall>(b.grid.at({row, column}));
                assert(wallA == wallB);
            }
        }
    }

    // Prueba 7: todas las salas quedan conectadas por el árbol
    {
        BuiltMap<5, 5> map = MapBuilder<5, 5>::build(1, 42, 0);

        DisjointSet sets{25};
        for (const auto &opening : map.treeOpenings) {
            sets.unite(opening.cellA, opening.cellB);
        }

        // Las salas están en (par, par); todas deben estar unidas con la sala 0
        for (std::size_t row = 0; row < 5; row += 2) {
            for (std::size_t column = 0; column < 5; column += 2) {
                assert(sets.connected(0, toIndex({row, column}, 5)));
            }
        }
    }

    // Prueba 8: MapSelector con el ejemplo de 6 mapas
    {
        std::vector<ValidatedMap> maps{
            ValidatedMap{1, 18, true},
            ValidatedMap{2, 10, true},
            ValidatedMap{3, 25, false},   // sin victoria: se descarta
            ValidatedMap{4, 32, true},
            ValidatedMap{5, 18, true},
            ValidatedMap{6, 21, true},
        };

        SelectionResult result = MapSelector::select(maps);
        assert(result.status == SelectionStatus::Ok);
        assert(result.easy.id == 2);
        assert(result.medium.id == 6);
        assert(result.hard.id == 4);
    }

    // Prueba 9: el caso del enunciado (10, 18 y 32 con promedio 20)
    {
        std::vector<ValidatedMap> maps{
            ValidatedMap{1, 10, true},
            ValidatedMap{2, 18, true},
            ValidatedMap{3, 32, true},
        };

        SelectionResult result = MapSelector::select(maps);
        assert(result.status == SelectionStatus::Ok);
        assert(result.easy.cost == 10);
        assert(result.medium.cost == 18);
        assert(result.hard.cost == 32);
    }

    // Prueba 10: empates (gana el id más bajo)
    {
        std::vector<ValidatedMap> easyTie{
            ValidatedMap{5, 10, true},
            ValidatedMap{2, 10, true},    // mismo costo que el 5, id más bajo
            ValidatedMap{3, 18, true},
            ValidatedMap{4, 30, true},
        };
        SelectionResult easyResult = MapSelector::select(easyTie);
        assert(easyResult.easy.id == 2);

        // Promedio = (10 + 18 + 22 + 30) / 4 = 20
        // el 18 y el 22 quedan a distancia 2
        std::vector<ValidatedMap> mediumTie{
            ValidatedMap{1, 10, true},
            ValidatedMap{3, 18, true},    // aparece primero, pero tiene id mayor
            ValidatedMap{2, 22, true},
            ValidatedMap{4, 30, true},
        };
        SelectionResult mediumResult = MapSelector::select(mediumTie);
        assert(mediumResult.medium.id == 2);
    }

    // Prueba 11: lote sin tres costos distintos
    {
        std::vector<ValidatedMap> maps{
            ValidatedMap{1, 10, true},
            ValidatedMap{2, 10, true},
            ValidatedMap{3, 18, true},
        };

        SelectionResult result = MapSelector::select(maps);
        assert(result.status == SelectionStatus::NotEnoughDistinctCosts);
    }

    // Prueba 12: lote sin ninguna victoria
    {
        std::vector<ValidatedMap> maps{
            ValidatedMap{1, 10, false},
            ValidatedMap{2, 18, false},
            ValidatedMap{3, 32, false},
        };

        SelectionResult result = MapSelector::select(maps);
        assert(result.status == SelectionStatus::NoWinnableMaps);
    }

    std::cout << "Todas las pruebas de map_generation pasaron\n";
    return 0;
}