#pragma once
#include <cstddef>
#include <cstdint>
#include <vector>
#include "controllers.hpp"
#include "environment.hpp"

// --- SIMULACIÓN REPRODUCIBLE (enunciado 7, 8 y 10.2) ---
// Resumen de una partida automática: cómo terminó, los datos finales (enunciado 4)
// y la secuencia completa de acciones, que permite comparar dos ejecuciones.
struct SimulationResult {
    std::uint32_t seed{};
    EndReason reason{EndReason::none};
    std::size_t turns{};
    int energy{};
    int score{};
    std::size_t collectedResources{};
    std::vector<Action> actions;

    friend bool operator==(const SimulationResult&, const SimulationResult&) = default;
};

// Juega una partida completa sin consola (no usa std::cin ni std::cout):
// reinicia el entorno con la semilla y deja que la política elija cada acción hasta que la partida termine.
// Acepta cualquier tipo que cumpla el concept NavigationPolicy (se comprueba al compilar).
template <std::size_t Rows, std::size_t Columns, NavigationPolicy Policy>
SimulationResult runSimulation(NavigationEnvironment<Rows, Columns>& environment, Policy& policy,
                               std::uint32_t seed) {
    environment.reset(seed);

    SimulationResult result{};
    result.seed = seed;
    while (!environment.isFinished()) {
        const Observation observation = environment.state();
        const Action action = policy.selectAction(observation, observation.availableActions);
        result.actions.push_back(action);
        const StepResult step = environment.step(action);
        result.reason = step.reason;
    }

    const Observation last = environment.state();
    result.turns = last.turn;
    result.energy = last.energy;
    result.score = last.score;
    result.collectedResources = last.collectedResources;
    return result;
}