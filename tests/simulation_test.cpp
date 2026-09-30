#include <cassert>
#include <iostream>
#include "../include/circuit_escape/controllers.hpp"
#include "../include/circuit_escape/environment.hpp"
#include "../include/circuit_escape/game_rules.hpp"
#include "../include/circuit_escape/scenario.hpp"
#include "../include/circuit_escape/simulation.hpp"

int main() {
    // ===== reset(seed) =====

    // Prueba 1: reset devuelve la partida al estado inicial y el recurso se puede recoger otra vez
    {
        Grid<Cell, 3, 4> g{};
        g.at({0, 1}) = ResourceCell<int>{10};
        g.at({2, 3}) = Exit{};
        NavigationEnvironment<3, 4> env{g, Position(0, 0), 10, 20};

        StepResult first = env.step(Action::right); // recoge el recurso
        assert(first.observation.score == 10);
        assert(first.observation.collectedResources == 1);

        env.reset(5);
        Observation initial = env.state();
        assert(env.seed() == 5);
        assert(initial.agent == Position(0, 0));
        assert(initial.turn == 0);
        assert(initial.energy == 10);
        assert(initial.score == 0);
        assert(initial.collectedResources == 0);

        StepResult again = env.step(Action::right); // el recurso volvió a estar disponible
        assert(again.observation.score == 10);
        assert(again.observation.collectedResources == 1);
        // grid() entrega el tablero actual: el recurso quedó marcado como recogido
        assert(std::get<ResourceCell<int>>(env.grid().at({0, 1})).collected);
    }

    // Prueba 2: Después de terminar, reset permite jugar de nuevo (step ya no lanza logic_error)
    {
        Grid<Cell, 3, 4> g{};
        g.at({0, 1}) = Exit{};
        NavigationEnvironment<3, 4> env{g, Position(0, 0), 10, 20};
        (void)env.step(Action::right);
        assert(env.isFinished());

        env.reset(1);
        assert(!env.isFinished());
        assert(!env.availableActions().empty());
        StepResult result = env.step(Action::right);
        assert(result.reason == EndReason::goalReached);
    }

    // ===== SIMULACIÓN REPRODUCIBLE (escenario 2, perfil standard) =====
    auto scenario = loadScenario<20, 30>(scenarioPath(scenario02File));
    const GameRules<Difficulty::Standard> standard{};

    // Prueba 3: Misma semilla en dos entornos distintos -> exactamente la misma partida
    {
        NavigationEnvironment<20, 30> envA{scenario.grid, scenario.start, standard};
        NavigationEnvironment<20, 30> envB{scenario.grid, scenario.start, standard};
        RandomPolicy policyA{42};
        RandomPolicy policyB{42};
        SimulationResult a = runSimulation(envA, policyA, 42);
        SimulationResult b = runSimulation(envB, policyB, 42);
        assert(a == b);
        assert(a.reason != EndReason::none); // la partida terminó
        assert(a.turns == a.actions.size());
    }

    // Prueba 4: Mismo entorno, reset con la misma semilla -> se repite la partida
    // (comprueba que reset restaura todo, incluidas baterías y recursos consumidos)
    {
        NavigationEnvironment<20, 30> env{scenario.grid, scenario.start, standard};
        RandomPolicy first{7};
        SimulationResult a = runSimulation(env, first, 7);
        RandomPolicy second{7}; // política nueva con la misma semilla
        SimulationResult b = runSimulation(env, second, 7);
        assert(a == b);
    }

    // Prueba 5: Semillas distintas producen partidas distintas
    {
        NavigationEnvironment<20, 30> env{scenario.grid, scenario.start, standard};
        RandomPolicy base{0};
        SimulationResult reference = runSimulation(env, base, 0);
        bool someDifferent = false;
        for (std::uint32_t seed = 1; seed <= 5; ++seed) {
            RandomPolicy policy{seed};
            if (runSimulation(env, policy, seed).actions != reference.actions) {
                someDifferent = true;
            }
        }
        assert(someDifferent);
    }

    // Prueba 6: La heurística es determinista y completa el escenario 2
    {
        NavigationEnvironment<20, 30> env{scenario.grid, scenario.start, standard};
        HeuristicPolicy first;
        SimulationResult a = runSimulation(env, first, 0);
        HeuristicPolicy second; // política nueva: no arrastra memoria de la partida anterior
        SimulationResult b = runSimulation(env, second, 0);
        assert(a == b);
        assert(a.reason == EndReason::goalReached);
    }

    std::cout << "simulation_test: todas las pruebas pasaron\n";
    return 0;
}