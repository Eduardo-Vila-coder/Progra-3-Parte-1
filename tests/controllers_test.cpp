#include <algorithm>
#include <cassert>
#include <iostream>
#include <list>
#include <stdexcept>
#include <vector>
#include "../include/circuit_escape/controllers.hpp"
#include "../include/circuit_escape/environment.hpp"

// Observación mínima para probar las políticas sin necesidad de un entorno
Observation observationAt(Position agent, Position goal) {
    Observation observation{};
    observation.agent = agent;
    observation.goal = goal;
    return observation;
}

// ===== CONCEPT NavigationPolicy (se comprueba en compilación) =====
// (RandomPolicy y HeuristicPolicy ya se comprueban con static_assert en controllers.hpp)
// Tipos que NO cumplen el contrato:
struct PolicyWithoutSelectAction {};
struct PolicyReturningInt {
    int selectAction(const Observation&, std::span<const Action>) { return 0; }
};
static_assert(!NavigationPolicy<int>);
static_assert(!NavigationPolicy<PolicyWithoutSelectAction>);
static_assert(!NavigationPolicy<PolicyReturningInt>); // devuelve int, no Action

int main() {
    const std::vector<Action> allActions{Action::up, Action::down, Action::left, Action::right, Action::wait};

    // ===== TEMPLATE bestBy (rangos vacíos y distintos contenedores) =====

    // Prueba 1: Con un rango vacío retorna last, tanto en vector como en list
    {
        std::vector<Action> emptyVector;
        assert(bestBy(emptyVector.begin(), emptyVector.end(), [](Action) { return 0; }) == emptyVector.end());

        std::list<Position> emptyList;
        assert(bestBy(emptyList.begin(), emptyList.end(), [](Position) { return 0; }) == emptyList.end());
    }

    // Prueba 2: Funciona con vector<Action> (elige el de menor costo)
    {
        std::vector<Action> actions{Action::up, Action::left, Action::right};
        auto best = bestBy(actions.begin(), actions.end(), [](Action action) {
            return action == Action::left ? 0 : 5;
        });
        assert(*best == Action::left);
    }

    // Prueba 3: Funciona con list<Position> (otro contenedor y otro tipo de dato)
    {
        std::list<Position> positions{Position(5, 5), Position(1, 2), Position(3, 0)};
        auto best = bestBy(positions.begin(), positions.end(), [](Position position) {
            return manhattanDistance(position, Position(0, 0));
        });
        assert(*best == Position(1, 2));
    }

    // Prueba 4: En empate gana el primero del rango
    {
        std::vector<Action> actions{Action::down, Action::right};
        auto best = bestBy(actions.begin(), actions.end(), [](Action) { return 1; });
        assert(*best == Action::down);
    }

    // ===== POLÍTICA ALEATORIA =====

    // Prueba 5: Siempre devuelve una acción legal
    {
        RandomPolicy policy{42};
        const std::vector<Action> legal{Action::down, Action::right, Action::wait};
        for (int i = 0; i < 300; ++i) {
            Action chosen = policy.selectAction(Observation{}, legal);
            assert(std::find(legal.begin(), legal.end(), chosen) != legal.end());
        }
    }

    // Prueba 6: Con la misma semilla produce la misma secuencia de acciones
    {
        RandomPolicy first{7};
        RandomPolicy second{7};
        for (int i = 0; i < 100; ++i) {
            assert(first.selectAction(Observation{}, allActions) == second.selectAction(Observation{}, allActions));
        }
    }

    // Prueba 7: No elige siempre lo mismo: en 300 intentos aparecen todas las acciones legales
    {
        RandomPolicy policy{123};
        const std::vector<Action> legal{Action::up, Action::left, Action::wait};
        bool seen[5]{};
        for (int i = 0; i < 300; ++i) {
            seen[static_cast<int>(policy.selectAction(Observation{}, legal))] = true;
        }
        assert(seen[static_cast<int>(Action::up)]);
        assert(seen[static_cast<int>(Action::left)]);
        assert(seen[static_cast<int>(Action::wait)]);
    }

    // Prueba 8: Sin acciones legales lanza std::invalid_argument
    {
        RandomPolicy policy{1};
        bool threw = false;
        try {
            (void)policy.selectAction(Observation{}, std::vector<Action>{});
        } catch (const std::invalid_argument&) {
            threw = true;
        }
        assert(threw);
    }

    // ===== POLÍTICA HEURÍSTICA =====

    // Prueba 9: Elige el movimiento que reduce la distancia Manhattan a la salida
    {
        HeuristicPolicy policy;
        assert(policy.selectAction(observationAt(Position(1, 1), Position(1, 3)), allActions) == Action::right);

        HeuristicPolicy policy2;
        assert(policy2.selectAction(observationAt(Position(2, 2), Position(0, 2)), allActions) == Action::up);
    }

    // Prueba 10: Evita obstáculos inmediatos: solo usa acciones legales
    // (a la derecha hay un muro, así que right no es legal aunque acerque a la salida)
    {
        HeuristicPolicy policy;
        const std::vector<Action> legal{Action::up, Action::down, Action::wait};
        Action chosen = policy.selectAction(observationAt(Position(1, 1), Position(1, 3)), legal);
        assert(chosen == Action::up || chosen == Action::down);
    }

    // Prueba 11: No elige wait si existe algún movimiento, aunque el movimiento aleje de la salida
    {
        HeuristicPolicy policy;
        const std::vector<Action> legal{Action::left, Action::wait};
        assert(policy.selectAction(observationAt(Position(1, 1), Position(1, 3)), legal) == Action::left);
    }

    // Prueba 12: Si wait es la única acción legal, elige wait
    {
        HeuristicPolicy policy;
        const std::vector<Action> legal{Action::wait};
        assert(policy.selectAction(observationAt(Position(1, 1), Position(1, 3)), legal) == Action::wait);
    }

    // Prueba 13: En empate de distancia decide de forma determinista (primer movimiento legal)
    {
        HeuristicPolicy policy;
        // down y right acercan igual a (2, 2); down aparece primero
        assert(policy.selectAction(observationAt(Position(1, 1), Position(2, 2)), allActions) == Action::down);
    }

    // Prueba 14: No retrocede a la casilla anterior si hay otra opción
    {
        HeuristicPolicy policy;
        // Turno 1: desde (1,1) hacia la salida (0,0): up y left empatan, elige up -> (0,1)
        assert(policy.selectAction(observationAt(Position(1, 1), Position(0, 0)), allActions) == Action::up);
        // Turno 2: en (0,1) solo hay down (regresa a (1,1)) y right; ambos a distancia 2.
        // La voraz simple elegiría down (primero), pero no se retrocede: elige right
        const std::vector<Action> legal{Action::down, Action::right, Action::wait};
        assert(policy.selectAction(observationAt(Position(0, 1), Position(0, 0)), legal) == Action::right);
    }

    // Prueba 15: Retrocede si es la única opción (callejón sin salida)
    {
        HeuristicPolicy policy;
        assert(policy.selectAction(observationAt(Position(1, 1), Position(0, 0)), allActions) == Action::up);
        const std::vector<Action> legal{Action::down, Action::wait};
        assert(policy.selectAction(observationAt(Position(0, 1), Position(0, 0)), legal) == Action::down);
    }

    // Prueba 16: Sin acciones legales lanza std::invalid_argument
    {
        HeuristicPolicy policy;
        bool threw = false;
        try {
            (void)policy.selectAction(Observation{}, std::vector<Action>{});
        } catch (const std::invalid_argument&) {
            threw = true;
        }
        assert(threw);
    }

    // ===== PARTIDAS COMPLETAS CON EL ENTORNO =====

    // Prueba 17: La heurística sale de un callejón y llega a la salida.
    // Tablero 3x4 (S = inicio, E = salida, # = muro):
    //   . . . .
    //   . # . .
    //   S # . E
    // La voraz simple rebotaría entre (2,0) y (1,0) para siempre; con "no retroceder" llega en 7 turnos.
    {
        Grid<Cell, 3, 4> g{};
        g.at({1, 1}) = Wall{};
        g.at({2, 1}) = Wall{};
        g.at({2, 3}) = Exit{};
        NavigationEnvironment<3, 4> env{g, Position(2, 0), Position(2, 0), 20, 50};

        HeuristicPolicy policy;
        StepResult last{};
        while (!env.isFinished()) {
            Observation observation = env.state();
            last = env.step(policy.selectAction(observation, observation.availableActions));
        }
        assert(last.reason == EndReason::goalReached);
        assert(last.observation.turn == 7);
        assert(last.observation.energy == 13);
    }

    // Prueba 18: La política aleatoria solo propone acciones legales durante una partida completa
    // (nunca genera MovementRejectedEvent) y la partida termina
    {
        Grid<Cell, 3, 4> g{};
        g.at({1, 1}) = Wall{};
        g.at({2, 3}) = Exit{};
        NavigationEnvironment<3, 4> env{g, Position(0, 0), Position(0, 0), 30, 40};

        RandomPolicy policy{2026};
        while (!env.isFinished()) {
            Observation observation = env.state();
            StepResult result = env.step(policy.selectAction(observation, observation.availableActions));
            for (const NavigationEvent& event : result.events) {
                assert(!std::holds_alternative<MovementRejectedEvent>(event));
            }
        }
        assert(env.isFinished());
    }

    std::cout << "controllers_test: todas las pruebas pasaron\n";
    return 0;
}
