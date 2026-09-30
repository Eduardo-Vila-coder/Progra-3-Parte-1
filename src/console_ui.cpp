#include "circuit_escape/console_ui.hpp"

#include <algorithm>
#include <array>
#include <utility>

#include "circuit_escape/overloaded.hpp"

namespace {
// Tablero de la demostración (enunciado 5.1 y 5.8)
constexpr std::size_t boardRows = 20;
constexpr std::size_t boardColumns = 30;

//? cada coordenada y cada celda es un elemento independiente de exactamente 2 columnas (enunciado 5.8)
ftxui::Element coordinateCell(std::string label)
{
    return ftxui::text(std::move(label)) | ftxui::size(ftxui::WIDTH, ftxui::EQUAL, 2);
}

//? dígitos de las reglas: keycaps en modo emoji, dígitos normales en modo ASCII
std::string coordinateLabel(std::size_t index, RenderMode mode)
{
    static const std::array<std::string, 10> keycapDigits{
        "0️⃣", "1️⃣", "2️⃣", "3️⃣", "4️⃣", "5️⃣", "6️⃣", "7️⃣", "8️⃣", "9️⃣"};
    return mode == RenderMode::emoji ? keycapDigits[index % 10] : std::to_string(index % 10);
}
} // namespace

ConsoleUI::ConsoleUI(RenderMode mode) : mode_(mode)
{
}

std::string ConsoleUI::glyphFor(const Cell &cell) const
{
    //? los recursos recogidos y las baterías consumidas se comportan como espacio libre, y así se dibujan
    const bool ascii = mode_ == RenderMode::ascii;
    const std::string empty = ascii ? "." : "⬜";
    //! Overloaded contiene lambdas las cuales usaremos std::visit(visitante, variante) con visitante siendo el obejto overloaded configurado con la lista de lambdas
    return std::visit(Overloaded{//! el std::visit verificara el tipo de cell que se le pase
                                 [&](const Empty &)
                                 { return empty; },
                                 [&](const Wall &)
                                 { return std::string(ascii ? "#" : "⬛"); },
                                 [&](const RoughTerrain &)
                                 { return std::string(ascii ? "~" : "🟫"); },
                                 [&](const ResourceCell<int> &resource)
                                 { return resource.collected ? empty : std::string(ascii ? "R" : "💎"); },
                                 [&](const Battery &battery)
                                 { return battery.consumed ? empty : std::string(ascii ? "B" : "⚡"); },
                                 [&](const Trap &)
                                 { return std::string(ascii ? "T" : "💥"); },
                                 [&](const Exit &)
                                 { return std::string(ascii ? "S" : "🏁"); }},
                      cell);
}

std::optional<UiCommand> ConsoleUI::translate(const ftxui::Event &event) const
{
    //? se aceptan mayúsculas y minúsculas, y también las flechas
    if (event == ftxui::Event::Character('w') || event == ftxui::Event::ArrowUp || event == ftxui::Event::Character('W'))
    {
        return Action::up;
    }
    if (event == ftxui::Event::Character('s') || event == ftxui::Event::ArrowDown || event == ftxui::Event::Character('S'))
    {
        return Action::down;
    }
    if (event == ftxui::Event::Character('a') || event == ftxui::Event::ArrowLeft || event == ftxui::Event::Character('A'))
    {
        return Action::left;
    }
    if (event == ftxui::Event::Character('d') || event == ftxui::Event::ArrowRight || event == ftxui::Event::Character('D'))
    {
        return Action::right;
    }
    if (event == ftxui::Event::Character('e') || event == ftxui::Event::Character('E'))
    {
        return Action::wait;
    }
    if (event == ftxui::Event::Character('h') || event == ftxui::Event::Character('H'))
    {
        return HelpCommand{};
    }
    if (event == ftxui::Event::Character('q') || event == ftxui::Event::Character('Q'))
    {
        return QuitCommand{};
    }
    return std::nullopt;
}

