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
                                 std::span<const NavigationEvent> recentEvents,
                                 bool automatic) const
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

    //? pie: el último evento relevante junto a una ayuda breve (sin prompt ni cursor de entrada).
    //? Se prefiere el último evento que NO sea un cambio de energía (la energía ya está en la barra de estado)
    std::string lastEvent = "Listo";
    if (!recentEvents.empty())
    {
        const auto relevant = std::find_if(recentEvents.rbegin(), recentEvents.rend(), [](const NavigationEvent &event)
                                           { return !holdsAnyOf<EnergyChangedEvent>(event); });
        lastEvent = describe(relevant != recentEvents.rend() ? *relevant : recentEvents.back());
    }
    const std::string controls = automatic ? "Espacio/Enter avanzar · H ayuda · Q salir"
                                           : "WASD mover · E esperar · H ayuda · Q salir";
    Element footer = text(lastEvent + " | " + controls);

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

std::string finalSummary(EndReason reason, const Observation &observation)
{
    std::string outcome;
    switch (reason)
    {
    case EndReason::goalReached: outcome = "¡Partida completada!"; break;
    case EndReason::noEnergy:    outcome = "Sin energía."; break;
    case EndReason::turnLimit:   outcome = "Límite de turnos alcanzado."; break;
    case EndReason::none:        outcome = "Partida en curso."; break;
    }
    return outcome + " Turnos " + std::to_string(observation.turn) +
           " | Energía " + std::to_string(observation.energy) +
           " | Recursos " + std::to_string(observation.collectedResources) +
           " | Puntaje " + std::to_string(observation.score);
}

// ===== GameSession =====

namespace
{
const std::string automaticHint = "Modo automático: pulsa Espacio o Enter para avanzar un turno.";
} // namespace

GameSession::GameSession(NavigationEnvironment<20, 30> &environment, const ConsoleUI &ui,
                         std::unique_ptr<IController> automatic)
    : environment_(environment), ui_(ui), automatic_(std::move(automatic)),
      message_(automatic_ ? automaticHint : "")
{
}

// Ejecuta un turno y guarda sus eventos para dibujarlos
void GameSession::play(Action action)
{
    StepResult result = environment_.step(action);
    recentEvents_ = std::move(result.events);
    if (result.finished)
    {
        finalResult_ = finalSummary(result.reason, result.observation); // el pie ya indica "Q salir"
    }
    message_ = finalResult_;
}

// Pide la acción a un controlador (humano o automático) mediante la interfaz IController
void GameSession::decideWith(IController &controller)
{
    const Observation observation = environment_.state();
    play(controller.selectAction(observation, observation.availableActions)); // despacho dinámico
}

KeyResult GameSession::handle(const ftxui::Event &event)
{
    const std::optional<UiCommand> command = ui_.translate(event);

    if (!command)
    {
        // Modo automático: Espacio o Enter pide la acción al controlador
        const bool advance = event == ftxui::Event::Character(' ') || event == ftxui::Event::Return;
        if (automatic_ && advance && !environment_.isFinished())
        {
            decideWith(*automatic_);
            return KeyResult::handled;
        }
        // Tecla desconocida: se muestra un mensaje y NO se ejecuta step (enunciado 5.8)
        if (event.is_character() || event == ftxui::Event::Return)
        {
            message_ = environment_.isFinished() ? finalResult_ : "Tecla no reconocida. Pulsa H para ver la ayuda.";
            return KeyResult::handled;
        }
        return KeyResult::ignored; // otros eventos (mouse, cambio de tamaño) los maneja FTXUI
    }

    return std::visit(Overloaded{
                          [](QuitCommand)
                          { return KeyResult::quit; },
                          [this](HelpCommand)
                          {
                              showHelp_ = !showHelp_;
                              return KeyResult::handled;
                          },
                          [this](Action action)
                          {
                              if (environment_.isFinished())
                              {
                                  message_ = finalResult_;
                              }
                              else if (automatic_)
                              {
                                  message_ = automaticHint;
                              }
                              else
                              {
                                  human_.provide(action); // la interfaz entrega la decisión al controlador humano
                                  decideWith(human_);
                              }
                              return KeyResult::handled;
                          },
                      },
                      *command);
}

ftxui::Element GameSession::render() const
{
    // La UI solo dibuja el estado que produce el entorno
    ftxui::Element view = showHelp_ ? ui_.help() : ui_.render(environment_, recentEvents_, isAutomatic());
    return ftxui::vbox({view, ftxui::text(message_)});
}
