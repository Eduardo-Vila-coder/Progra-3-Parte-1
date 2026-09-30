#pragma once
#include <algorithm>
#include <cstdint>
#include <iterator>
#include <optional>
#include <stdexcept>
#include <vector>
#include "cells.hpp"
#include "game_rules.hpp"
#include "grid.hpp"
#include "observation.hpp"
#include "overloaded.hpp"
#include "position.hpp"

template <std::size_t Rows, std::size_t Columns>
class NavigationEnvironment {
private:
    // Atributos originales segun el informe
    Grid<Cell, Rows, Columns>  initialGrid;
    Position start;
    int initialEnergy;
    std::size_t turnLimit;
    Grid<Cell, Rows, Columns>  originalGrid_; // Copia del tablero tal como empezó (para reset)

    // --- 5.3 AGENTE: Atributos mínimos exigidos ---
    Position agent_;
    int energy_{};
    int maxEnergy_{};
    int score_{0};
    std::size_t collectedResources_{0};
    bool active_{true};
    std::uint32_t seed_{0}; // Semilla del último reset (simulación reproducible)

    std::size_t turn_{0};

    // --- CONDICIONES DE TÉRMINO ---
    // Motivo por el que terminó la partida (none = sigue activa)
    EndReason endReason_{EndReason::none};

    // Evalúa las condiciones de término respetando la precedencia del enunciado:
    // goalReached (en la salida y con energía) > noEnergy > turnLimit
    EndReason evaluateEnd() const {
        if (std::holds_alternative<Exit>(initialGrid.at(agent_)) && energy_ > 0) {
            return EndReason::goalReached;
        }
        if (energy_ == 0) {
            return EndReason::noEnergy;
        }
        if (turn_ >= turnLimit) {
            return EndReason::turnLimit;
        }
        return EndReason::none;
    }

    // Se llama al final de step, después de aplicar los efectos de la celda destino
    void checkEnd(std::vector<NavigationEvent>& events) {
        endReason_ = evaluateEnd();
        if (endReason_ == EndReason::none) {
            return;
        }

        active_ = false; // La partida terminó: el agente ya no puede actuar

        if (endReason_ == EndReason::goalReached) {
            events.emplace_back(GoalReachedEvent{agent_});
        }
    }

    // Busca la posición de la salida recorriendo el tablero en orden por filas
    Position goalPosition() const {
        auto it = std::find_if(initialGrid.begin(), initialGrid.end(), [](const Cell& cell) {
            return std::holds_alternative<Exit>(cell);
        });
        std::size_t index = static_cast<std::size_t>(std::distance(initialGrid.begin(), it));
        return Position{index / Columns, index % Columns};
    }

    // --- ENERGÍA E INTERACCIONES (Cristhian) ---
    // Reglas de la partida (costos, recompensas, penalizaciones). Por ahora estándar.
    GameRules<> rules_{};

    // INVARIANTE: La energía se mantiene entre 0 y maxEnergy_.
    // Solo se emite EnergyChangedEvent si la energía cambió de verdad.
    void changeEnergy(int delta, std::vector<NavigationEvent>& events) {
        const int previous = energy_;
        energy_ = std::clamp(energy_ + delta, 0, maxEnergy_);
        if (energy_ != previous) {
            appendEvents(events, EnergyChangedEvent{previous, energy_});
        }
    }

    // Gastar energía es cambiarla en negativo
    void spendEnergy(int cost, std::vector<NavigationEvent>& events) {
        changeEnergy(-cost, events);
    }

    // Costo de entrar a una celda: el terreno elevado cuesta más, el resto cuesta lo general
    int entryCost(const Cell& cell) const {
        return std::visit(Overloaded{
            [this](const RoughTerrain&) { return rules_.energyCostRoughTerrain; },
            [this](const auto&) { return rules_.energyCostGeneral; },
        }, cell);
    }

