//
// Created by LucasMCgamer on 13/09/2026.
//

#include "../include/circuit_escape/controllers.hpp"

#include <stdexcept>
#include <vector>

std::size_t manhattanDistance(Position a, Position b) {
    const std::size_t rowDistance = a.row > b.row ? a.row - b.row : b.row - a.row;
    const std::size_t columnDistance = a.column > b.column ? a.column - b.column : b.column - a.column;
    return rowDistance + columnDistance;
}

// ===== RandomPolicy =====

RandomPolicy::RandomPolicy(std::uint32_t seed) : engine_(seed) {}

Action RandomPolicy::selectAction(const Observation&, std::span<const Action> legalActions) {
    // Sin acciones legales la partida ya terminó: pedir una acción es un error de quien llama
    if (legalActions.empty()) {
        throw std::invalid_argument("RandomPolicy: no hay acciones legales");
    }

    // Índice al azar entre 0 y size-1, usando el generador con semilla
    std::uniform_int_distribution<std::size_t> distribution(0, legalActions.size() - 1);
    return legalActions[distribution(engine_)];
}

// ===== HeuristicPolicy =====

Action HeuristicPolicy::selectAction(const Observation& observation, std::span<const Action> legalActions) {
    if (legalActions.empty()) {
        throw std::invalid_argument("HeuristicPolicy: no hay acciones legales");
    }

    // 1. Separamos los movimientos (todo menos wait)
    std::vector<Action> moves;
    for (Action action : legalActions) {
        if (action != Action::wait) {
            moves.push_back(action);
        }
    }

    // Si wait es lo único legal, no hay otra opción
    if (moves.empty()) {
        return Action::wait;
    }

    // Casilla a la que lleva un movimiento legal (siempre existe porque la acción es legal)
    auto targetOf = [&observation](Action action) {
        return neighbor(observation.agent, action).value();
    };

    // 2. Quitamos el movimiento que regresa a la casilla anterior (no retroceder)
    std::vector<Action> forward;
    for (Action action : moves) {
        if (!previous_.has_value() || targetOf(action) != *previous_) {
            forward.push_back(action);
        }
    }

    // Si el único movimiento posible es retroceder (callejón sin salida), se permite
    if (forward.empty()) {
        forward = moves;
    }

    // 3. Elegimos el movimiento que deja al agente más cerca de la salida
    auto best = bestBy(forward.begin(), forward.end(), [&](Action action) {
        return manhattanDistance(targetOf(action), observation.goal);
    });

    previous_ = observation.agent; // recordamos de dónde salimos
    return *best;
}

// ===== HumanController =====

Action HumanController::selectAction(const Observation&, std::span<const Action>) {
    if (!pending_.has_value()) {
        throw std::logic_error("HumanController: la interfaz no entrego ninguna decision");
    }
    const Action action = *pending_;
    pending_.reset(); // cada decisión se usa una sola vez
    return action;
}

// ===== Fábrica =====

std::unique_ptr<IController> makeController(ControllerKind kind, std::uint32_t seed) {
    switch (kind) {
    case ControllerKind::random:
        return std::make_unique<PolicyController<RandomPolicy>>(RandomPolicy{seed});
    case ControllerKind::heuristic:
        return std::make_unique<PolicyController<HeuristicPolicy>>(HeuristicPolicy{});
    }
    throw std::invalid_argument("makeController: tipo de controlador desconocido");
}

std::optional<ControllerKind> controllerKindFrom(std::string_view name) {
    if (name == "random") return ControllerKind::random;
    if (name == "heuristic") return ControllerKind::heuristic;
    return std::nullopt;
}
