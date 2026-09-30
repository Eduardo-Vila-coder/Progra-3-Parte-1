// Pruebas de la interfaz de consola (enunciado 8):
// traducción de comandos, comando desconocido, emoji/ASCII de cada celda y tamaño del renderizado.
// No abre una pantalla interactiva: dibuja sobre un ftxui::Screen en memoria.

#include <cassert>
#include <iostream>
#include <memory>
#include <optional>
#include <string>
#include <variant>
#include <vector>

#include <ftxui/component/event.hpp>
#include <ftxui/dom/elements.hpp>
#include <ftxui/screen/screen.hpp>

#include "circuit_escape/console_ui.hpp"
#include "circuit_escape/scenario.hpp"

namespace {

bool isAction(const std::optional<UiCommand>& command, Action expected) {
    return command.has_value() && std::holds_alternative<Action>(*command) && std::get<Action>(*command) == expected;
}

NavigationEnvironment<20, 30> scenarioOne() {
    const auto scenario = loadScenario<20, 30>(scenarioPath(scenario01File));
    return makeEnvironment(scenario.grid, scenario.start, Difficulty::Standard);
}

ftxui::Screen draw(ftxui::Element element) {
    auto screen = ftxui::Screen::Create(ftxui::Dimension::Fit(element));
    ftxui::Render(screen, element);
    return screen;
}

}  // namespace