    // Efecto de la celda a la que acaba de entrar el agente.
    // Se aplica aunque la energía haya quedado en 0 (una batería todavía puede recargar).
    void applyCellEffect(std::vector<NavigationEvent>& events) {
        std::visit(Overloaded{
            [](Empty&) {},
            [](Wall&) {},          // nunca se entra a un muro
            [](RoughTerrain&) {},  // su costo ya se cobró al entrar
            [](Exit&) {},          // la victoria la decide checkEnd
            [this, &events](ResourceCell<int>& resource) {
                if (resource.collected) return;          // se recoge una sola vez
                resource.collected = true;
                score_ += rules_.reward;
                ++collectedResources_;
                appendEvents(events, ResourceCollectedEvent{agent_, rules_.reward});
            },
            [this, &events](Battery& battery) {
                if (battery.consumed) return;            // se consume una sola vez
                battery.consumed = true;
                changeEnergy(rules_.energy, events);
            },
            [this, &events](Trap&) {                     // se activa CADA vez que se entra
                appendEvents(events, TrapTriggeredEvent{agent_});
                changeEnergy(-rules_.energyPenalty, events);
                score_ -= rules_.scorePenalty;           // el puntaje puede ser negativo
            },
        }, initialGrid.at(agent_));
    }

    // Calculamos la posición destino. Retorna nullopt si el movimiento choca o se sale.
    // [Se usó IA para la lógica]
    std::optional<Position> targetOf(Action action) const {
        std::optional<Position> candidate = neighbor(agent_, action);

        // Si neighbor falló, o si se sale del tablero:
        if (!candidate.has_value() || !initialGrid.contains(*candidate)) {
            return std::nullopt;
        }

        // Si la celda destino es un muro (obstáculo):
        if (std::holds_alternative<Wall>(initialGrid.at(*candidate))) {
            return std::nullopt;
        }

        return candidate; // El movimiento es válido
    }

public:
    // Constructor: Configura el entorno y verifica las invariantes
    NavigationEnvironment(Grid<Cell, Rows, Columns> initialGrid, Position start, Position startPos, int startEnergy, std::size_t turnLimit)
        :  initialGrid(initialGrid), originalGrid_(initialGrid), start(start), agent_(startPos), energy_(startEnergy), maxEnergy_(startEnergy), turnLimit(turnLimit) {

        // INVARIANTE: La posición del agente siempre pertenece al tablero
        if (! initialGrid.contains(agent_)) {
            throw std::invalid_argument("Error: El agente inicia fuera del tablero");
        }
        
        // INVARIANTE: El agente nunca ocupa una celda bloqueada
        // std::holds_alternative verifica si la celda actual es un Muro (Wall)
        if (std::holds_alternative<Wall>(initialGrid.at(agent_))) {
            throw std::invalid_argument("Error: El agente inicia dentro de un muro");
        }
    
    }
    // Constructor que recibe cualquier perfil de Eduardo: GameRules<Easy>, <Standard> o <Hard>.
    // Toma del perfil la energía inicial/máxima y el límite de turnos, y copia sus costos y recompensas.
    template <Difficulty Level>
    NavigationEnvironment(Grid<Cell, Rows, Columns> initialGrid, Position start, GameRules<Level> rules)
        : NavigationEnvironment(initialGrid, start, rules.initialMaxEnergy, rules.turnLimit) {
        rules_ = {rules.initialMaxEnergy, rules.turnLimit, rules.energyCostGeneral,
                  rules.energyCostRoughTerrain, rules.energyCostOthers, rules.reward,
                  rules.energy, rules.energyPenalty, rules.scorePenalty};
    }

    // Métodos para consultar el estado del agente
    Position getAgentPosition() const { return agent_; }
    int getEnergy() const { return energy_; }
    bool isActive() const { return active_; }
    int getScore() const { return score_; }
    std::size_t getCollectedResources() const { return collectedResources_; }


    // Metodos descritos en el informe

    // Vuelve la partida a su estado inicial: tablero original (recursos y baterías disponibles otra vez),
    // agente en el inicio, energía máxima, puntaje, recursos y turno en cero. Las reglas no cambian.
    // La semilla se guarda para que la simulación sepa con qué semilla se reinició.
    void reset(std::uint32_t seed) {
        seed_ = seed;
        initialGrid = originalGrid_;
        agent_ = start;
        energy_ = maxEnergy_;
        score_ = 0;
        collectedResources_ = 0;
        turn_ = 0;
        active_ = true;
        endReason_ = EndReason::none;
    }

