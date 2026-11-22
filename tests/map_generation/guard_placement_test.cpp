// Pruebas de la ubicación del vigilante en los mapas generados (proyecto 2).
// El vigilante debe quedar en una sala desde la que la partida se pueda ganar.
// winnableWithGuard (rápido) se compara con una exploración usando el entorno REAL
// (NavigationEnvironment + BfsGuardPolicy), para comprobar que aplica las mismas reglas.
// No se guarda ni se reproduce una secuencia ganadora: solo se comprueba que existe.
#include <cassert>
#include <cstddef>
#include <iostream>
#include <memory>
#include <queue>
#include <string>
#include <vector>

#include "circuit_escape/controllers/guard_controller.hpp"
#include "circuit_escape/environment.hpp"
#include "circuit_escape/graph/graph.hpp"
#include "circuit_escape/map_generation/map_batch_runner.hpp"
#include "circuit_escape/map_generation/map_builder.hpp"
#include "circuit_escape/map_generation/map_layout.hpp"
#include "circuit_escape/scenario.hpp"

namespace {

// Exploración de referencia con el motor real: prueba todas las acciones del jugador
// por turnos y descarta estados repetidos (jugador, vigilante, paridad, A, B).
template <std::size_t Rows, std::size_t Columns>
bool winnableWithRealEngine(const Grid<Cell, Rows, Columns>& grid, const map_layout::MapEntities& e)
{
    Grid<Cell, Rows, Columns> board = grid;
    board.at(e.goal) = Exit{};
    board.at(e.switchA) = Switch{SwitchId::A};
    board.at(e.switchB) = Switch{SwitchId::B};

    using Environment = NavigationEnvironment<Rows, Columns>;
    using Policy = BfsGuardPolicy<Rows, Columns>;
    Environment initial{board, e.start, GameRules<Difficulty::Hard>{}};
    initial.setGuard(e.guard, std::make_shared<GuardPolicyController<Policy>>(
                                  Policy{MapGraph<Rows, Columns>{board, GameRules<>{}}}));

    constexpr std::size_t cells = Rows * Columns;
    std::vector<bool> seen(cells * cells * 8, false);
    std::queue<Environment> pending;
    pending.push(initial);
    while (!pending.empty()) {
        const Environment current = pending.front();
        pending.pop();
        for (const Action action : {Action::up, Action::down, Action::left, Action::right, Action::wait}) {
            Environment next = current;
            const StepResult result = next.step(action);
            if (result.reason == EndReason::goalReached) {
                return true;
            }
            if (result.finished) {
                continue;
            }
            const Observation& o = result.observation;
            const std::size_t key = (((o.agent.row * Columns + o.agent.column) * cells +
                                      (o.guard.row * Columns + o.guard.column)) * 2 + o.turn % 2) * 4 +
                                    (o.switchAActive ? 2 : 0) + (o.switchBActive ? 1 : 0);
            if (!seen[key]) {
                seen[key] = true;
                pending.push(std::move(next));
            }
        }
    }
    return false;
}

} // namespace

int main()
{
    // 1. La comprobación rápida coincide con el motor real para TODAS las salas posibles
    //    del vigilante, en varios laberintos pequeños (incluye casos ganables y no ganables).
    {
        constexpr std::size_t Rows = 7;
        constexpr std::size_t Columns = 9;
        std::size_t winnable = 0;
        std::size_t notWinnable = 0;
        for (unsigned seed = 1; seed <= 4; ++seed) {
            const auto built = MapBuilder<Rows, Columns>::build(seed, seed * 97, 2);
            const std::vector<Position> rooms = map_layout::rooms(built.grid);
            map_layout::MapEntities base{rooms.front(), rooms.back(), rooms[1], rooms[2], rooms[3]};
            for (const Position room : rooms) {
                if (room == base.start || room == base.goal || room == base.switchA || room == base.switchB) {
                    continue;
                }
                map_layout::MapEntities attempt = base;
                attempt.guard = room;
                const bool fast = map_layout::winnableWithGuard(built.grid, attempt);
                assert(fast == winnableWithRealEngine(built.grid, attempt));
                ++(fast ? winnable : notWinnable);
            }
        }
        assert(winnable > 0 && notWinnable > 0);  // se probaron ambos casos
    }

    // 2. chooseEntities ubica al vigilante en una sala ganable, distinta de los demás elementos
    {
        constexpr std::size_t Rows = 20;
        constexpr std::size_t Columns = 30;
        std::size_t placed = 0;
        for (unsigned seed = 1; seed <= 6; ++seed) {
            const auto built = MapBuilder<Rows, Columns>::build(seed, seed * 1234567u, 2);
            const map_layout::MapEntities e = map_layout::chooseEntities(built.grid, built.seed);
            if (!e.winnable) {
                // Ninguna sala sirve: el validador debe rechazar el mapa
                MazeMapValidator<Rows, Columns> validator;
                assert(!validator.validate(built).hasVictory);
                continue;
            }
            ++placed;
            assert(e.guard != e.start && e.guard != e.goal && e.guard != e.switchA && e.guard != e.switchB);
            assert(map_layout::winnableWithGuard(built.grid, e));
        }
        assert(placed > 0);
    }

    // 3. Los tres niveles entregados (assets/maps) se pueden ganar con el vigilante en standard
    for (const std::string file : {"mapa_facil.txt", "mapa_medio.txt", "mapa_dificil.txt"}) {
        const auto scenario = loadScenario<20, 30>(scenarioPath(file));
        assert(scenario.guard.has_value());
        map_layout::MapEntities e{};
        e.start = scenario.start;
        e.guard = *scenario.guard;
        for (std::size_t row = 0; row < 20; ++row) {
            for (std::size_t column = 0; column < 30; ++column) {
                const Cell& cell = scenario.grid.at({row, column});
                if (std::holds_alternative<Exit>(cell)) e.goal = {row, column};
                if (const Switch* s = std::get_if<Switch>(&cell)) {
                    (s->id == SwitchId::A ? e.switchA : e.switchB) = Position{row, column};
                }
            }
        }
        assert(map_layout::winnableWithGuard(scenario.grid, e));
    }

    std::cout << "guard_placement_test: todas las pruebas pasaron\n";
    return 0;
}
