#include <cassert>
#include "../include/circuit_escape/environment.hpp"
#include "../include/circuit_escape/cells.hpp"

int main() {
    // Prueba 1 (movimiento): Un agente en (0, 0) solo debe tener los movimientos validos: down, right, wait
    // Actualizar con el agente
    Grid<Cell, 20, 30> grid{};
    NavigationEnvironment<20, 30> env{grid, Position(0, 0), 10, 13};
    // Definir un agente
    std::vector<Action> availableActions1{Action::down, Action::right, Action::wait};
    assert(availableActions1 == env.availableActions());

    // Prueba 2 (movimiento): Un agente en (19, 29) solo debe tener los movimientos validos: up, left, wait
    // std::vector<Action> availableActions2{Action::up, Action::left, Action::wait};
    // assert(availableActions2 == env.availableActions());

    // Prueba 3 (movimiento): Un agente en (10, 10) en un mapa con Walls en (9, 10) y (11, 10)
    // solo debe tener los movimientos validos: left, right, wait
    // std::vector<Action> availableActions3{Action::left, Action::right, Action::wait};
    // assert(availableActions3 == env.availableActions());

    return 0;
}