int main() {
    const ConsoleUI emoji{RenderMode::emoji};
    const ConsoleUI ascii{RenderMode::ascii};

    // ===== TRADUCCIÓN DE COMANDOS =====

    // Prueba 1: W A S D E, sin distinguir mayúsculas, y las flechas
    for (char key : {'w', 'W'}) assert(isAction(emoji.translate(ftxui::Event::Character(key)), Action::up));
    for (char key : {'s', 'S'}) assert(isAction(emoji.translate(ftxui::Event::Character(key)), Action::down));
    for (char key : {'a', 'A'}) assert(isAction(emoji.translate(ftxui::Event::Character(key)), Action::left));
    for (char key : {'d', 'D'}) assert(isAction(emoji.translate(ftxui::Event::Character(key)), Action::right));
    for (char key : {'e', 'E'}) assert(isAction(emoji.translate(ftxui::Event::Character(key)), Action::wait));
    assert(isAction(emoji.translate(ftxui::Event::ArrowUp), Action::up));
    assert(isAction(emoji.translate(ftxui::Event::ArrowDown), Action::down));
    assert(isAction(emoji.translate(ftxui::Event::ArrowLeft), Action::left));
    assert(isAction(emoji.translate(ftxui::Event::ArrowRight), Action::right));

    // Prueba 2: H (ayuda) y Q (salir), sin distinguir mayúsculas
    for (char key : {'h', 'H'}) {
        const auto command = emoji.translate(ftxui::Event::Character(key));
        assert(command.has_value() && std::holds_alternative<HelpCommand>(*command));
    }
    for (char key : {'q', 'Q'}) {
        const auto command = emoji.translate(ftxui::Event::Character(key));
        assert(command.has_value() && std::holds_alternative<QuitCommand>(*command));
    }

    // Prueba 3: teclas que no son comandos
    for (char key : {'x', 'z', '1', ' '}) {
        assert(!emoji.translate(ftxui::Event::Character(key)).has_value());
    }
    assert(!emoji.translate(ftxui::Event::Return).has_value());

    // ===== SESIÓN DE JUEGO (GameSession, el mismo código que usa app/main.cpp) =====

    // Prueba 4: una tecla desconocida muestra un mensaje y NO modifica el entorno
    {
        NavigationEnvironment<20, 30> environment = scenarioOne();
        GameSession session{environment, emoji};
        const Observation before = environment.state();

        assert(session.handle(ftxui::Event::Character('x')) == KeyResult::handled);
        assert(session.handle(ftxui::Event::Character('?')) == KeyResult::handled);
        assert(session.message().find("no reconocida") != std::string::npos);
        const Observation after = environment.state();
        assert(after.turn == before.turn);
        assert(after.energy == before.energy);
        assert(after.agent == before.agent);
        assert(after.score == before.score);
        assert(session.recentEvents().empty());

        // Eventos que no son teclas (por ejemplo, uno personalizado) se ignoran sin tocar el entorno
        assert(session.handle(ftxui::Event::Custom) == KeyResult::ignored);
        assert(environment.state().turn == before.turn);
    }

    // Prueba 5: modo humano: una tecla válida ejecuta un turno, H alterna la ayuda sin gastar turno, Q pide salir
    {
        NavigationEnvironment<20, 30> environment = scenarioOne();  // agente en (1,1), (1,2) libre
        GameSession session{environment, emoji};
        assert(!session.isAutomatic());

        assert(session.handle(ftxui::Event::Character('D')) == KeyResult::handled);
        assert(environment.state().turn == 1);
        assert(environment.state().agent == Position(1, 2));
        assert(!session.recentEvents().empty());

        assert(session.handle(ftxui::Event::Character('w')) == KeyResult::handled);  // (0,2) es muro
        assert(environment.state().turn == 2);
        assert(environment.state().agent == Position(1, 2));
        assert(std::holds_alternative<MovementRejectedEvent>(session.recentEvents().front()));

        assert(session.handle(ftxui::Event::Character('H')) == KeyResult::handled);
        assert(session.showingHelp());
        assert(environment.state().turn == 2);

        assert(session.handle(ftxui::Event::Character('q')) == KeyResult::quit);
        assert(environment.state().turn == 2);
    }

    // Prueba 6: modo automático: Espacio/Enter piden la acción al controlador; WASD no mueve al agente
    {
        const auto scenario = loadScenario<20, 30>(scenarioPath(scenario02File));
        NavigationEnvironment<20, 30> environment =
            makeEnvironment(scenario.grid, scenario.start, Difficulty::Standard);
        GameSession session{environment, ascii,
                            std::make_unique<PolicyController<HeuristicPolicy>>(HeuristicPolicy{})};
        assert(session.isAutomatic());

        assert(session.handle(ftxui::Event::Character('d')) == KeyResult::handled);
        assert(environment.state().turn == 0);
        assert(session.message().find("Espacio") != std::string::npos);

        assert(session.handle(ftxui::Event::Character(' ')) == KeyResult::handled);
        assert(environment.state().turn == 1);
        assert(session.handle(ftxui::Event::Return) == KeyResult::handled);
        assert(environment.state().turn == 2);

        // La heurística completa el escenario 2; el resumen final queda visible aunque se pulsen más teclas
        while (!environment.isFinished()) {
            (void)session.handle(ftxui::Event::Character(' '));
        }
        const std::string summary = session.message();
        assert(summary.find("¡Partida completada!") != std::string::npos);
        assert(summary.find("Turnos 31") != std::string::npos);
        (void)session.handle(ftxui::Event::Character('x'));
        (void)session.handle(ftxui::Event::Character(' '));
        (void)session.handle(ftxui::Event::Character('d'));
        assert(session.message() == summary);
        assert(environment.state().turn == 31);
    }

    // Prueba 7: finalSummary informa si se completó, turnos, energía, recursos y puntaje (enunciado 4)
    {
        Observation observation{};
        observation.turn = 12;
        observation.energy = 0;
        observation.collectedResources = 2;
        observation.score = -3;
        assert(finalSummary(EndReason::noEnergy, observation) ==
               "Sin energía. Turnos 12 | Energía 0 | Recursos 2 | Puntaje -3");
        assert(finalSummary(EndReason::goalReached, observation).find("¡Partida completada!") == 0);
        assert(finalSummary(EndReason::turnLimit, observation).find("Límite de turnos") == 0);
    }

    // ===== REPRESENTACIÓN DE CADA CELDA =====

    // Prueba 8: emoji y ASCII de las siete clases de celda
    {
        assert(emoji.glyphFor(Empty{}) == "⬜" && ascii.glyphFor(Empty{}) == ".");
        assert(emoji.glyphFor(Wall{}) == "⬛" && ascii.glyphFor(Wall{}) == "#");
        assert(emoji.glyphFor(RoughTerrain{}) == "🟫" && ascii.glyphFor(RoughTerrain{}) == "~");
        assert(emoji.glyphFor(ResourceCell<int>{}) == "💎" && ascii.glyphFor(ResourceCell<int>{}) == "R");
        assert(emoji.glyphFor(Battery{}) == "⚡" && ascii.glyphFor(Battery{}) == "B");
        assert(emoji.glyphFor(Trap{}) == "💥" && ascii.glyphFor(Trap{}) == "T");
        assert(emoji.glyphFor(Exit{}) == "🏁" && ascii.glyphFor(Exit{}) == "S");

        // Consumibles ya usados se dibujan como espacio libre
        assert(ascii.glyphFor(ResourceCell<int>{10, true}) == ".");
        assert(emoji.glyphFor(Battery{3, true}) == "⬜");
    }

    // ===== RENDERIZADO COMPLETO: 20 FILAS Y 30 CELDAS POR FILA =====

    // Prueba 9 (ASCII): estado + regla + 20 filas + pie; cada fila mide 62 columnas (2 + 30 x 2)
    {
        const NavigationEnvironment<20, 30> environment = scenarioOne();
        const std::vector<NavigationEvent> noEvents;
        const ftxui::Screen screen = draw(ascii.render(environment, noEvents));

        assert(screen.dimy() == 1 + 1 + 20 + 1);
        assert(screen.dimx() == 62);

        // Regla horizontal: 0..9 repetido tres veces, empezando en x = 2
        for (int column = 0; column < 30; ++column) {
            assert(screen.PixelAt(2 + 2 * column, 1).character == std::to_string(column % 10));
        }
        // Cada fila: su coordenada y los muros de las columnas 0 y 29 (30 celdas por fila)
        for (int row = 0; row < 20; ++row) {
            const int y = 2 + row;
            assert(screen.PixelAt(0, y).character == std::to_string(row % 10));
            assert(screen.PixelAt(2, y).character == "#");
            assert(screen.PixelAt(2 + 2 * 29, y).character == "#");
        }
        // Agente en (1,1) y salida en (18,28), como en el enunciado
        assert(screen.PixelAt(2 + 2 * 1, 2 + 1).character == "@");
        assert(screen.PixelAt(2 + 2 * 28, 2 + 18).character == "S");
    }

    // Prueba 10 (emoji): mismas dimensiones; cada emoji ocupa exactamente 2 columnas
    {
        const NavigationEnvironment<20, 30> environment = scenarioOne();
        const std::vector<NavigationEvent> noEvents;
        const ftxui::Screen screen = draw(emoji.render(environment, noEvents));

        assert(screen.dimy() == 1 + 1 + 20 + 1);
        assert(screen.dimx() == 62);
        assert(screen.PixelAt(2 + 2 * 1, 2 + 1).character == "🤖");
        assert(screen.PixelAt(2 + 2 * 28, 2 + 18).character == "🏁");
        assert(screen.PixelAt(2, 2).character == "⬛");
    }

    // Prueba 11: la barra de estado y el pie muestran el estado producido por el entorno
    {
        NavigationEnvironment<20, 30> environment = scenarioOne();
        const StepResult result = environment.step(Action::up);  // (0,1) es muro -> rechazado
        const std::string text = draw(ascii.render(environment, result.events)).ToString();
        assert(text.find("Turno 1/180") != std::string::npos);
        assert(text.find("Energía 59/60") != std::string::npos);
        assert(text.find("Recursos 0/3") != std::string::npos);
        assert(text.find("WASD") != std::string::npos);
    }

    // Prueba 12: el pie muestra el último evento RELEVANTE (no el cambio de energía, que ya está arriba)
    {
        NavigationEnvironment<20, 30> environment = scenarioOne();
        const StepResult result = environment.step(Action::up);  // eventos: rechazo + cambio de energía
        assert(std::holds_alternative<EnergyChangedEvent>(result.events.back()));
        const std::string text = draw(ascii.render(environment, result.events)).ToString();
        assert(text.find("Rechazado: arriba en (1,1)") != std::string::npos);
        assert(text.find("Energia 60 -> 59") == std::string::npos);
    }

    // Prueba 13: el pie más largo posible cabe en una terminal de 80 columnas
    {
        NavigationEnvironment<20, 30> environment = scenarioOne();
        const std::vector<NavigationEvent> longest{MovedEvent{{18, 27}, {18, 28}, 3}};
        const ftxui::Screen screen = draw(emoji.render(environment, longest));
        assert(screen.dimx() <= 80);
    }

    // Prueba 14: en modo automático el pie indica cómo avanzar
    {
        const NavigationEnvironment<20, 30> environment = scenarioOne();
        const std::vector<NavigationEvent> noEvents;
        const std::string text = draw(ascii.render(environment, noEvents, true)).ToString();
        assert(text.find("Espacio/Enter avanzar") != std::string::npos);
        assert(text.find("WASD") == std::string::npos);
    }

    std::cout << "ui_test: todas las pruebas pasaron\n";
    return 0;
}
