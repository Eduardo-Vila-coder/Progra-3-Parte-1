// Pruebas de eventos, std::variant y templates variádicos (enunciado 6.4 y 8)
// Reglas estándar: costo general 1, esperar/rechazo 1, recurso +10, trampa -2 energía y -1 punto.

#include <cassert>
#include <deque>
#include <iostream>
#include <list>
#include <string>
#include <variant>
#include <vector>

#include "circuit_escape/environment.hpp"
#include "circuit_escape/events.hpp"
#include "circuit_escape/overloaded.hpp"

using Env = NavigationEnvironment<3, 4>;

// Cuenta cada tipo de evento con std::visit + Overloaded (sin holds_alternative ni if por tipo)
struct EventCounter {
    int moved{}, rejected{}, resource{}, energy{}, trap{}, goal{};

    void operator()(const NavigationEvent& event) {
        std::visit(Overloaded{
            [this](const MovedEvent&)             { ++moved; },
            [this](const MovementRejectedEvent&)  { ++rejected; },
            [this](const ResourceCollectedEvent&) { ++resource; },
            [this](const EnergyChangedEvent&)     { ++energy; },
            [this](const TrapTriggeredEvent&)     { ++trap; },
            [this](const GoalReachedEvent&)       { ++goal; },
        }, event);
    }
};

int main() {
    const Position start{1, 1};

    // 1. Una partida corta produce los 6 tipos de evento y todos se procesan con visit
    //    Tablero:  (1,0)=Trampa  (1,1)=inicio  (1,2)=Recurso  (1,3)=Salida  (0,1)=Muro
    {
        Grid<Cell, 3, 4> g{};
        g.at({0, 1}) = Wall{};
        g.at({1, 0}) = Trap{};
        g.at({1, 2}) = ResourceCell<int>{};
        g.at({1, 3}) = Exit{};
        Env e{g, start, 20, 100};

        EventCounter counter;
        std::vector<NavigationEvent> all;
        for (Action action : {Action::up, Action::left, Action::right, Action::right, Action::right}) {
            StepResult r = e.step(action);
            for (const NavigationEvent& event : r.events) {
                counter(event);
                all.push_back(event);
            }
        }
        assert(counter.rejected == 1);   // up contra el muro
        assert(counter.trap == 1);       // left a la trampa
        assert(counter.resource == 1);   // recurso
        assert(counter.goal == 1);       // salida
        assert(counter.moved == 4);      // left, right, right, right
        assert(counter.energy >= 5);     // cada acción gastó energía
        assert(e.isFinished());
        assert(std::holds_alternative<GoalReachedEvent>(all.back())); // la victoria es el último evento
    }

    // 2. Contenido de cada evento: valores concretos
    {
        Grid<Cell, 3, 4> g{};
        g.at({1, 2}) = ResourceCell<int>{};
        Env e{g, start, 10, 100};
        StepResult r = e.step(Action::right);
        const auto& moved = std::get<MovedEvent>(r.events[0]);
        assert(moved.from == start);
        assert(moved.to == Position(1, 2));
        assert(moved.energyCost == 1);
        const auto& energy = std::get<EnergyChangedEvent>(r.events[1]);
        assert(energy.previous == 10 && energy.current == 9);
        const auto& resource = std::get<ResourceCollectedEvent>(r.events[2]);
        assert(resource.at == Position(1, 2));
        assert(resource.points == 10);
    }

    // 3. MovementRejectedEvent guarda la posición y la acción intentada
    {
        Grid<Cell, 3, 4> g{};
        Env e{g, Position{0, 0}, 10, 100};
        StepResult r = e.step(Action::left);
        const auto& rejected = std::get<MovementRejectedEvent>(r.events[0]);
        assert(rejected.from == Position(0, 0));
        assert(rejected.action == Action::left);
    }

    // 4. holdsAnyOf (fold expression con ||)
    {
        const NavigationEvent trap = TrapTriggeredEvent{{0, 0}};
        const NavigationEvent moved = MovedEvent{{0, 0}, {0, 1}, 1};
        assert(holdsAnyOf<TrapTriggeredEvent>(trap));
        assert((holdsAnyOf<GoalReachedEvent, TrapTriggeredEvent>(trap)));
        assert((!holdsAnyOf<GoalReachedEvent, ResourceCollectedEvent>(trap)));
        assert((holdsAnyOf<MovedEvent, MovementRejectedEvent, EnergyChangedEvent>(moved)));
        static_assert(holdsAnyOf<GoalReachedEvent>(NavigationEvent{GoalReachedEvent{}})); // también en compilación
    }

    // 5. countEvents con distintos contenedores y con rango vacío (mismo algoritmo)
    {
        const std::vector<NavigationEvent> v{
            MovedEvent{}, EnergyChangedEvent{}, TrapTriggeredEvent{}, EnergyChangedEvent{}};
        const std::list<NavigationEvent> l(v.begin(), v.end());
        const std::deque<NavigationEvent> d(v.begin(), v.end());

        assert(countEvents<EnergyChangedEvent>(v.begin(), v.end()) == 2);
        assert(countEvents<EnergyChangedEvent>(l.begin(), l.end()) == 2);
        assert(countEvents<EnergyChangedEvent>(d.begin(), d.end()) == 2);
        assert((countEvents<MovedEvent, TrapTriggeredEvent>(v.begin(), v.end()) == 2));
        assert(countEvents<GoalReachedEvent>(v.begin(), v.end()) == 0);

        const std::vector<NavigationEvent> empty;
        assert(countEvents<MovedEvent>(empty.begin(), empty.end()) == 0);
    }

    // 6. describe: un texto para cada tipo de evento
    {
        assert(describe(MovedEvent{{1, 1}, {1, 2}, 2}) == "Movimiento (1,1) -> (1,2) (costo 2)");
        assert(describe(MovementRejectedEvent{{0, 0}, Action::up}) == "Movimiento rechazado hacia arriba desde (0,0)");
        assert(describe(ResourceCollectedEvent{{2, 3}, 10}) == "Recurso +10 en (2,3)");
        assert(describe(EnergyChangedEvent{5, 3}) == "Energia 5 -> 3");
        assert(describe(TrapTriggeredEvent{{1, 0}}) == "Trampa activada en (1,0)");
        assert(describe(GoalReachedEvent{{1, 3}}) == "Salida alcanzada en (1,3)");
    }

    // 7. appendEvents (fold expression con ,) respeta el orden y admite cero argumentos
    {
        std::vector<NavigationEvent> events;
        appendEvents(events, TrapTriggeredEvent{{1, 0}}, EnergyChangedEvent{5, 3}, GoalReachedEvent{{1, 3}});
        assert(events.size() == 3);
        assert(std::holds_alternative<TrapTriggeredEvent>(events[0]));
        assert(std::holds_alternative<EnergyChangedEvent>(events[1]));
        assert(std::holds_alternative<GoalReachedEvent>(events[2]));
        appendEvents(events);
        assert(events.size() == 3);
    }

    // 8. Esperar sin cambiar nada relevante: solo EnergyChangedEvent, ningún evento de celda
    {
        Grid<Cell, 3, 4> g{};
        g.at(start) = Trap{};
        Env e{g, start, 10, 100};
        StepResult r = e.step(Action::wait);
        assert(countEvents<EnergyChangedEvent>(r.events.begin(), r.events.end()) == 1);
        assert((countEvents<MovedEvent, TrapTriggeredEvent, ResourceCollectedEvent>(
                    r.events.begin(), r.events.end()) == 0));
    }

    std::cout << "events_test: todas las pruebas pasaron\n";
    return 0;
}
