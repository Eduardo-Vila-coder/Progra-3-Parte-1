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
};

// Los eventos (MovedEvent, ..., NavigationEvent) viven en events.hpp

enum class EndReason { none, goalReached, noEnergy, turnLimit };

struct StepResult {
    Observation observation;
    std::vector<NavigationEvent> events;
    bool finished{false};
    EndReason reason{EndReason::none};
};