#pragma once
#include <algorithm>
#include <cstddef>
#include <iterator>
#include <string>
#include <variant>

#include "overloaded.hpp"
#include "position.hpp"

// --- EVENTOS (enunciado 5.7 y 6.4) ---
// Un evento describe algo que YA ocurrió durante un step. El entorno los produce
// en el mismo orden en que ocurrieron los cambios; la UI y las pruebas solo los leen.
struct MovedEvent { Position from; Position to; int energyCost; };
struct MovementRejectedEvent { Position from; Action action; };
struct ResourceCollectedEvent { Position at; int points; };
struct EnergyChangedEvent { int previous; int current; };
struct TrapTriggeredEvent { Position at; };
struct GoalReachedEvent { Position at; };

// std::variant: un NavigationEvent guarda exactamente UNO de estos tipos.
// Así se evita una jerarquía con herencia y el compilador conoce todos los casos posibles.
using NavigationEvent = std::variant<
    MovedEvent,
    MovementRejectedEvent,
    ResourceCollectedEvent,
    EnergyChangedEvent,
    TrapTriggeredEvent,
    GoalReachedEvent>;

// Nombre legible de una acción (para diagnósticos y para describe)
inline std::string toString(Action action) {
    switch (action) {
    case Action::up:    return "arriba";
    case Action::down:  return "abajo";
    case Action::left:  return "izquierda";
    case Action::right: return "derecha";
    case Action::wait:  return "esperar";
    }
    return "?";
}

// --- TEMPLATE VARIÁDICO + FOLD EXPRESSION ---
// ¿El evento es de ALGUNO de los tipos indicados?
//   holdsAnyOf<TrapTriggeredEvent, GoalReachedEvent>(event)
// se expande a: holds_alternative<TrapTriggeredEvent>(event) || holds_alternative<GoalReachedEvent>(event)
template <typename... Events>
[[nodiscard]] constexpr bool holdsAnyOf(const NavigationEvent& event) noexcept {
    static_assert(sizeof...(Events) > 0, "holdsAnyOf necesita al menos un tipo de evento");
    return (std::holds_alternative<Events>(event) || ...);
}

// --- TEMPLATE DE FUNCIÓN SOBRE UN RANGO (iteradores) ---
// Cuenta cuántos eventos del rango [first, last) son de alguno de los tipos indicados.
// Funciona con vector, list, deque, span... sin duplicar el algoritmo.
//   countEvents<MovedEvent>(events.begin(), events.end())
template <typename... Events, std::input_iterator Iterator, std::sentinel_for<Iterator> Sentinel>
[[nodiscard]] std::size_t countEvents(Iterator first, Sentinel last) {
    return static_cast<std::size_t>(std::count_if(first, last, [](const NavigationEvent& event) {
        return holdsAnyOf<Events...>(event);
    }));
}

// --- std::visit + Overloaded ---
// Convierte cualquier evento en un texto corto (pensado para el pie de la interfaz de consola:
// junto con la ayuda breve debe caber en una terminal de 80 columnas).
// Si se agrega un tipo nuevo al variant y no se maneja aquí, el programa NO compila:
// así ningún evento queda sin describir.
[[nodiscard]] inline std::string describe(const NavigationEvent& event) {
    return std::visit(Overloaded{
        [](const MovedEvent& e) -> std::string {
            return "Mueve " + toString(e.from) + " -> " + toString(e.to) +
                   " costo " + std::to_string(e.energyCost);
        },
        [](const MovementRejectedEvent& e) -> std::string {
            return "Rechazado: " + toString(e.action) + " en " + toString(e.from);
        },
        [](const ResourceCollectedEvent& e) -> std::string {
            return "Recurso +" + std::to_string(e.points) + " en " + toString(e.at);
        },
        [](const EnergyChangedEvent& e) -> std::string {
            return "Energia " + std::to_string(e.previous) + " -> " + std::to_string(e.current);
        },
        [](const TrapTriggeredEvent& e) -> std::string {
            return "Trampa en " + toString(e.at);
        },
        [](const GoalReachedEvent& e) -> std::string {
            return "Salida en " + toString(e.at);
        },
    }, event);
}
