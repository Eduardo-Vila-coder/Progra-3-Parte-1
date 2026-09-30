#pragma once
// --- CAPA DE PRESENTACIÓN (enunciado 5.8) ---
// Es la ÚNICA parte que incluye FTXUI. Grid, NavigationEnvironment, eventos y controladores no la conocen,
// por eso el motor y sus pruebas se compilan sin FTXUI.
#include <optional>
#include <span>
#include <string>
#include <variant>
#include <ftxui/component/event.hpp>
#include <ftxui/dom/elements.hpp>
#include "circuit_escape/cells.hpp"
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
    //? dibuja barra de estado, coordenadas, tablero de 20 x 30 y el pie con el último evento
    ftxui::Element render(
        const NavigationEnvironment<20, 30> &environment,
        std::span<const NavigationEvent> recentEvents) const;
    ftxui::Element help() const;
    //? símbolo de una celda según el modo (emoji o ASCII); público para poder probarlo
    std::string glyphFor(const Cell &cell) const;

private:
    RenderMode mode_;
};
