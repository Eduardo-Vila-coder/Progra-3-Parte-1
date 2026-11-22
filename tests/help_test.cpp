// Pruebas de la tecla P (cálculo de ruta con Dijkstra) y de que la ayuda no consume turno.
// Se enlaza con circuit_escape_ui porque GameSession vive en la capa de presentación.
#include <cassert>
#include <iostream>
#include <string>
#include <variant>

#include <ftxui/component/event.hpp>

#include "circuit_escape/console_ui.hpp"
#include "circuit_escape/grid.hpp"

int main() {
    const ConsoleUI ui{RenderMode::ascii};

    // 1. P y P mayúscula se traducen al comando de cálculo de ruta.
    for (char key : {'p', 'P'}) {
        const auto command = ui.translate(ftxui::Event::Character(key));
        assert(command.has_value());
        assert(std::holds_alternative<Calculo_ruta>(*command));
    }

    Grid<Cell, 20, 30> grid{};
    grid.at({0, 1}) = Exit{};
    NavigationEnvironment<20, 30> environment{grid, Position{0, 0}, 10, 100};
    GameSession session{environment, ui};
    const Observation before = environment.state();

    // 2. Calcular la ruta muestra el tramo inmediato del Dijkstra sin mover al agente.
    assert(session.handle(ftxui::Event::Character('p')) == KeyResult::handled);
    const std::string route = session.message();
    assert(route.find("El camino a seguir") != std::string::npos);
    assert(route.find("(0,1)") != std::string::npos);
    assert(environment.state().turn == before.turn);
    assert(environment.state().energy == before.energy);
    assert(environment.state().agent == before.agent);
    assert(environment.state().score == before.score);

    // 3. La ayuda H sigue sin consumir turno; P tampoco lo consume con la ayuda abierta.
    assert(session.handle(ftxui::Event::Character('H')) == KeyResult::handled);
    assert(session.showingHelp());
    assert(session.handle(ftxui::Event::Character('p')) == KeyResult::handled);
    assert(session.showingHelp());
    assert(environment.state().turn == before.turn);
    assert(environment.state().energy == before.energy);
    assert(session.handle(ftxui::Event::Character('h')) == KeyResult::handled);
    assert(!session.showingHelp());

    std::cout << "help_test: todas las pruebas pasaron\n";
    return 0;
}
