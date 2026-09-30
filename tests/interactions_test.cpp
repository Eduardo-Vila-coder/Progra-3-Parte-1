// Pruebas de energía e interacciones con celdas (Cristhian)
// Se usan las reglas estándar: costo general 1, terreno elevado 2, esperar/rechazo 1,
// recurso +10, batería +3, trampa -2 energía y -1 punto.

#include <cassert>
#include <iostream>
#include <variant>
#include <vector>

#include "circuit_escape/environment.hpp"
#include "circuit_escape/overloaded.hpp"

using Env = NavigationEnvironment<3, 4>;

int main() {
    const Position start{1, 1};

    // 1. Moverse a una celda vacía cuesta 1 y emite MovedEvent + EnergyChangedEvent
    {
        Grid<Cell, 3, 4> g{};
        g.at({2, 3}) = Exit{};  // el entorno exige exactamente una salida
        Env e{g, start, 10, 100};
        StepResult r = e.step(Action::right);
        assert(e.getEnergy() == 9);
        assert(r.events.size() == 2);
        assert(std::holds_alternative<MovedEvent>(r.events[0]));
        assert(std::get<MovedEvent>(r.events[0]).energyCost == 1);
        assert(std::holds_alternative<EnergyChangedEvent>(r.events[1]));
    }

    // 2. Entrar a terreno elevado cuesta 2
    {
        Grid<Cell, 3, 4> g{};
        g.at({2, 3}) = Exit{};  // el entorno exige exactamente una salida
        g.at({1, 2}) = RoughTerrain{};
        Env e{g, start, 10, 100};
        StepResult r = e.step(Action::right);
        assert(e.getEnergy() == 8);
        assert(std::get<MovedEvent>(r.events[0]).energyCost == 2);
    }

    // 3. Esperar cuesta 1 y NO activa la celda donde está el agente
    {
        Grid<Cell, 3, 4> g{};
        g.at({2, 3}) = Exit{};  // el entorno exige exactamente una salida
        g.at(start) = Trap{};
        Env e{g, start, 10, 100};
        StepResult r = e.step(Action::wait);
        assert(e.getEnergy() == 9);   // solo el costo de esperar, sin penalización de trampa
        assert(e.getScore() == 0);
        assert(r.events.size() == 1);
        assert(std::holds_alternative<EnergyChangedEvent>(r.events[0]));
    }

    // 4. Movimiento rechazado (muro): no se mueve, cuesta 1, primero MovementRejected
    {
        Grid<Cell, 3, 4> g{};
        g.at({2, 3}) = Exit{};  // el entorno exige exactamente una salida
        g.at({1, 2}) = Wall{};
        Env e{g, start, 10, 100};
        StepResult r = e.step(Action::right);
        assert(e.getAgentPosition() == start);
        assert(e.getEnergy() == 9);
        assert(std::holds_alternative<MovementRejectedEvent>(r.events[0]));
        assert(std::holds_alternative<EnergyChangedEvent>(r.events[1]));
    }

    // 5. Movimiento rechazado (borde del tablero)
    {
        Grid<Cell, 3, 4> g{};
        g.at({2, 3}) = Exit{};  // el entorno exige exactamente una salida
        Env e{g, Position{0, 0}, 10, 100};
        StepResult r = e.step(Action::up);
        assert(e.getAgentPosition() == Position(0, 0));
        assert(std::holds_alternative<MovementRejectedEvent>(r.events[0]));
    }

    // 6. El recurso se recoge UNA sola vez
    {
        Grid<Cell, 3, 4> g{};
        g.at({2, 3}) = Exit{};  // el entorno exige exactamente una salida
        g.at({1, 2}) = ResourceCell<int>{};
        Env e{g, start, 10, 100};
        StepResult r = e.step(Action::right);
        assert(e.getScore() == 10);
        assert(e.getCollectedResources() == 1);
        assert(std::holds_alternative<ResourceCollectedEvent>(r.events.back()));
        assert(std::get<ResourceCollectedEvent>(r.events.back()).points == 10);

        (void)e.step(Action::left);
        (void)e.step(Action::right);          // vuelve a entrar: ya no da puntos
        assert(e.getScore() == 10);
        assert(e.getCollectedResources() == 1);
    }

    // 7. La batería recarga, pero nunca por encima del máximo
    {
        Grid<Cell, 3, 4> g{};
        g.at({2, 3}) = Exit{};  // el entorno exige exactamente una salida
        g.at({1, 2}) = Battery{};
        Env e{g, start, 10, 100};             // máximo = 10
        (void)e.step(Action::right);          // 10 - 1 + 3 -> se limita a 10
        assert(e.getEnergy() == 10);
    }

    // 8. La batería se aplica aunque el costo de entrada deje la energía en 0
    {
        Grid<Cell, 3, 4> g{};
        g.at({2, 3}) = Exit{};  // el entorno exige exactamente una salida
        g.at({1, 2}) = Battery{};
        Env e{g, start, 1, 100};              // máximo = 1
        StepResult r = e.step(Action::right); // 1 - 1 = 0, luego +3 -> se limita a 1
        assert(e.getEnergy() == 1);
        assert(!r.finished);                  // no termina por noEnergy: la batería lo salvó
        assert(r.events.size() == 3);         // Moved, EnergyChanged(1->0), EnergyChanged(0->1)
        assert(std::get<EnergyChangedEvent>(r.events[1]).current == 0);
        assert(std::get<EnergyChangedEvent>(r.events[2]).previous == 0);
    }

    // 9. La batería se consume UNA sola vez
    {
        Grid<Cell, 3, 4> g{};
        g.at({2, 3}) = Exit{};  // el entorno exige exactamente una salida
        g.at({1, 2}) = Battery{};
        Env e{g, start, 20, 100};
        (void)e.step(Action::wait);           // 19
        (void)e.step(Action::wait);           // 18
        (void)e.step(Action::right);          // 17 + 3 = 20
        assert(e.getEnergy() == 20);
        (void)e.step(Action::left);           // 19
        (void)e.step(Action::right);          // 18, sin recarga
        assert(e.getEnergy() == 18);
    }

    // 10. La trampa se activa CADA vez y aplica ambas penalizaciones
    {
        Grid<Cell, 3, 4> g{};
        g.at({2, 3}) = Exit{};  // el entorno exige exactamente una salida
        g.at({1, 2}) = Trap{};
        Env e{g, start, 20, 100};
        StepResult r = e.step(Action::right); // 20 - 1 - 2 = 17, score -1
        assert(e.getEnergy() == 17);
        assert(e.getScore() == -1);
        assert(std::holds_alternative<TrapTriggeredEvent>(r.events[2]));

        (void)e.step(Action::left);           // 16
        (void)e.step(Action::right);          // 16 - 1 - 2 = 13, score -2
        assert(e.getEnergy() == 13);
        assert(e.getScore() == -2);           // el puntaje puede ser negativo
    }

    // 11. La energía nunca baja de 0 (trampa con poca energía)
    {
        Grid<Cell, 3, 4> g{};
        g.at({2, 3}) = Exit{};  // el entorno exige exactamente una salida
        g.at({1, 2}) = Trap{};
        Env e{g, start, 2, 100};
        StepResult r = e.step(Action::right); // 2 - 1 - 2 -> 0, no -1
        assert(e.getEnergy() == 0);
        assert(r.finished);
        assert(r.reason == EndReason::noEnergy);
    }

    // 12. appendEvents agrega en orden, y con cero argumentos no agrega nada
    {
        std::vector<int> v;
        appendEvents(v, 1, 2, 3);
        assert((v == std::vector<int>{1, 2, 3}));
        appendEvents(v);
        assert(v.size() == 3);
    }

    // 13. Los eventos se pueden procesar con std::visit + Overloaded
    {
        Grid<Cell, 3, 4> g{};
        g.at({2, 3}) = Exit{};  // el entorno exige exactamente una salida
        g.at({1, 2}) = ResourceCell<int>{};
        Env e{g, start, 10, 100};
        StepResult r = e.step(Action::right);
        int moved = 0, energy = 0, resource = 0;
        for (const NavigationEvent& event : r.events) {
            std::visit(Overloaded{
                [&](const MovedEvent&) { ++moved; },
                [&](const EnergyChangedEvent&) { ++energy; },
                [&](const ResourceCollectedEvent&) { ++resource; },
                [](const auto&) {},
            }, event);
        }
        assert(moved == 1 && energy == 1 && resource == 1);
    }

    std::cout << "interactions_test: todas las pruebas pasaron\n";
    return 0;
}
