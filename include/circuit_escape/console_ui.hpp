#pragma once
// --- CAPA DE PRESENTACIÓN (enunciado 5.8) ---
// Es la ÚNICA parte que incluye FTXUI. Grid, NavigationEnvironment, eventos y controladores no la conocen,
// por eso el motor y sus pruebas se compilan sin FTXUI.
#include <memory>
#include <optional>
#include <span>
#include <string>
#include <variant>
#include <vector>
#include <ftxui/component/event.hpp>
#include <ftxui/dom/elements.hpp>
#include "circuit_escape/cells.hpp"
#include "circuit_escape/controllers.hpp"
#include "circuit_escape/environment.hpp"
#include "circuit_escape/events.hpp"
#include "circuit_escape/position.hpp"

struct QuitCommand
{
};
struct HelpCommand
{
};
using UiCommand = std::variant<Action, QuitCommand, HelpCommand>;
enum class RenderMode
{
    emoji,
    ascii
};
class ConsoleUI
{
public:
    explicit ConsoleUI(RenderMode mode = RenderMode::emoji);
    //? convierte una pulsación en un comando; std::nullopt si la tecla no es un comando
    std::optional<UiCommand> translate(const ftxui::Event &event) const;
    //? dibuja barra de estado, coordenadas, tablero de 20 x 30 y el pie con el último evento relevante
    //? automatic cambia la ayuda breve del pie (en modo automático se avanza con Espacio o Enter)
    ftxui::Element render(
        const NavigationEnvironment<20, 30> &environment,
        std::span<const NavigationEvent> recentEvents,
        bool automatic = false) const;
    ftxui::Element help() const;
    //? símbolo de una celda según el modo (emoji o ASCII); público para poder probarlo
    std::string glyphFor(const Cell &cell) const;

private:
    RenderMode mode_;
};

// Resultado de la partida (enunciado 4): si se completó, turnos, energía, recursos y puntaje
std::string finalSummary(EndReason reason, const Observation &observation);

// Qué pasó con una tecla: se procesó, no le corresponde a la partida (mouse, redimensionar) o pidió salir
enum class KeyResult
{
    handled,
    ignored,
    quit
};

// --- CICLO DE LA PARTIDA INTERACTIVA (tabla de 5.7: "main o GameApplication") ---
// Recibe cada tecla, la traduce con ConsoleUI y pide la acción a un controlador:
//   - modo humano (automatic == nullptr): la tecla se entrega a un HumanController;
//   - modo automático: Espacio o Enter piden la acción al IController recibido.
// En ambos casos la acción sale de IController::selectAction (despacho dinámico) y la ejecuta step.
// No duplica reglas: todo cambio del juego lo decide NavigationEnvironment.
// Está separado de main para poder probarlo sin abrir la pantalla.
class GameSession
{
public:
    GameSession(NavigationEnvironment<20, 30> &environment, const ConsoleUI &ui,
                std::unique_ptr<IController> automatic = nullptr);

    //? procesa una pulsación; una tecla desconocida muestra un mensaje y NO llama a step
    KeyResult handle(const ftxui::Event &event);
    //? tablero (o ayuda) más la línea de mensajes
    ftxui::Element render() const;

    const std::vector<NavigationEvent> &recentEvents() const noexcept { return recentEvents_; }
    const std::string &message() const noexcept { return message_; }
    bool showingHelp() const noexcept { return showHelp_; }
    bool isAutomatic() const noexcept { return automatic_ != nullptr; }

private:
    NavigationEnvironment<20, 30> &environment_;
    const ConsoleUI &ui_;
    std::unique_ptr<IController> automatic_;
    HumanController human_;                      // recibe las decisiones del teclado (modo humano)
    std::vector<NavigationEvent> recentEvents_;  // eventos del último step (los muestra el pie)
    std::string message_;
    std::string finalResult_;                    // resumen de la partida terminada: sigue visible (5.8)
    bool showHelp_{false};

    void play(Action action);
    void decideWith(IController &controller);
};
