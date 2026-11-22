#pragma once

#include <algorithm>
#include <cstddef>
#include <limits>
#include <optional>
#include <queue>
#include <stdexcept>
#include <utility>
#include <vector>

#include "circuit_escape/cells.hpp"
#include "circuit_escape/game_rules.hpp"
#include "circuit_escape/graph/graph.hpp"
#include "circuit_escape/map_generation/map_builder.hpp"
#include "circuit_escape/position.hpp"

// Colocación de los elementos jugables sobre un mapa generado por Kruskal.
// MapBuilder solo produce salas (Empty) y muros (Wall); aquí se eligen posiciones
// deterministas para el inicio, la salida, los interruptores A/B y el vigilante.
// La salida y el vigilante se ubican de modo que la partida SÍ se pueda ganar en los tres
// perfiles de dificultad (ver chooseEntities).
namespace map_layout {

// Última fila/columna que es sala (índice par). Para 20x30 devuelve 18 y 28.
[[nodiscard]] constexpr std::size_t lastRoomIndex(std::size_t extent) noexcept
{
    return extent == 0 ? 0 : ((extent - 1) / 2) * 2;
}

template <std::size_t Rows, std::size_t Columns>
[[nodiscard]] std::vector<Position> rooms(const Grid<Cell, Rows, Columns>& grid)
{
    std::vector<Position> result;
    for (std::size_t row = 0; row < Rows; row += 2) {
        for (std::size_t column = 0; column < Columns; column += 2) {
            if (isTraversable(grid.at({row, column}))) {
                result.push_back(Position{row, column});
            }
        }
    }
    return result;
}

struct MapEntities
{
    Position start{};
    Position goal{};
    Position switchA{};
    Position switchB{};
    Position guard{};
    bool winnable{false}; // existe al menos una partida ganadora con el vigilante en esta sala
};

// Distancia en movimientos desde 'origin' a cada casilla (max si es inalcanzable)
template <std::size_t Rows, std::size_t Columns>
[[nodiscard]] std::vector<std::size_t> distancesFrom(const Grid<Cell, Rows, Columns>& grid, Position origin)
{
    constexpr std::size_t unreachable = std::numeric_limits<std::size_t>::max();
    std::vector<std::size_t> distance(Rows * Columns, unreachable);
    std::queue<Position> pending;
    distance[origin.row * Columns + origin.column] = 0;
    pending.push(origin);
    while (!pending.empty()) {
        const Position current = pending.front();
        pending.pop();
        for (const Action action : {Action::right, Action::up, Action::down, Action::left}) {
            const std::optional<Position> next = neighbor(current, action);
            if (!next.has_value() || !grid.contains(*next) || !isTraversable(grid.at(*next))) {
                continue;
            }
            std::size_t& d = distance[next->row * Columns + next->column];
            if (d == unreachable) {
                d = distance[current.row * Columns + current.column] + 1;
                pending.push(*next);
            }
        }
    }
    return distance;
}

// Paso del vigilante: next[g * celdas + p] es la casilla a la que avanza el vigilante que está
// en g cuando el jugador está en p (o g si no hay camino). Reproduce exactamente BfsGuardPolicy:
// un BFS desde g con el orden de vecinos de MapGraph y el primer paso del camino reconstruido.
template <std::size_t Rows, std::size_t Columns>
[[nodiscard]] std::vector<std::size_t> guardStepTable(const MapGraph<Rows, Columns>& graph)
{
    constexpr std::size_t cells = Rows * Columns;
    constexpr std::size_t none = std::numeric_limits<std::size_t>::max();
    std::vector<std::size_t> next(cells * cells);
    std::vector<std::size_t> previous(cells);
    for (std::size_t guard = 0; guard < cells; ++guard) {
        for (std::size_t player = 0; player < cells; ++player) {
            next[guard * cells + player] = guard;
        }
        if (!graph.isOpen(guard)) {
            continue;
        }
        std::fill(previous.begin(), previous.end(), none);
        std::queue<std::size_t> pending;
        previous[guard] = guard;
        pending.push(guard);
        while (!pending.empty()) {
            const std::size_t current = pending.front();
            pending.pop();
            for (const std::size_t neighborCell : graph.neighbors(current)) {
                if (previous[neighborCell] == none) {
                    previous[neighborCell] = current;
                    pending.push(neighborCell);
                }
            }
        }
        for (std::size_t player = 0; player < cells; ++player) {
            if (previous[player] == none || player == guard) {
                continue;
            }
            std::size_t step = player;
            while (previous[step] != guard) {
                step = previous[step];
            }
            next[guard * cells + player] = step;
        }
    }
    return next;
}

// Perfil más exigente: si un mapa se puede ganar en hard (menos energía y menos turnos, mismos
// costos en salas y muros), también se puede ganar en standard y en easy.
using HardestRules = GameRules<Difficulty::Hard>;

// ¿Existe alguna partida ganadora con estas posiciones? Recorre por turnos todos los estados
// (jugador, vigilante, paridad del turno, A, B) siguiendo las reglas del proyecto 2 con el
// perfil hard (el más exigente): el jugador actúa, captura, el vigilante avanza en turnos pares, captura o
// energía agotada, interruptor y escape. NO guarda ni reproduce una secuencia ganadora (el
// profesor indicó que no es necesario): solo evita colocar al vigilante donde nadie podría ganar.
// Los mapas generados solo tienen salas y muros, así que cada turno cuesta lo mismo y llegar
// antes a un estado nunca es peor. guard_placement_test lo compara con el entorno real.
template <std::size_t Rows, std::size_t Columns>
[[nodiscard]] bool winnableWithGuard(const Grid<Cell, Rows, Columns>& grid, const MapEntities& entities)
{
    const HardestRules rules{};
    if (rules.energyCostGeneral != rules.energyCostOthers) {
        throw std::logic_error("winnableWithGuard supone que todas las acciones cuestan lo mismo");
    }
    constexpr std::size_t cells = Rows * Columns;
    const MapGraph<Rows, Columns> graph{grid, GameRules<>{}};
    const std::vector<std::size_t> guardStep = guardStepTable(graph);
    const auto index = [](Position p) { return p.row * Columns + p.column; };
    const std::size_t goal = index(entities.goal);
    const std::size_t switchA = index(entities.switchA);
    const std::size_t switchB = index(entities.switchB);

    struct State { std::size_t player; std::size_t guard; std::size_t mask; };
    std::vector<bool> seen(cells * cells * 8, false);
    const auto key = [](const State& s, std::size_t turn) {
        return ((s.player * cells + s.guard) * 2 + turn % 2) * 4 + s.mask;
    };

    std::vector<State> current{State{index(entities.start), index(entities.guard), 0}};
    for (std::size_t turn = 1; turn <= rules.turnLimit && !current.empty(); ++turn) {
        const int energy = rules.initialMaxEnergy - static_cast<int>(turn) * rules.energyCostGeneral;
        std::vector<State> following;
        for (const State& state : current) {
            const Position at = toPosition(state.player, Columns);
            for (const Action action : {Action::up, Action::down, Action::left, Action::right, Action::wait}) {
                // 1. Actúa el jugador (un movimiento inválido lo deja en su sitio)
                std::size_t player = state.player;
                const std::optional<Position> target = neighbor(at, action);
                if (target.has_value() && grid.contains(*target) && isTraversable(grid.at(*target))) {
                    player = index(*target);
                }
                // 2. Captura inicial
                if (player == state.guard) {
                    continue;
                }
                // 3. El vigilante avanza en turnos pares
                const std::size_t guard = turn % 2 == 0 ? guardStep[state.guard * cells + player] : state.guard;
                // 4. Captura o energía agotada: derrota; si no, interruptor y escape
                if (player == guard || energy <= 0) {
                    continue;
                }
                std::size_t mask = state.mask;
                if (player == switchA) mask |= 2;
                if (player == switchB) mask |= 1;
                if (player == goal && mask == 3) {
                    return true;
                }
                const State next{player, guard, mask};
                if (!seen[key(next, turn)]) {
                    seen[key(next, turn)] = true;
                    following.push_back(next);
                }
            }
        }
        current = std::move(following);
    }
    return false;
}