ftxui::Element ConsoleUI::render(const NavigationEnvironment<20, 30> &environment,
                                 std::span<const NavigationEvent> recentEvents) const
{
    using namespace ftxui;
    const Observation state = environment.state();
    const auto &grid = environment.grid();

    //? total de recursos del mapa (los recogidos siguen en el tablero marcados como collected)
    //! ::Cell es la celda del juego (FTXUI también tiene un tipo Cell)
    const auto totalResources = std::count_if(grid.begin(), grid.end(), [](const ::Cell &cell)
                                              { return std::holds_alternative<ResourceCell<int>>(cell); });

    //! barra de estado: siempre encima de la grilla (enunciado 5.8)
    Element statusBar = text("Turno " + std::to_string(state.turn) + "/" + std::to_string(environment.maxTurns()) +
                             " | Energía " + std::to_string(state.energy) + "/" + std::to_string(state.maximumEnergy) +
                             " | Puntaje " + std::to_string(state.score) +
                             " | Recursos " + std::to_string(state.collectedResources) + "/" +
                             std::to_string(totalResources));

    Elements gridRows;

    //? regla horizontal: la esquina usa el mismo símbolo que un espacio vacío
    Elements columnLabels;
    columnLabels.push_back(coordinateCell(mode_ == RenderMode::emoji ? "⬜" : " "));
    for (std::size_t column = 0; column < boardColumns; ++column)
    {
        columnLabels.push_back(coordinateCell(coordinateLabel(column, mode_)));
    }
    gridRows.push_back(hbox(std::move(columnLabels)));

    //* cada fila empieza con su coordenada vertical y sigue con sus 30 celdas
    for (std::size_t row = 0; row < boardRows; ++row)
    {
        Elements rowElements;
        rowElements.push_back(coordinateCell(coordinateLabel(row, mode_)));
        for (std::size_t column = 0; column < boardColumns; ++column)
        {
            const Position current{row, column};
            //? el agente se dibuja encima de la celda que ocupa
            std::string symbol = current == state.agent ? (mode_ == RenderMode::emoji ? "🤖" : "@")
                                                        : glyphFor(grid.at(current));
            rowElements.push_back(text(std::move(symbol)) | size(WIDTH, EQUAL, 2));
        }
        gridRows.push_back(hbox(std::move(rowElements)));
    }

    //? pie: el último evento junto a una ayuda breve (sin prompt ni cursor de entrada)
    const std::string lastEvent = recentEvents.empty() ? "Listo" : describe(recentEvents.back());
    Element footer = text(lastEvent + " | WASD mover · E esperar · H ayuda · Q salir");

    return vbox({statusBar,
                 vbox(std::move(gridRows)),
                 footer});
}

ftxui::Element ConsoleUI::help() const
{
    using namespace ftxui;

    Elements help_lines;

    help_lines.push_back(text(" GUÍA Y CONTROLES DEL JUEGO ") | bold | center);
    help_lines.push_back(separator());

    help_lines.push_back(text("Controles de Navegación:") | bold);
    help_lines.push_back(text("  • W / Flecha Arriba   : Mover hacia arriba"));
    help_lines.push_back(text("  • S / Flecha Abajo    : Mover hacia abajo"));
    help_lines.push_back(text("  • A / Flecha Izquierda: Mover a la izquierda"));
    help_lines.push_back(text("  • D / Flecha Derecha  : Mover a la derecha"));
    help_lines.push_back(text("  • E                   : Esperar un turno"));
    help_lines.push_back(text("  • Espacio / Enter     : Avanzar un turno (modo automático)"));
    help_lines.push_back(text("  • H                   : Mostrar / Ocultar esta ayuda"));
    help_lines.push_back(text("  • Q                   : Salir del juego"));

    help_lines.push_back(separator());

    help_lines.push_back(text("Simbología del Tablero:") | bold);

    if (mode_ == RenderMode::emoji)
    {
        help_lines.push_back(text("  • 🤖 : Agente / Jugador"));
        help_lines.push_back(text("  • ⬜ : Espacio libre"));
        help_lines.push_back(text("  • ⬛ : Muro (no se puede atravesar)"));
        help_lines.push_back(text("  • 🟫 : Terreno de costo elevado"));
        help_lines.push_back(text("  • 💎 : Recurso (suma puntos, una sola vez)"));
        help_lines.push_back(text("  • ⚡ : Batería (recupera energía, una sola vez)"));
        help_lines.push_back(text("  • 💥 : Trampa (resta energía y puntaje en cada entrada)"));
        help_lines.push_back(text("  • 🏁 : Salida"));
    }
    else
    {
        help_lines.push_back(text("  •  @ : Agente / Jugador"));
        help_lines.push_back(text("  •  . : Espacio libre"));
        help_lines.push_back(text("  •  # : Muro"));
        help_lines.push_back(text("  •  ~ : Terreno de costo elevado"));
        help_lines.push_back(text("  •  R : Recurso"));
        help_lines.push_back(text("  •  B : Batería"));
        help_lines.push_back(text("  •  T : Trampa"));
        help_lines.push_back(text("  •  S : Salida"));
    }

    help_lines.push_back(separator());
    help_lines.push_back(text("Pulsa H para volver al tablero") | dim);

    return vbox(std::move(help_lines)) | border;
}
