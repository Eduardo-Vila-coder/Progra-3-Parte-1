#include <algorithm>
#include <cassert>
#include <iostream>
#include <stdexcept>
#include <string>
#include <variant>
#include <vector>
#include "../include/circuit_escape/environment.hpp"
#include "../include/circuit_escape/game_rules.hpp"
#include "../include/circuit_escape/scenario.hpp"

// Convierte una letra de la ruta en una acción: U arriba, D abajo, L izquierda, R derecha
Action actionFromLetter(char letter) {
    switch (letter) {
    case 'U': return Action::up;
    case 'D': return Action::down;
    case 'L': return Action::left;
    case 'R': return Action::right;
    }
    return Action::wait;
}

// Ejecuta una ruta fija en el motor real. Ningún paso debe chocar con un muro ni salir del tablero.
template <std::size_t Rows, std::size_t Columns>
StepResult playRoute(NavigationEnvironment<Rows, Columns>& environment, const std::string& route) {
    StepResult last{};
    for (char letter : route) {
        assert(!environment.isFinished()); // la partida no debe terminar antes de completar la ruta
        last = environment.step(actionFromLetter(letter));
        for (const NavigationEvent& event : last.events) {
            assert(!std::holds_alternative<MovementRejectedEvent>(event));
        }
    }
    return last;
}

// El borde completo del tablero debe ser muro (enunciado 5.8)
template <std::size_t Rows, std::size_t Columns>
bool hasWallBorder(const Grid<Cell, Rows, Columns>& grid) {
    for (std::size_t row = 0; row < Rows; ++row) {
        for (std::size_t column = 0; column < Columns; ++column) {
            const bool border = row == 0 || row == Rows - 1 || column == 0 || column == Columns - 1;
            if (border && !std::holds_alternative<Wall>(grid.at({row, column}))) {
                return false;
            }
        }
    }
    return true;
}

// Deben aparecer los siete tipos de celda (enunciado 5.5)
template <std::size_t Rows, std::size_t Columns>
bool hasAllCellTypes(const Grid<Cell, Rows, Columns>& grid) {
    std::vector<bool> seen(std::variant_size_v<Cell>, false);
    for (const Cell& cell : grid) {
        seen[cell.index()] = true;
    }
    return std::all_of(seen.begin(), seen.end(), [](bool value) { return value; });
}

