// Pruebas del vigilante: controlador con BFS y reglas del turno (proyecto 2, secciones 3.1, 3.2 y 7)
#include <cassert>
#include <cstddef>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <vector>
#include "../include/circuit_escape/controllers/guard_controller.hpp"
#include "../include/circuit_escape/environment.hpp"
#include "../include/circuit_escape/events.hpp"
#include "../include/circuit_escape/graph/graph.hpp"
#include "../include/circuit_escape/scenario.hpp"

using Env = NavigationEnvironment<3, 5>;
using Policy = BfsGuardPolicy<3, 5>;

static_assert(GuardPolicy<Policy>);  // la política cumple el contrato del vigilante

std::shared_ptr<IGuardController> makeGuard(const Grid<Cell, 3, 5>& grid) {
    return std::make_shared<GuardPolicyController<Policy>>(Policy{MapGraph<3, 5>{grid, GameRules<>{}}});
}

std::size_t guardMoves(const StepResult& r) {
    return countEvents<GuardMovedEvent>(r.events.begin(), r.events.end());
}

std::size_t distance(Position a, Position b) {
    const std::size_t rows = a.row > b.row ? a.row - b.row : b.row - a.row;
    const std::size_t columns = a.column > b.column ? a.column - b.column : b.column - a.column;
    return rows + columns;
}

