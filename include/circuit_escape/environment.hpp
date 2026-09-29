#pragma once
#include <stdexcept>
#include <vector>
#include "cells.hpp"
#include "grid.hpp"
#include "observation.hpp"
#include "position.hpp"

template <std::size_t Rows, std::size_t Columns>
class NavigationEnvironment {
private:
    Grid<Cell, Rows, Columns>  initialGrid;

    // --- 5.3 AGENTE: Atributos mínimos exigidos ---
    Position agent_;
    int energy_{};
    int maxEnergy_{};
    int score_{0};
    std::size_t collectedResources_{0};
    bool active_{true};
    std::size_t turnLimit;

    Position start; // Atributo original segun el informe
    int turn_{0};

    // INVARIANTE: La energía se mantiene entre el Mín y Máx
    void changeEnergy(int amount) {
        energy_ += amount;

        if (energy_ < 0) {
            energy_ = 0;
        } else if (energy_ > maxEnergy_) {
            energy_ = maxEnergy_;
        }
    }

    // Calculamos la posición destino. Retorna nullopt si el movimiento choca o se sale.
    // [Se usó IA para la lógica]
    std::optional<Position> targetOf(Action action) const {
        std::optional<Position> candidate = neighbor(agent_, action);

        // Si neighbor falló, o si se sale del tablero:
        if (!candidate.has_value() || !grid_.contains(*candidate)) {
            return std::nullopt;
        }

        // Si la celda destino es un muro (obstáculo):
        if (std::holds_alternative<Wall>(grid_.at(*candidate))) {
            return std::nullopt;
        }

        return candidate; // El movimiento es válido
    }

public:
    // Constructor: Configura el entorno y verifica las invariantes
    NavigationEnvironment(Grid<Cell, Rows, Columns> initialGrid, Position start, Position startPos, int startEnergy, std::size_t turnLimit)
        :  initialGrid(initialGrid), start(start), agent_(startPos), energy_(startEnergy), maxEnergy_(startEnergy), turnLimit(turnLimit) {

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

    // Métodos para consultar el estado del agente
    Position getAgentPosition() const { return agent_; }
    int getEnergy() const { return energy_; }
    bool isActive() const { return active_; }


    // Metodos descritos en el informe

    void reset(std::uint32_t seed) {}

    [[nodiscard]] Observation state() const {}

    [[nodiscard]] std::vector<Action> availableActions() const {
        std::vector<Action> actions;

        // Iteramos sobre cada accion existente [se usó IA para saber cómo iterar sobre Enum]
        for (std::size_t i = 0; i <= 4; i++) {
            Action action = static_cast<Action>(i);

            std::optional<Position> position{neighbor(start, action)};

            // Ignoramos la accion si esta produce una posicion con indices negativos
            if (!position.has_value()) {
                continue;
            }

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

    [[nodiscard]] bool isFinished() const noexcept {}

    [[nodiscard]] StepResult step(Action action) {
        if (!active_) {
            throw std::logic_error("Error: la partida ya termino");
        }

        // INVARIANTE: Un agente sin energía no puede ejecutar otra acción
        if (energy_ == 0) {
            active_ = false; // Se desactiva el agente
            return;          // Sale de la función sin ejecutar el turno
        }

        //Incrementamos el turno
        turn_++;

        //Validamos la acción
        if (action == Action::wait) {
            // Si espera, no se mueve. Solo gasta energía.
            // (Eduardo pon aquí las GameRules)
            changeEnergy(-1);

        } else {
            std::optional<Position> target = targetOf(action);

            if (target.has_value()) {
                // Movimiento válido
                agent_ = *target; // Actualiza la posición
                changeEnergy(-1);     // Descuenta el costo de moverse

                //CRISTHIAN AÑADIRÁ SU CÓDIGO DE EFECTOS DE CELDAS (applyCellEffect)

            } else {
                // MOVIMIENTO RECHAZADO (chocó contra muro o borde)
                changeEnergy(-1); // Gasta energía por el intento fallido
            }
        }

        // Protege la invariante de energía (no puede bajar de cero)
        if (energy_ < 0) {
            energy_ = 0;
        }

        //MATHIAS AÑADIRÁ SU CÓDIGO DE CONDICIONES DE TÉRMINO (checkEnd)
    }

    [[nodiscard]] const Grid<Cell, Rows, Columns>& grid() const noexcept {}

};
