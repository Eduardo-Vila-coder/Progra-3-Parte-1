#pragma once
#include <concepts>
#include <cstddef>
#include <span>
#include <utility>
#include "circuit_escape/graph/graph.hpp"
#include "circuit_escape/pathfinding/bfs.hpp"
#include "circuit_escape/position.hpp"

// --- CONTROLADOR DEL VIGILANTE (proyecto 2, secciones 3.1, 3.2 y 4) ---
// Es la adaptación del contrato de decisiones del proyecto 1 (concept + adaptador + interfaz)
// al vigilante: recibe su posición, la del jugador y sus movimientos legales.
// El controlador solo PROPONE la acción; NavigationEnvironment la valida y la ejecuta.

// Lo que el vigilante puede observar (copia de solo lectura)
struct GuardObservation
{
    Position guard;        // posición propia
    Position player;       // posición actual del jugador
    std::size_t turn{};    // turno en curso (el vigilante avanza en los pares)
};

// Contrato que debe cumplir una política de vigilante (se verifica al compilar)
template <typename Policy>
concept GuardPolicy = requires(Policy& policy, const GuardObservation& observation,
                               std::span<const Action> legalActions) {
    { policy.selectAction(observation, legalActions) } -> std::same_as<Action>;
};

// Interfaz polimórfica: el entorno guarda un controlador de vigilante sin conocer su tipo
class IGuardController
{
public:
    virtual ~IGuardController() = default;
    virtual Action selectAction(const GuardObservation& observation, std::span<const Action> legalActions) = 0;
};

// Adaptador genérico: convierte cualquier GuardPolicy en un IGuardController
template <GuardPolicy Policy>
class GuardPolicyController final : public IGuardController
{
public:
    explicit GuardPolicyController(Policy policy) : policy_(std::move(policy)) {}

    Action selectAction(const GuardObservation& observation, std::span<const Action> legalActions) override
    {
        return policy_.selectAction(observation, legalActions);
    }

private:
    Policy policy_;
};

// Acción que lleva de una casilla a otra vecina (wait si no son vecinas). Implementada en
// src/controllers/guard_controller.cpp.
[[nodiscard]] Action actionTowards(Position from, Position to);

// Política de persecución: cada vez que decide, recalcula con BFS el camino más corto
// (en movimientos) hacia la posición ACTUAL del jugador y propone el primer paso.
// - Respeta muros y bordes porque el grafo solo conecta casillas transitables.
// - Desempate fijo: el orden de vecinos de MapGraph.
// - Si no hay camino, o si el paso no es legal, espera.
template <std::size_t Rows, std::size_t Columns>
class BfsGuardPolicy
{
public:
    // Guarda su propia copia del grafo: los muros no cambian durante la partida
    explicit BfsGuardPolicy(MapGraph<Rows, Columns> graph) : graph_(std::move(graph)) {}

    Action selectAction(const GuardObservation& observation, std::span<const Action> legalActions)
    {
        const SearchResult route = bfs(graph_, toIndex(observation.guard, Columns),
                                       toIndex(observation.player, Columns));
        if (!route.found || route.path.size() < 2) {
            return Action::wait; // sin camino (o ya está sobre el jugador): espera
        }
        const Action step = actionTowards(observation.guard, toPosition(route.path[1], Columns));
        for (const Action legal : legalActions) {
            if (legal == step) {
                return step;
            }
        }
        return Action::wait;
    }

private:
    MapGraph<Rows, Columns> graph_;
};