int main() {
    const std::vector<Action> all{Action::wait, Action::up, Action::down, Action::left, Action::right};

    // 1. actionTowards convierte el paso del camino en una acción
    {
        assert(actionTowards({1, 1}, {0, 1}) == Action::up);
        assert(actionTowards({1, 1}, {2, 1}) == Action::down);
        assert(actionTowards({1, 1}, {1, 0}) == Action::left);
        assert(actionTowards({1, 1}, {1, 2}) == Action::right);
        assert(actionTowards({1, 1}, {1, 1}) == Action::wait);
        assert(actionTowards({1, 1}, {2, 2}) == Action::wait);   // no son vecinas
    }

    // 2. La política propone el primer paso del camino más corto y respeta muros
    {
        Grid<Cell, 3, 5> g{};
        Policy free{MapGraph<3, 5>{g, GameRules<>{}}};
        assert(free.selectAction({{1, 4}, {1, 0}, 2}, all) == Action::left);

        g.at({1, 3}) = Wall{};   // muro entre el vigilante y el jugador: debe rodearlo
        Policy blocked{MapGraph<3, 5>{g, GameRules<>{}}};
        const Action step = blocked.selectAction({{1, 4}, {1, 0}, 2}, all);
        assert(step == Action::up || step == Action::down);
        assert(step == blocked.selectAction({{1, 4}, {1, 0}, 2}, all));   // desempate fijo
    }

    // 3. Sin camino hacia el jugador: espera
    {
        Grid<Cell, 3, 5> g{};
        g.at({0, 2}) = Wall{};
        g.at({1, 2}) = Wall{};
        g.at({2, 2}) = Wall{};
        Policy policy{MapGraph<3, 5>{g, GameRules<>{}}};
        assert(policy.selectAction({{1, 4}, {1, 0}, 2}, all) == Action::wait);
    }

    // Tablero base: inicio del jugador (1,0), salida (2,4), vigilante (1,4)
    Grid<Cell, 3, 5> base{};
    base.at({2, 4}) = Exit{};

    // 4. Avanza solo en turnos pares y recalcula hacia la posición ACTUAL del jugador
    {
        Env e{base, Position{1, 0}, 50, 100};
        e.setGuard({1, 4}, makeGuard(base));
        assert(e.state().hasGuard && !e.state().guardMovesNextTurn);   // el turno 1 es impar

        StepResult r = e.step(Action::wait);                            // turno 1: quieto
        assert(guardMoves(r) == 0 && (*e.guardPosition() == Position{1, 4}));
        assert(r.observation.guardMovesNextTurn);                       // el turno 2 es par

        r = e.step(Action::wait);                                       // turno 2: avanza
        assert(guardMoves(r) == 1 && (*e.guardPosition() == Position{1, 3}));

        r = e.step(Action::up);                                         // turno 3: jugador a (0,0); vigilante quieto
        assert(guardMoves(r) == 0);
        const std::size_t before = distance(*e.guardPosition(), e.getAgentPosition());
        r = e.step(Action::wait);                                       // turno 4: persigue la nueva posición
        assert(guardMoves(r) == 1);
        assert(distance(*e.guardPosition(), e.getAgentPosition()) + 1 == before);
    }

    // 5. Captura inicial: el jugador entra a la casilla del vigilante (en turno impar)
    {
        Env e{base, Position{1, 0}, 50, 100};
        e.setGuard({1, 1}, makeGuard(base));
        StepResult r = e.step(Action::right);
        assert(r.finished && r.reason == EndReason::captured);
        assert(countEvents<PlayerCapturedEvent>(r.events.begin(), r.events.end()) == 1);
        assert(guardMoves(r) == 0);                                     // capturado antes de que actúe
    }

    // 6. Captura por el movimiento del vigilante (turno par)
    {
        Env e{base, Position{1, 0}, 50, 100};
        e.setGuard({1, 2}, makeGuard(base));
        (void)e.step(Action::wait);                                     // turno 1: vigilante quieto en (1,2)
        StepResult r = e.step(Action::right);                           // turno 2: jugador a (1,1), vigilante lo alcanza
        assert(r.finished && r.reason == EndReason::captured);
    }

    // 7. La captura tiene prioridad sobre el escape
    {
        Grid<Cell, 3, 5> g{};
        g.at({1, 2}) = Exit{};
        Env e{g, Position{1, 1}, 50, 100};
        e.setGuard({1, 3}, makeGuard(g));
        (void)e.step(Action::wait);                                     // turno 1
        StepResult r = e.step(Action::right);                           // turno 2: llega a la salida y el vigilante también
        assert(r.reason == EndReason::captured);
        assert(countEvents<GoalReachedEvent>(r.events.begin(), r.events.end()) == 0);
    }

    // 8. No consume baterías ni gasta energía del jugador
    {
        Grid<Cell, 3, 5> g = base;
        g.at({1, 3}) = Battery{};
        Env e{g, Position{1, 0}, 50, 100};
        e.setGuard({1, 4}, makeGuard(g));
        (void)e.step(Action::wait);
        (void)e.step(Action::wait);                                     // el vigilante pisa la batería
        assert((*e.guardPosition() == Position{1, 3}));
        assert(!std::get<Battery>(e.grid().at({1, 3})).consumed);
        assert(e.getEnergy() == 48);                                    // solo las dos esperas del jugador
    }

    // 9. Encerrado por muros: espera y nunca atraviesa un muro
    {
        Grid<Cell, 3, 5> g = base;
        g.at({0, 4}) = Wall{};
        g.at({1, 3}) = Wall{};
        g.at({2, 4}) = Wall{};
        g.at({2, 3}) = Exit{};
        Env e{g, Position{1, 0}, 50, 100};
        e.setGuard({1, 4}, makeGuard(g));
        for (int turn = 0; turn < 4; ++turn) (void)e.step(Action::wait);
        assert((*e.guardPosition() == Position{1, 4}));
    }

    // 10. reset devuelve al vigilante a su casilla inicial
    {
        Env e{base, Position{1, 0}, 50, 100};
        e.setGuard({1, 4}, makeGuard(base));
        (void)e.step(Action::wait);
        (void)e.step(Action::wait);
        e.reset(3);
        assert((*e.guardPosition() == Position{1, 4}));
        assert(!e.isFinished());
    }

    // 11. Validación de setGuard
    {
        Env e{base, Position{1, 0}, 50, 100};
        bool threw = false;
        try { e.setGuard({1, 4}, nullptr); } catch (const std::invalid_argument&) { threw = true; }
        assert(threw);
        threw = false;
        try { e.setGuard({1, 0}, makeGuard(base)); } catch (const std::invalid_argument&) { threw = true; }
        assert(threw);   // en el inicio del jugador
        threw = false;
        try { e.setGuard({2, 4}, makeGuard(base)); } catch (const std::invalid_argument&) { threw = true; }
        assert(threw);   // en la salida (no es casilla libre)
        assert(!e.guardPosition().has_value());
    }

    // 12. Escenario de texto: 'V' marca el inicio del vigilante
    {
        auto scenario = scenarioFromLines<3, 5>({".....", "@...V", "....S"});
        assert(scenario.guard.has_value() && (*scenario.guard == Position{1, 4}));
        assert(std::holds_alternative<Empty>(scenario.grid.at({1, 4})));
        bool threw = false;
        try { (void)scenarioFromLines<3, 5>({"V....", "@...V", "....S"}); }
        catch (const std::invalid_argument&) { threw = true; }
        assert(threw);
    }

    std::cout << "guard_test: todas las pruebas pasaron\n";
    return 0;
}