int main() {
    const GameRules<Difficulty::Standard> standard{};

    // ===== CARGADOR (tableros pequeños de 3x4) =====

    // Prueba 1: Un mapa válido se carga con cada símbolo en su celda
    {
        auto scenario = scenarioFromLines<3, 4>({"@..S",
                                                 ".#~.",
                                                 "RBT."});
        assert(scenario.start == Position(0, 0));
        assert(std::holds_alternative<Empty>(scenario.grid.at({0, 0})));
        assert(std::holds_alternative<Exit>(scenario.grid.at({0, 3})));
        assert(std::holds_alternative<Wall>(scenario.grid.at({1, 1})));
        assert(std::holds_alternative<RoughTerrain>(scenario.grid.at({1, 2})));
        assert(std::holds_alternative<ResourceCell<int>>(scenario.grid.at({2, 0})));
        assert(std::holds_alternative<Battery>(scenario.grid.at({2, 1})));
        assert(std::holds_alternative<Trap>(scenario.grid.at({2, 2})));
    }

    // Prueba 2: Mapas inválidos lanzan std::invalid_argument
    {
        const std::vector<std::vector<std::string>> invalidMaps{
            {"@..S", ".#~."},                // faltan filas
            {"@..S", ".#~", "RBT."},         // fila corta
            {"@..S", ".X~.", "RBT."},        // símbolo desconocido
            {"@..S", ".#@.", "RBT."},        // dos inicios
            {"@...", ".#~.", "RBT."},        // sin salida
            {"@.SS", ".#~.", "RBT."},        // dos salidas
        };
        for (const auto& lines : invalidMaps) {
            bool threw = false;
            try {
                (void)scenarioFromLines<3, 4>(lines);
            } catch (const std::invalid_argument&) {
                threw = true;
            }
            assert(threw);
        }
    }

    // Prueba 3: Un archivo inexistente lanza std::runtime_error
    {
        bool threw = false;
        try {
            (void)loadScenario<20, 30>(scenarioPath("no_existe.txt"));
        } catch (const std::runtime_error&) {
            threw = true;
        }
        assert(threw);
    }

    // ===== ESCENARIO 1 (ejemplo del enunciado, página 11) =====
    // Se puede completar en easy y standard; en hard NO (ver nota en scenario.hpp).
    {
        auto scenario = loadScenario<20, 30>(scenarioPath(scenario01File));

        // Prueba 4: Estructura: 20 x 30, borde de muros y los siete tipos de celda
        assert(hasWallBorder(scenario.grid));
        assert(hasAllCellTypes(scenario.grid));

        // Prueba 5: Es idéntico al enunciado (inicio, salida y elementos en las mismas posiciones)
        assert(scenario.start == Position(1, 1));
        assert(std::holds_alternative<Exit>(scenario.grid.at({18, 28})));
        assert(std::holds_alternative<ResourceCell<int>>(scenario.grid.at({2, 10})));
        assert(std::holds_alternative<ResourceCell<int>>(scenario.grid.at({10, 24})));
        assert(std::holds_alternative<ResourceCell<int>>(scenario.grid.at({17, 4})));
        assert(std::holds_alternative<Battery>(scenario.grid.at({8, 17})));
        assert(std::holds_alternative<Trap>(scenario.grid.at({5, 11})));
        assert(std::holds_alternative<Trap>(scenario.grid.at({14, 23})));
        assert(std::holds_alternative<RoughTerrain>(scenario.grid.at({3, 16})));
        assert(std::holds_alternative<RoughTerrain>(scenario.grid.at({9, 4})));
        assert(std::holds_alternative<RoughTerrain>(scenario.grid.at({15, 18})));

        // Prueba 6: Ruta A (cruza por el centro) llega a la salida con el perfil standard
        {
            NavigationEnvironment<20, 30> environment{scenario.grid, scenario.start,
                                                      standard.initialMaxEnergy, standard.turnLimit};
            assert(environment.state().goal == Position(18, 28));
            StepResult result = playRoute(environment, "RRRRDDDRRRRRRRDDDRRRRRDRRDDRRRDDDDDDDRRRRRRD");
            assert(result.finished);
            assert(result.reason == EndReason::goalReached);
            assert(result.observation.turn == 44);
            assert(result.observation.energy == 19);
        }

        // Prueba 7: Ruta B (alternativa: cruza todo por el norte) también llega con standard
        {
            NavigationEnvironment<20, 30> environment{scenario.grid, scenario.start,
                                                      standard.initialMaxEnergy, standard.turnLimit};
            StepResult result = playRoute(environment, "RRRRDDDRRUUURRRRRRRRRRRRDDRRRDDDDDDDDDDDDDDRRRRRRD");
            assert(result.finished);
            assert(result.reason == EndReason::goalReached);
            assert(result.observation.turn == 50);
            assert(result.observation.energy == 10);
        }
    }

    // ===== ESCENARIO 2 (diseño propio: paso norte y paso sur) =====
    // Se puede completar en easy, standard y hard (ver pruebas 11 a 13).
    {
        auto scenario = loadScenario<20, 30>(scenarioPath(scenario02File));

        // Prueba 8: Estructura: 20 x 30, borde de muros y los siete tipos de celda
        assert(hasWallBorder(scenario.grid));
        assert(hasAllCellTypes(scenario.grid));
        assert(scenario.start == Position(9, 3));
        assert(std::holds_alternative<Exit>(scenario.grid.at({9, 26})));

        // Prueba 9: Ruta norte (pasa por la batería) llega a la salida con standard
        {
            NavigationEnvironment<20, 30> environment{scenario.grid, scenario.start,
                                                      standard.initialMaxEnergy, standard.turnLimit};
            assert(environment.state().goal == Position(9, 26));
            StepResult result = playRoute(environment, "UUUURRRRRRRRRRRRRRRRRRRRRRRDDDD");
            assert(result.finished);
            assert(result.reason == EndReason::goalReached);
            assert(result.observation.turn == 31);
            assert(result.observation.energy == 32); // 60 - 31 + 3 de la batería
        }

        // Prueba 10: Ruta sur (pasa por el terreno elevado) también llega con standard
        {
            NavigationEnvironment<20, 30> environment{scenario.grid, scenario.start,
                                                      standard.initialMaxEnergy, standard.turnLimit};
            StepResult result = playRoute(environment, "RRRRRRRRRRDDDDRRRRRRUUUURRRRRRR");
            assert(result.finished);
            assert(result.reason == EndReason::goalReached);
            assert(result.observation.turn == 31);
            assert(result.observation.energy == 27); // 60 - 31 - 2 extra por el terreno elevado
        }

        // Prueba 11: En easy (80 de energía) la ruta norte llega a la salida
        {
            NavigationEnvironment<20, 30> environment{scenario.grid, scenario.start, GameRules<Difficulty::Easy>{}};
            StepResult result = playRoute(environment, "UUUURRRRRRRRRRRRRRRRRRRRRRRDDDD");
            assert(result.reason == EndReason::goalReached);
            assert(result.observation.maximumEnergy == 80);
            assert(result.observation.energy == 54); // 80 - 31 + 5 de la batería
        }

        // Prueba 12: En hard (40 de energía, batería +2) la ruta norte llega a la salida
        {
            NavigationEnvironment<20, 30> environment{scenario.grid, scenario.start, GameRules<Difficulty::Hard>{}};
            StepResult result = playRoute(environment, "UUUURRRRRRRRRRRRRRRRRRRRRRRDDDD");
            assert(result.reason == EndReason::goalReached);
            assert(result.observation.maximumEnergy == 40);
            assert(result.observation.energy == 11); // 40 - 31 + 2 de la batería
        }

        // Prueba 13: En hard (terreno elevado cuesta 3) la ruta sur también llega a la salida
        {
            NavigationEnvironment<20, 30> environment{scenario.grid, scenario.start, GameRules<Difficulty::Hard>{}};
            StepResult result = playRoute(environment, "RRRRRRRRRRDDDDRRRRRRUUUURRRRRRR");
            assert(result.reason == EndReason::goalReached);
            assert(result.observation.energy == 5); // 40 - 31 - 4 extra por dos casillas de terreno elevado
        }
    }

    // ===== DIFICULTAD ELEGIDA AL EJECUTAR =====

    // Prueba 14: El escenario 1 NO se puede completar en hard: con la ruta más corta
    // (44 movimientos) la energía se agota antes de llegar a la salida
    {
        auto scenario = loadScenario<20, 30>(scenarioPath(scenario01File));
        NavigationEnvironment<20, 30> environment{scenario.grid, scenario.start, GameRules<Difficulty::Hard>{}};
        const std::string routeA = "RRRRDDDRRRRRRRDDDRRRRRDRRDDRRRDDDDDDDRRRRRRD";
        StepResult result{};
        for (char letter : routeA) {
            if (environment.isFinished()) {
                break;
            }
            result = environment.step(actionFromLetter(letter));
        }
        assert(result.finished);
        assert(result.reason == EndReason::noEnergy);
        assert(result.observation.agent != Position(18, 28));
    }

    // Prueba 15: makeEnvironment crea el entorno con el perfil de la dificultad pedida
    {
        auto scenario = loadScenario<20, 30>(scenarioPath(scenario02File));
        assert(makeEnvironment(scenario.grid, scenario.start, Difficulty::Easy).state().maximumEnergy == 80);
        assert(makeEnvironment(scenario.grid, scenario.start, Difficulty::Standard).state().maximumEnergy == 60);
        assert(makeEnvironment(scenario.grid, scenario.start, Difficulty::Hard).state().maximumEnergy == 40);

        // El perfil también cambia los costos: en hard el terreno elevado cuesta 3
        auto hard = makeEnvironment(scenario.grid, scenario.start, Difficulty::Hard);
        StepResult result = playRoute(hard, "RRRRRRRRRRDDDDRRRRRR"); // hasta pasar el terreno elevado del paso sur
        assert(result.observation.energy == 40 - 20 - 4);
    }

    std::cout << "scenarios_test: todas las pruebas pasaron\n";
    return 0;
}
