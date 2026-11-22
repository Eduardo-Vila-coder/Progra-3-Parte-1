// Pruebas de la ayuda dinámica de energía y de que ninguna de sus teclas consume turno.
// La tabla se abre con H (rúbrica) y también con P ( acceso directo al cálculo de Dijkstra ).
// Se enlaza con circuit_escape_ui porque GameSession vive en la capa de presentación.
#include <cassert>
#include <iostream>
#include <string>
#include <variant>

#include <ftxui/component/event.hpp>
#include <ftxui/dom/elements.hpp>
#include <ftxui/screen/screen.hpp>

#include "circuit_escape/console_ui.hpp"
#include "circuit_escape/grid.hpp"

namespace {

ftxui::Screen draw(ftxui::Element element)
{
    // `true` evita recortar la pantalla a la altura de la terminal de CI.
    auto screen = ftxui::Screen::Create(ftxui::Dimension::Fit(element, true));
    ftxui::Render(screen, element);
    return screen;
}

bool contains(const std::string& text, const std::string& fragment)
{
    return text.find(fragment) != std::string::npos;
}

} // namespace

int main()
{
    const ConsoleUI ui{RenderMode::ascii};

    // 1. Traducción: P calcula la ruta, H abre/cierra la ayuda.
    for (char key : {'p', 'P'})
    {
        const auto command = ui.translate(ftxui::Event::Character(key));
        assert(command.has_value());
        assert(std::holds_alternative<Calculo_ruta>(*command));
    }
    for (char key : {'h', 'H'})
    {
        const auto command = ui.translate(ftxui::Event::Character(key));
        assert(command.has_value());
        assert(std::holds_alternative<HelpCommand>(*command));
    }

    Grid<Cell, 20, 30> grid{};
    grid.at({0, 1}) = Exit{};
    NavigationEnvironment<20, 30> environment{grid, Position{0, 0}, 10, 100};
    GameSession session{environment, ui};
    const Observation before = environment.state();

    // 2. P muestra la tabla de energía con el estado actual; no mueve al agente.
    assert(session.handle(ftxui::Event::Character('p')) == KeyResult::handled);
    assert(contains(session.message(), "Ayuda de energía"));
    const std::string table = draw(session.render()).ToString();
    assert(contains(table, "AYUDA DE ENERGÍA"));
    assert(contains(table, "Energía disponible: 10"));
    assert(contains(table, "Estado:"));
    assert(environment.state().turn == before.turn);
    assert(environment.state().energy == before.energy);
    assert(environment.state().agent == before.agent);
    assert(environment.state().score == before.score);

    // 3. Una segunda pulsación de P cierra la tabla.
    assert(session.handle(ftxui::Event::Character('P')) == KeyResult::handled);
    assert(!contains(session.message(), "Ayuda de energía"));
    assert(environment.state().turn == before.turn);
    assert(environment.state().energy == before.energy);

    // 4. H abre la ayuda completa: tabla dinámica ARRIBA y guía de controles ABAJO.
    assert(session.handle(ftxui::Event::Character('H')) == KeyResult::handled);
    assert(session.showingHelp());
    const std::string help = draw(session.render()).ToString();
    assert(contains(help, "AYUDA DE ENERGÍA"));
    assert(contains(help, "Energía disponible: 10"));
    assert(contains(help, "GUÍA Y CONTROLES DEL JUEGO"));
    assert(help.find("AYUDA DE ENERGÍA") < help.find("GUÍA Y CONTROLES DEL JUEGO"));
    assert(environment.state().turn == before.turn);
    assert(environment.state().energy == before.energy);
    assert(environment.state().agent == before.agent);
    assert(environment.state().score == before.score);

    // 5. H también cierra; tampoco consume turno.
    assert(session.handle(ftxui::Event::Character('h')) == KeyResult::handled);
    assert(!session.showingHelp());
    assert(environment.state().turn == before.turn);
    assert(environment.state().energy == before.energy);

    // 6. H cierra la tabla abierta con P (P y H comparten la misma pantalla).
    assert(session.handle(ftxui::Event::Character('p')) == KeyResult::handled);
    assert(contains(session.message(), "Ayuda de energía"));
    assert(session.handle(ftxui::Event::Character('H')) == KeyResult::handled);
    assert(!contains(session.message(), "Ayuda de energía"));
    assert(!session.showingHelp());
    assert(environment.state().turn == before.turn);
    assert(environment.state().energy == before.energy);

    std::cout << "help_test: todas las pruebas pasaron\n";
    return 0;
}
