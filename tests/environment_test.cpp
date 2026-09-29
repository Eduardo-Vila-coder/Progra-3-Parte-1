#include <cassert>
#include <iostream>
#include "../include/circuit_escape/environment.hpp"
#include "../include/circuit_escape/cells.hpp"

int main() {
    // Prueba 1 (consulta movimiento): Un agente en (0, 0) solo debe tener los movimientos validos: down, right, wait
    // Actualizar con el agente
    Grid<Cell, 20, 30> grid{};
    NavigationEnvironment<20, 30> env{grid, Position(0, 0), 10, 13};
    // Definir un agente
    std::vector<Action> availableActions1{Action::down, Action::right, Action::wait};
    assert(availableActions1 == env.availableActions());

    // Prueba 2 (consulta movimiento): Un agente en (19, 29) solo debe tener los movimientos validos: up, left, wait
    // std::vector<Action> availableActions2{Action::up, Action::left, Action::wait};
    // assert(availableActions2 == env.availableActions());

    // Prueba 3 (consulta movimiento): Un agente en (10, 10) en un mapa con Walls en (9, 10) y (11, 10)
    // solo debe tener los movimientos validos: left, right, wait
    // std::vector<Action> availableActions3{Action::left, Action::right, Action::wait};
    // assert(availableActions3 == env.availableActions());

    // Prueba 1 (carga perfiles): Dificultad estandar
    auto g1 = GameRules<Difficulty::Standard>{};
    assert(g1.initialMaxEnergy == 60);
    assert(g1.turnLimit == 180);
    assert(g1.energyCostGeneral == 1);
    assert(g1.energyCostRoughTerrain == 2);
    assert(g1.energyCostOthers == 1);
    assert(g1.reward == 10);
    assert(g1.energy == 3);
    assert(g1.energyPenalty == 2);
    assert(g1.scorePenalty == 1);

    // Prueba 2 (carga perfiles): Dificultad sencilla
    auto g2 = GameRules<Difficulty::Easy>{};
    assert(g2.initialMaxEnergy == 80);
    assert(g2.turnLimit == 240);
    assert(g2.energyCostGeneral == 1);
    assert(g2.energyCostRoughTerrain == 2);
    assert(g2.energyCostOthers == 1);
    assert(g2.reward == 15);
    assert(g2.energy == 5);
    assert(g2.energyPenalty == 1);
    assert(g2.scorePenalty == 0);

    // Prueba 3 (carga perfiles): Dificultad elevada
    auto g3 = GameRules<Difficulty::Hard>{};
    assert(g3.initialMaxEnergy == 40);
    assert(g3.turnLimit == 140);
    assert(g3.energyCostGeneral == 1);
    assert(g3.energyCostRoughTerrain == 3);
    assert(g3.energyCostOthers == 1);
    assert(g3.reward == 8);
    assert(g3.energy == 2);
    assert(g3.energyPenalty == 3);
    assert(g3.scorePenalty == 2);

    return 0;
}