    // Solo ELIGE las posiciones (no modifica el tablero). La usa el validador.
    // - Inicio, A y B: salas fijas (las primeras por filas).
    // - Salida: una sala cuyo recorrido mínimo Inicio -> A -> B -> Salida (o B -> A) se pueda pagar
    //   con la energía del perfil hard. Entre esas, la más cercana a un costo objetivo que depende
    //   de la semilla, para que los mapas tengan costos distintos y MapSelector pueda clasificarlos.
    // - Vigilante: de la sala más lejana a la más cercana al inicio, la primera desde la que existe
    //   al menos una partida ganadora en hard (y por lo tanto también en standard y easy).
    // Si no encuentra ninguna combinación, devuelve winnable == false y el validador rechaza el mapa.
    template <std::size_t Rows, std::size_t Columns>
    [[nodiscard]] MapEntities chooseEntities(const Grid<Cell, Rows, Columns>& grid, unsigned seed = 0)
    {
        const std::vector<Position> available = rooms(grid);
        if (available.size() < 5) {
            throw std::invalid_argument("El mapa no tiene salas suficientes para los elementos");
        }
        const MapEntities fallback{available.front(), available.back(), available[1], available[2], available[3]};

        constexpr std::size_t unreachable = std::numeric_limits<std::size_t>::max();
        const std::vector<std::size_t> fromStart = distancesFrom(grid, fallback.start);
        const std::vector<std::size_t> fromA = distancesFrom(grid, fallback.switchA);
        const std::vector<std::size_t> fromB = distancesFrom(grid, fallback.switchB);
        const auto at = [](const std::vector<std::size_t>& d, Position p) { return d[p.row * Columns + p.column]; };
        const auto sum = [](std::size_t x, std::size_t y, std::size_t z) {
            return (x == unreachable || y == unreachable || z == unreachable) ? unreachable : x + y + z;
        };
        // Movimientos del mejor recorrido completo hasta 'goal' (cada paso cuesta 1 en estos mapas)
        const auto routeCost = [&](Position goal) {
            const std::size_t viaAB = sum(at(fromStart, fallback.switchA), at(fromA, fallback.switchB), at(fromB, goal));
            const std::size_t viaBA = sum(at(fromStart, fallback.switchB), at(fromB, fallback.switchA), at(fromA, goal));
            return std::min(viaAB, viaBA);
        };
        const auto reserved = [&](Position p) {
            return p == fallback.start || p == fallback.switchA || p == fallback.switchB;
        };

        // Energía del perfil hard: hay que llegar con energía positiva
        const std::size_t budget = static_cast<std::size_t>(HardestRules{}.initialMaxEnergy - 1);
        std::vector<Position> goals;
        std::size_t cheapest = unreachable;
        for (const Position room : available) {
            if (!reserved(room) && routeCost(room) <= budget) {
                goals.push_back(room);
                cheapest = std::min(cheapest, routeCost(room));
            }
        }
        if (goals.empty()) {
            return fallback;
        }
        // El objetivo va desde la mitad de la energía hasta el máximo pagable: ningún nivel es trivial
        const std::size_t lowest = std::max(cheapest, budget / 2);
        const std::size_t target = lowest + seed % (budget - lowest + 1);
        const auto gap = [&](Position p) {
            const std::size_t cost = routeCost(p);
            return cost > target ? cost - target : target - cost;
        };
        std::stable_sort(goals.begin(), goals.end(), [&](Position a, Position b) { return gap(a) < gap(b); });

        constexpr std::size_t maxGoals = 6;       // acotan el tiempo de generación
        constexpr std::size_t maxCandidates = 12;
        for (std::size_t g = 0; g < goals.size() && g < maxGoals; ++g) {
            MapEntities entities = fallback;
            entities.goal = goals[g];

            std::vector<Position> guards;
            for (const Position room : available) {
                if (!reserved(room) && room != entities.goal && at(fromStart, room) != unreachable) {
                    guards.push_back(room);
                }
            }
            std::stable_sort(guards.begin(), guards.end(), [&](Position a, Position b) {
                return at(fromStart, a) > at(fromStart, b);
            });
            for (std::size_t i = 0; i < guards.size() && i < maxCandidates; ++i) {
                entities.guard = guards[i];
                if (winnableWithGuard(grid, entities)) {
                    entities.winnable = true;
                    return entities;
                }
            }
        }
        return fallback;
    }

// Elige salas distintas en orden fijo y las escribe en el tablero. El inicio y el
// vigilante siguen siendo Empty en la celda; el escritor de texto los marca con '@' y 'V'.
    template <std::size_t Rows, std::size_t Columns>
    MapEntities placeEntities(BuiltMap<Rows, Columns>& map)
    {
        const MapEntities entities = chooseEntities(map.grid, map.seed);

        map.grid.at(entities.goal) = Exit{};
        map.grid.at(entities.switchA) = Switch{SwitchId::A};
        map.grid.at(entities.switchB) = Switch{SwitchId::B};

        return entities;
    }

} // namespace map_layout
