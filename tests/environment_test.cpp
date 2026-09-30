#include <cassert>
#include <iostream>
#include "../include/circuit_escape/environment.hpp"
#include "../include/circuit_escape/cells.hpp"

int main() {
    // Prueba 1 (consulta movimiento): Un agente en (0, 0) solo debe tener los movimientos validos: down, right, wait
    Grid<Cell, 20, 30> grid{};
    grid.at(Position{15, 15}) = Exit{};  // el entorno exige exactamente una salida (enunciado 5.7)
    NavigationEnvironment<20, 30> env{grid, Position(0, 0), Position(0, 0), 10, 13};
    std::vector<Action> availableActions1{Action::down, Action::right, Action::wait};
    assert(availableActions1 == env.availableActions());

    // Prueba 2 (consulta movimiento): Un agente en (19, 29) solo debe tener los movimientos validos: up, left, wait
    NavigationEnvironment<20, 30> env2{grid, Position(0, 0), Position(19, 29), 10, 13};
    std::vector<Action> availableActions2{Action::up, Action::left, Action::wait};
    assert(availableActions2 == env2.availableActions());

    // Prueba 3 (consulta movimiento): Un agente en (10, 10) en un mapa con Walls en (9, 10) y (11, 10)
    // solo debe tener los movimientos validos: left, right, wait
    Grid<Cell, 20, 30> grid2{};
    grid2.at(Position{15, 15}) = Exit{};  // el entorno exige exactamente una salida (enunciado 5.7)
    grid2.at(Position{9, 10}) = Wall{};     // Se uso IA para saber como hacer esta asignacion de std::variant
    grid2.at(Position{11, 10}) = Wall{};
    NavigationEnvironment<20, 30> env3{grid2, Position(0, 0), Position(10, 10), 10, 13};
    std::vector<Action> availableActions3{Action::left, Action::right, Action::wait};
    assert(availableActions3 == env3.availableActions());

    // Prueba 4 (consulta movimiento): Un agente en (11, 0) en un mapa con Walls en (10, 0) y (12, 0), y con Trap en (11, 1)
    // solo debe tener los movimientos validos: right, wait
    Grid<Cell, 20, 30> grid3{};
    grid3.at(Position{15, 15}) = Exit{};  // el entorno exige exactamente una salida (enunciado 5.7)
    grid3.at(Position{10, 0}) = Wall{};
    grid3.at(Position{12, 0}) = Wall{};
    grid3.at(Position{11, 1}) = Trap{};
    NavigationEnvironment env4{grid3, Position(0, 0), Position(11, 0), 10, 13};
    std::vector availableActions4{Action::right, Action::wait};
    assert(availableActions4 == env4.availableActions());

    // Prueba 1 (realizacion movimiento): Un agente en (0, 0) debe ser capaz de realizar el movimiento down
    auto d = env.step(Action::down);
    assert((env.getAgentPosition() == Position{1, 0}));

    // Prueba 2 (realizacion movimiento): Un agente en (19, 29) no debe ser capaz de realizar el movimiento right
    auto posicionAnterior = env2.getAgentPosition();
    auto energiaAnterior = env2.getEnergy();
    d = env2.step(Action::right);
    assert(posicionAnterior == env2.getAgentPosition() && energiaAnterior - 1 == env2.getEnergy());

    // Prueba 3 (realizacion movimiento): Un agente en (10, 10) en un mapa con Walls en (9, 10) y (11, 10)
    // no debe ser capaz de realizar el movimiento up
    posicionAnterior = env3.getAgentPosition();
    energiaAnterior = env3.getEnergy();
    d = env3.step(Action::up);
    assert(posicionAnterior == env3.getAgentPosition() && energiaAnterior - 1 == env3.getEnergy());

    // Prueba 1 (carga perfiles): Dificultad estandar
    auto g1 = GameRules<Difficulty::Standard>{};
    assert(g1.initialMaxEnergy == 60);
    assert(g1.turnLimit == 180);
    assert(g1.energyCostGeneral == 1);
    assert(g1.energyCostRoughTerrain == 2);
    assert(g1.energyCostOthers == 1);
    assert(g1.reward == 10);
    assert(g1.energy == 3);
    assert(g1.energyPenalty == 2);
    assert(g1.scorePenalty == 1);

    // Prueba 2 (carga perfiles): Dificultad sencilla
    auto g2 = GameRules<Difficulty::Easy>{};
    assert(g2.initialMaxEnergy == 80);
    assert(g2.turnLimit == 240);
    assert(g2.energyCostGeneral == 1);
    assert(g2.energyCostRoughTerrain == 2);
    assert(g2.energyCostOthers == 1);
    assert(g2.reward == 15);
    assert(g2.energy == 5);
    assert(g2.energyPenalty == 1);
    assert(g2.scorePenalty == 0);

    // Prueba 3 (carga perfiles): Dificultad elevada
    auto g3 = GameRules<Difficulty::Hard>{};
    assert(g3.initialMaxEnergy == 40);
    assert(g3.turnLimit == 140);
    assert(g3.energyCostGeneral == 1);
    assert(g3.energyCostRoughTerrain == 3);
    assert(g3.energyCostOthers == 1);
    assert(g3.reward == 8);
    assert(g3.energy == 2);
    assert(g3.energyPenalty == 3);
    assert(g3.scorePenalty == 2);

    // ===== CONDICIONES DE TÉRMINO (tableros 3x4) =====

    // Prueba 1 (término): Llegar a la salida con energía termina la partida por goalReached
    {
        Grid<Cell, 3, 4> g{};
        g.at({0, 1}) = Exit{};
        NavigationEnvironment<3, 4> e{g, Position(0, 0), Position(0, 0), 10, 20};
        StepResult r = e.step(Action::right);
        assert(r.finished);
        assert(r.reason == EndReason::goalReached);
        assert(e.isFinished());
        assert(r.observation.agent == Position(0, 1));
        assert(r.observation.goal == Position(0, 1));
        assert(r.observation.energy == 9);
        assert(r.observation.turn == 1);
        assert(!r.events.empty());
        assert(std::holds_alternative<GoalReachedEvent>(r.events.back()));
        assert(std::get<GoalReachedEvent>(r.events.back()).at == Position(0, 1));
    }

    // Prueba 2 (término): La energía llega a cero termina la partida por noEnergy
    {
        Grid<Cell, 3, 4> g{};
        g.at({2, 3}) = Exit{};
        NavigationEnvironment<3, 4> e{g, Position(0, 0), Position(0, 0), 2, 50};
        StepResult r1 = e.step(Action::right);
        assert(!r1.finished);
        assert(r1.reason == EndReason::none);
        assert(!e.isFinished());
        StepResult r2 = e.step(Action::right);
        assert(r2.finished);
        assert(r2.reason == EndReason::noEnergy);
        assert(r2.observation.energy == 0);
    }

    // Prueba 3 (término): Alcanzar el límite de turnos termina la partida por turnLimit
    {
        Grid<Cell, 3, 4> g{};
        g.at({2, 3}) = Exit{};
        NavigationEnvironment<3, 4> e{g, Position(0, 0), Position(0, 0), 50, 2};
        StepResult r1 = e.step(Action::wait);
        assert(!r1.finished);
        StepResult r2 = e.step(Action::wait);
        assert(r2.finished);
        assert(r2.reason == EndReason::turnLimit);
        assert(r2.observation.turn == 2);
    }

    // Prueba 4 (precedencia): Llegar a la salida con energía 0 NO es victoria -> noEnergy
    {
        Grid<Cell, 3, 4> g{};
        g.at({0, 1}) = Exit{};
        NavigationEnvironment<3, 4> e{g, Position(0, 0), Position(0, 0), 1, 20};
        StepResult r = e.step(Action::right);
        assert(r.finished);
        assert(r.reason == EndReason::noEnergy);
        for (const NavigationEvent& event : r.events) {
            assert(!std::holds_alternative<GoalReachedEvent>(event));
        }
    }

    // Prueba 5 (precedencia): Llegar a la salida con energía en el último turno SÍ es victoria
    {
        Grid<Cell, 3, 4> g{};
        g.at({0, 2}) = Exit{};
        NavigationEnvironment<3, 4> e{g, Position(0, 0), Position(0, 0), 10, 2};
        StepResult r1 = e.step(Action::right);
        assert(!r1.finished);
        StepResult r2 = e.step(Action::right);
        assert(r2.finished);
        assert(r2.reason == EndReason::goalReached);
    }

    // Prueba 6 (precedencia): Energía agotada en el último turno -> noEnergy antes que turnLimit
    {
        Grid<Cell, 3, 4> g{};
        g.at({2, 3}) = Exit{};
        NavigationEnvironment<3, 4> e{g, Position(0, 0), Position(0, 0), 1, 1};
        StepResult r = e.step(Action::wait);
        assert(r.finished);
        assert(r.reason == EndReason::noEnergy);
    }

    // Prueba 7 (término): step después de terminar lanza std::logic_error
    // y no hay acciones disponibles
    {
        Grid<Cell, 3, 4> g{};
        g.at({0, 1}) = Exit{};
        NavigationEnvironment<3, 4> e{g, Position(0, 0), Position(0, 0), 10, 20};
        StepResult r = e.step(Action::right);
        assert(r.finished);
        assert(e.availableActions().empty());
        assert(e.state().availableActions.empty());
        assert(r.observation.availableActions.empty());

        bool threw = false;
        try {
            (void)e.step(Action::left);
        } catch (const std::logic_error&) {
            threw = true;
        }
        assert(threw);
        assert(e.state().turn == 1); // El intento rechazado no consumió turno
    }

    // ===== PRECONDICIONES DEL CONSTRUCTOR (enunciado 5.7) =====
    // Cada caso inválido debe lanzar std::invalid_argument
    {
        auto rejects = [](auto build) {
            try {
                build();
            } catch (const std::invalid_argument&) {
                return true;
            }
            return false;
        };
        Grid<Cell, 3, 4> valid{};
        valid.at({2, 3}) = Exit{};

        // Válido: no lanza
        assert(!rejects([&] { NavigationEnvironment<3, 4> e{valid, Position(0, 0), 10, 20}; }));
        // Inicio fuera del tablero
        assert(rejects([&] { NavigationEnvironment<3, 4> e{valid, Position(3, 0), 10, 20}; }));
        // Inicio sobre un muro
        Grid<Cell, 3, 4> wallStart = valid;
        wallStart.at({0, 0}) = Wall{};
        assert(rejects([&] { NavigationEnvironment<3, 4> e{wallStart, Position(0, 0), 10, 20}; }));
        // Sin salida
        Grid<Cell, 3, 4> noExit{};
        assert(rejects([&] { NavigationEnvironment<3, 4> e{noExit, Position(0, 0), 10, 20}; }));
        // Dos salidas
        Grid<Cell, 3, 4> twoExits = valid;
        twoExits.at({0, 3}) = Exit{};
        assert(rejects([&] { NavigationEnvironment<3, 4> e{twoExits, Position(0, 0), 10, 20}; }));
        // Energía inicial no positiva
        assert(rejects([&] { NavigationEnvironment<3, 4> e{valid, Position(0, 0), 0, 20}; }));
        assert(rejects([&] { NavigationEnvironment<3, 4> e{valid, Position(0, 0), -5, 20}; }));
        // Límite de turnos no positivo
        assert(rejects([&] { NavigationEnvironment<3, 4> e{valid, Position(0, 0), 10, 0}; }));
        // También con el constructor de 5 parámetros y con el de perfil de dificultad
        assert(rejects([&] { NavigationEnvironment<3, 4> e{noExit, Position(0, 0), Position(0, 0), 10, 20}; }));
        assert(rejects([&] { NavigationEnvironment<3, 4> e{noExit, Position(0, 0), GameRules<Difficulty::Hard>{}}; }));
    }

    // ===== CellTraits (especialización total y parcial, enunciado 6.3) =====
    {
        static_assert(!CellTraits<Wall>::traversable);                 // especialización total
        static_assert(CellTraits<ResourceCell<int>>::traversable);     // especialización parcial
        static_assert(CellTraits<ResourceCell<double>>::traversable);  // misma familia, otra recompensa
        static_assert(CellTraits<Empty>::traversable);                 // plantilla general
        static_assert(CellTraits<RoughTerrain>::traversable);

        // El entorno usa los rasgos para decidir qué movimientos son legales
        Grid<Cell, 3, 4> g{};
        g.at({2, 3}) = Exit{};
        g.at({0, 1}) = ResourceCell<int>{};
        g.at({1, 0}) = Wall{};
        NavigationEnvironment<3, 4> e{g, Position(0, 0), 10, 20};
        const std::vector<Action> expected{Action::right, Action::wait};  // abajo es muro
        assert(e.availableActions() == expected);
    }

    std::cout << "environment_test: todas las pruebas pasaron\n";
    return 0;
}