    [[nodiscard]] std::uint32_t seed() const noexcept { return seed_; }

    [[nodiscard]] Observation state() const {
        return Observation{agent_, goalPosition(), energy_, maxEnergy_, score_,
                           collectedResources_, turn_, availableActions()};
    }

    [[nodiscard]] std::vector<Action> availableActions() const {
        std::vector<Action> actions;

        // Después del término no hay acciones legales
        if (isFinished()) {
            return actions;
        }

        // Iteramos sobre cada accion existente [se usó IA para saber cómo iterar sobre Enum]
        for (std::size_t i = 0; i <= 4; i++) {
            Action action = static_cast<Action>(i);

            std::optional<Position> optionalPosition{neighbor(agent_, action)};

            // Ignoramos la accion si esta produce una posicion con indices negativos
            if (!optionalPosition.has_value()) {
                continue;
            }

            // Como sabemos que si hay una posicion, se la asignamos a una variable no opcional
            Position position(optionalPosition.value());

            // Ignoramos la accion si esta produce una posicion que no se encuentre dentro del tablero
            if (!initialGrid.contains(position)) {
                continue;
            }

            // Ignoramos la accion si esta produce una posicion que traspase un muro
            // [Se usó IA para saber cómo identificar si el CellType es Wall]
            if (std::holds_alternative<Wall>(initialGrid.at(position))) {
                continue;
            }

            // Ya hemos verificado que se trate de una accion valida, la aniadimos al vector de acciones validas
            actions.emplace_back(action);
        }

        return actions;
    }

    [[nodiscard]] bool isFinished() const noexcept {
        return endReason_ != EndReason::none;
    }

    [[nodiscard]] StepResult step(Action action) {
        // INVARIANTE: Un agente sin energía no puede ejecutar otra acción
        // (con energía 0 la partida termina por noEnergy y se lanza esta excepción)
        if (!active_) {
            throw std::logic_error("Error: la partida ya termino");
        }

        std::vector<NavigationEvent> events;

        //Incrementamos el turno
        turn_++;

        //Validamos la acción
        if (action == Action::wait) {
            // Esperar no mueve al agente ni activa la celda donde está. Solo gasta energía.
            spendEnergy(rules_.energyCostOthers, events);

        } else {
            std::optional<Position> target = targetOf(action);

            if (target.has_value()) {
                // Movimiento válido: se mueve, paga la entrada y se aplica el efecto de la celda
                const Position from = agent_;
                agent_ = *target;
                const int cost = entryCost(initialGrid.at(agent_));
                appendEvents(events, MovedEvent{from, agent_, cost});
                spendEnergy(cost, events);
                applyCellEffect(events);

            } else {
                // MOVIMIENTO RECHAZADO (chocó contra muro o borde): no se mueve, pero gasta energía
                appendEvents(events, MovementRejectedEvent{agent_, action});
                spendEnergy(rules_.energyCostOthers, events);
            }
        }

        // Comprobamos las condiciones de término (último paso de la resolución)
        checkEnd(events);

        return StepResult{state(), std::move(events), isFinished(), endReason_};
    }

    [[nodiscard]] const Grid<Cell, Rows, Columns>& grid() const noexcept {
        return initialGrid;
    }

};

// Crea el entorno con la dificultad elegida al ejecutar (por ejemplo, desde --difficulty en la consola).
// Es el único lugar donde se decide según el nivel; el entorno solo recibe las reglas.
template <std::size_t Rows, std::size_t Columns>
NavigationEnvironment<Rows, Columns> makeEnvironment(Grid<Cell, Rows, Columns> grid, Position start,
                                                     Difficulty difficulty) {
    switch (difficulty) {
    case Difficulty::Easy: return NavigationEnvironment<Rows, Columns>{grid, start, GameRules<Difficulty::Easy>{}};
    case Difficulty::Hard: return NavigationEnvironment<Rows, Columns>{grid, start, GameRules<Difficulty::Hard>{}};
    default:               return NavigationEnvironment<Rows, Columns>{grid, start, GameRules<Difficulty::Standard>{}};
    }
}
