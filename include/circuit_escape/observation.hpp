#pragma once

#include <variant>
#include <vector>

#include "events.hpp"
#include "position.hpp"

struct Observation {
    Position agent;
    Position goal;
    int energy{};
    int maximumEnergy{};
    int score{};
    std::size_t collectedResources{};
    std::size_t turn{};
    std::vector<Action> availableActions;
    // --- Proyecto 2: interruptores ---
    bool hasSwitches{false};   // el mapa tiene los interruptores A y B
    bool switchAActive{false}; // A ya fue activado
    bool switchBActive{false}; // B ya fue activado
    // --- Proyecto 2: vigilante ---
    bool hasGuard{false};           // la partida tiene vigilante
    Position guard{};               // posición actual del vigilante
    bool guardMovesNextTurn{false}; // el próximo turno es par: el vigilante avanzará
    // --- Proyecto 2: mapa de solo lectura (enunciado, sección 4) ---
    // [Se usó IA para esta parte] Es una COPIA del tablero: quien la lee (controladores, ayuda H)
    // no puede modificar la partida y la copia sigue válida aunque el entorno ya no exista.
    std::size_t rows{};    // filas del tablero
    std::size_t columns{}; // columnas del tablero
    std::vector<Cell> map; // celdas por filas; incluye si cada interruptor está activado

    // Celda de una posición del mapa (fila * columnas + columna). Lanza out_of_range si no existe.
    [[nodiscard]] const Cell& cellAt(Position position) const {
        return map.at(position.row * columns + position.column);
    }
};

// Los eventos (MovedEvent, ..., NavigationEvent) viven en events.hpp

enum class EndReason { none, goalReached, noEnergy, turnLimit, captured }; // captured: proyecto 2

struct StepResult {
    Observation observation;
    std::vector<NavigationEvent> events;
    bool finished{false};
    EndReason reason{EndReason::none};
};