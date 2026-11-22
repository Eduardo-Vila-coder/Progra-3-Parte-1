// Pruebas de los interruptores A y B y de la salida (proyecto 2, secciones 3.1 y 7)
#include <cassert>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>
#include "../include/circuit_escape/environment.hpp"
#include "../include/circuit_escape/events.hpp"
#include "../include/circuit_escape/scenario.hpp"

using Env = NavigationEnvironment<3, 5>;

// Tablero de prueba (fila 1):  (1,0)=a  (1,1)=inicio  (1,2)=Salida  (1,3)=b
Grid<Cell, 3, 5> switchesGrid() {
    Grid<Cell, 3, 5> g{};
    g.at({1, 0}) = Switch{SwitchId::A};
    g.at({1, 2}) = Exit{};
    g.at({1, 3}) = Switch{SwitchId::B};
    return g;
}

const Position start{1, 1};

int main() {
    // 1. Orden A -> B: la salida no gana antes de tiempo y sí gana con ambos activados
    {
        Env e{switchesGrid(), start, 20, 100};
        StepResult r = e.step(Action::left);                       // entra a A
        assert(countEvents<SwitchActivatedEvent>(r.events.begin(), r.events.end()) == 1);
        assert(r.observation.switchAActive && !r.observation.switchBActive);
        r = e.step(Action::right);                                 // vuelve al inicio
        r = e.step(Action::right);                                 // pisa la salida solo con A
        assert(!r.finished && r.reason == EndReason::none);        // salida anticipada: no gana
        r = e.step(Action::right);                                 // entra a B
        assert(r.observation.switchAActive && r.observation.switchBActive);
        r = e.step(Action::left);                                  // vuelve a la salida
        assert(r.finished && r.reason == EndReason::goalReached);
    }

    // 2. Orden B -> A: también se puede ganar
    {
        Env e{switchesGrid(), start, 20, 100};
        StepResult r = e.step(Action::right);                      // pisa la salida sin interruptores
        assert(!r.finished);
        r = e.step(Action::right);                                 // B
        r = e.step(Action::left);                                  // salida solo con B: no gana
        assert(!r.finished);
        r = e.step(Action::left);                                  // inicio
        r = e.step(Action::left);                                  // A
        r = e.step(Action::right);
        r = e.step(Action::right);                                 // salida con A y B
        assert(r.finished && r.reason == EndReason::goalReached);
    }

    // 3. La activación persiste: volver a pisar A no genera otro evento y sigue activado
    {
        Env e{switchesGrid(), start, 20, 100};
        (void)e.step(Action::left);
        (void)e.step(Action::right);
        StepResult r = e.step(Action::left);
        assert(countEvents<SwitchActivatedEvent>(r.events.begin(), r.events.end()) == 0);
        assert(e.isSwitchActive(SwitchId::A));
        r = e.step(Action::wait);                                  // esperar encima tampoco lo repite
        assert(countEvents<SwitchActivatedEvent>(r.events.begin(), r.events.end()) == 0);
    }

    // 4. reset vuelve a dejar A y B desactivados
    {
        Env e{switchesGrid(), start, 20, 100};
        (void)e.step(Action::left);
        assert(e.isSwitchActive(SwitchId::A));
        e.reset(7);
        const Observation o = e.state();
        assert(o.hasSwitches && !o.switchAActive && !o.switchBActive);
        assert(!e.isSwitchActive(SwitchId::A) && !e.isSwitchActive(SwitchId::B));
    }

    // 5. Si la energía se agota al entrar al interruptor, la partida termina y NO se activa
    {
        Env e{switchesGrid(), start, 1, 100};
        StepResult r = e.step(Action::left);
        assert(r.finished && r.reason == EndReason::noEnergy);
        assert(!r.observation.switchAActive);
        assert(countEvents<SwitchActivatedEvent>(r.events.begin(), r.events.end()) == 0);
    }

    // 6. El último turno permitido cuenta: escapar justo en el turno límite es victoria
    {
        Env e{switchesGrid(), start, 20, 5};
        (void)e.step(Action::left);   // 1: A
        (void)e.step(Action::right);  // 2
        (void)e.step(Action::right);  // 3: salida sin B
        (void)e.step(Action::right);  // 4: B
        StepResult r = e.step(Action::left);  // 5: salida con A y B
        assert(r.finished && r.reason == EndReason::goalReached);
    }

    // 7. Validación del tablero: un A y un B, o ninguno
    {
        Grid<Cell, 3, 5> soloA{};
        soloA.at({1, 2}) = Exit{};
        soloA.at({1, 0}) = Switch{SwitchId::A};
        bool threw = false;
        try { Env e{soloA, start, 20, 100}; } catch (const std::invalid_argument&) { threw = true; }
        assert(threw);

        Grid<Cell, 3, 5> repetidos = switchesGrid();
        repetidos.at({0, 0}) = Switch{SwitchId::A};
        repetidos.at({0, 1}) = Switch{SwitchId::B};
        threw = false;
        try { Env e{repetidos, start, 20, 100}; } catch (const std::invalid_argument&) { threw = true; }
        assert(threw);

        // Sin interruptores (mapas del proyecto 1): la salida gana como antes
        Grid<Cell, 3, 5> sinInterruptores{};
        sinInterruptores.at({1, 2}) = Exit{};
        Env e{sinInterruptores, start, 20, 100};
        assert(!e.state().hasSwitches);
        StepResult r = e.step(Action::right);
        assert(r.finished && r.reason == EndReason::goalReached);
    }

    // 8. Escenarios de texto: 'a' y 'b' son los interruptores
    {
        const std::vector<std::string> lines{".....", "a@Sb.", "....."};
        auto scenario = scenarioFromLines<3, 5>(lines);
        assert(std::holds_alternative<Switch>(scenario.grid.at({1, 0})));
        assert(std::get<Switch>(scenario.grid.at({1, 0})).id == SwitchId::A);
        assert(std::get<Switch>(scenario.grid.at({1, 3})).id == SwitchId::B);
        assert((scenario.start == Position{1, 1}));

        bool threw = false;
        try { (void)scenarioFromLines<3, 5>({".....", "a@S..", "....."}); }
        catch (const std::invalid_argument&) { threw = true; }
        assert(threw);
    }

    // 9. El evento se describe para la interfaz
    {
        const NavigationEvent event = SwitchActivatedEvent{Position{1, 0}, SwitchId::A};
        assert(describe(event).find("Interruptor A activado") != std::string::npos);
    }

    // 10. La observación incluye el mapa de solo lectura (enunciado 4): es una copia del tablero
    {
        Env e{switchesGrid(), start, 20, 100};
        Observation before = e.state();
        assert(before.rows == 3 && before.columns == 5 && before.map.size() == 15);
        assert(std::holds_alternative<Exit>(before.cellAt({1, 2})));
        assert(!std::get<Switch>(before.cellAt({1, 0})).active);

        StepResult r = e.step(Action::left);                       // entra a A: el mapa del siguiente estado lo refleja
        assert(std::get<Switch>(r.observation.cellAt({1, 0})).active);
        assert(!std::get<Switch>(before.cellAt({1, 0})).active);   // la copia anterior no cambió

        bool threw = false;
        try { (void)before.cellAt({9, 9}); }
        catch (const std::out_of_range&) { threw = true; }
        assert(threw);
    }

    std::cout << "switches_test: todas las pruebas pasaron\n";
    return 0;
}
