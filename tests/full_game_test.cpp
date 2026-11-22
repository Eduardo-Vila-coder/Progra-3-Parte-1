// Partida completa reproducible (enunciado, sección 7): la misma secuencia de acciones,
// tras reset(seed), debe terminar exactamente igual.
#include <cassert>
#include <iostream>

#include "circuit_escape/controllers.hpp"
#include "circuit_escape/game_rules.hpp"
#include "circuit_escape/scenario.hpp"
#include "circuit_escape/simulation.hpp"

int main() {
    const auto scenario = loadScenario<20, 30>(scenarioPath(scenario02File));
    NavigationEnvironment<20, 30> environment =
        makeEnvironment(scenario.grid, scenario.start, Difficulty::Standard);

    HeuristicPolicy firstPolicy;
    const SimulationResult first = runSimulation(environment, firstPolicy, 7);
    assert(first.reason == EndReason::goalReached);
    assert(first.turns == 31);  // mismo recorrido que verifica ui_test
    assert(first.energy > 0);
    assert(!first.actions.empty());
    const Observation firstFinal = environment.state();

    // Un controlador nuevo evita arrastrar el estado interno ("casilla anterior") de la
    // primera partida; el entorno se reinicia con la misma semilla.
    HeuristicPolicy replayPolicy;
    const SimulationResult replay = runSimulation(environment, replayPolicy, 7);
    assert(first == replay);
    const Observation replayFinal = environment.state();
    assert(firstFinal.turn == replayFinal.turn);
    assert(firstFinal.agent == replayFinal.agent);
    assert(firstFinal.energy == replayFinal.energy);
    assert(firstFinal.score == replayFinal.score);
    assert(firstFinal.collectedResources == replayFinal.collectedResources);
    assert(firstFinal.switchAActive == replayFinal.switchAActive);
    assert(firstFinal.switchBActive == replayFinal.switchBActive);
    assert(firstFinal.guard == replayFinal.guard);

    std::cout << "full_game_test: todas las pruebas pasaron\n";
    return 0;
}
