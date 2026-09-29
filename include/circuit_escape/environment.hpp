#pragma once
#include <cstdint>
#include <stdexcept>
#include <vector>
#include "cells.hpp"
#include "game_rules.hpp"
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
    int initialEnergy; // Atributo original segun el informe

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


    NavigationEnvironment(Grid<Cell, Rows, Columns> initialGrid, Position start, int initialEnergy, std::size_t turnLimit)
        : initialGrid(initialGrid), start(start), initialEnergy(initialEnergy), turnLimit(turnLimit) {}     // Constructor original

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

            std::optional<Position> optionalPosition{neighbor(start, action)};  // Cambiar start por la posicion del agente

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

    [[nodiscard]] bool isFinished() const noexcept {}

    [[nodiscard]] StepResult step(Action action) {}

    [[nodiscard]] const Grid<Cell, Rows, Columns>& grid() const noexcept {}

};
