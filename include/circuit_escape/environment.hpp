#pragma once
#include <stdexcept>
#include "position.hpp"
#include "cells.hpp"
#include "grid.hpp"

template <size_t Rows, size_t Columns>
class NavigationEnvironment {
private:
    Grid<Cell, Rows, Columns> grid_;

    // --- 5.3 AGENTE: Atributos mínimos exigidos ---
    Position agent_;
    int energy_{};
    int maxEnergy_{};
    int score_{0};
    std::size_t collectedResources_{0};
    bool active_{true};

public:
    // Constructor: Configura el entorno y verifica las invariantes
    NavigationEnvironment(Grid<Cell, Rows, Columns> initialGrid, Position startPos, int startEnergy)
        : grid_(initialGrid), agent_(startPos), energy_(startEnergy), maxEnergy_(startEnergy) {

        // INVARIANTE: La posición del agente siempre pertenece al tablero
        if (!grid_.contains(agent_)) {
            throw std::invalid_argument("Error: El agente inicia fuera del tablero");
        }
        
        // INVARIANTE: El agente nunca ocupa una celda bloqueada
        // std::holds_alternative verifica si la celda actual es un Muro (Wall)
        if (std::holds_alternative<Wall>(grid_.at(agent_))) {
            throw std::invalid_argument("Error: El agente inicia dentro de un muro");
        }
    }

    // Métodos para consultar el estado del agente
    Position getAgentPosition() const { return agent_; }
    int getEnergy() const { return energy_; }
    bool isActive() const { return active_; }
};