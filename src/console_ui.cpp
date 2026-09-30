#include "circuit_escape/console_ui.hpp"
std::string ConsoleUI::glyphFor(const Cell &cell) const{
    if (mode_ == RenderMode::ascii)
        {
            //! Overloaded contiene lambdas las cuales usaremos std::visit(visitante, variante) con visitante siendo el obejto overloaded configurado con la lista de lambdas
            return std::visit(Overloaded{//! el std::visit verificara el tipo de cell que se le pase
                                         [](const Empty &)
                                         { return std::string(" ."); },
                                         [](const Wall &)
                                         { return std::string(" #"); },
                                         [](const RoughTerrain &)
                                         { return std::string(" ~"); },
                                         [](const ResourceCell<int> &)
                                         { return std::string(" R"); },
                                         [](const Battery &)
                                         { return std::string(" B"); },
                                         [](const Trap &)
                                         { return std::string(" T"); },
                                         [](const Exit &)
                                         { return std::string(" S"); }},
                              cell);
        }
        else
        {
            return std::visit(Overloaded{[](const Empty &)
                                         { return std::string("⬜"); },
                                         [](const Wall &)
                                         { return std::string("⬛"); },
                                         [](const RoughTerrain &)
                                         { return std::string("🟫"); },
                                         [](const ResourceCell<int> &)
                                         { return std::string("💎"); },
                                         [](const Battery &)
                                         { return std::string("⚡"); },
                                         [](const Trap &)
                                         { return std::string("💥"); },
                                         [](const Exit &)
                                         { return std::string("🏁"); }},
                              cell);
        }
}
ConsoleUI::ConsoleUI(RenderMode mode = RenderMode::emoji)
{
    mode_=mode;
}
std::optional<UiCommand> ConsoleUI::translate(const ftxui::Event &event) const{
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
template <size_t Rows, size_t Columns>
ftxui::Element ConsoleUI::render(const NavigationEnvironment<Rows, Columns> &environment, std::span<const NavigationEvent> recentEvents) const
{
            using namespace ftxui;
//! -------------------------------------------------------------
        //? como se dijo hbox hace un ensamble pero de forma horizontal por lo que se hace toda la barra de estado
        //! recordemos que environment es un struct por lo que guardara todo tipo de datos 
        auto state = environment.state(); 
        Element header = hbox({
            /*el uso de | es mas como un operador del propio ftxui*/
            text(" Turno: ") | bold, /* solo aplica un estilo de texto */
            text(std::to_string(state.turn_count)) | color(Color::Yellow), /* pintado de amarillo y con una conversion a string */
            text("  |  Energía: ") | bold, 
            text(std::to_string(state.energy)) | color(state.energy > 20 ? Color::Green : Color::Red),
            text("  |  Estado: ") | bold,
            /*lo mismo solo que es un ternario anidado*/
            /*  pues seria asi se verifica si state es verdad
                si es verdadero -> se verficara el reached_exit que mostrara el estado de juego
            */
            text(state.is_alive ? (state.reached_exit ? "VICTORIA " : "EN JUEGO ") : "CAÍDO ") | color(state.is_alive ? Color::Cyan : Color::Red)})|border;
            
            /*
            ///? |border como esta por fuera aplica una caja haciendo un enmarcado 
            Funcionalidades con respecto a text y color 
                 tanto text como color provienen de FTXUI
            */
//! -----------------------------------------------------
//? es el tipo de dato para construir interfaces en FTXUI        

        Elements grid_rows;
        Elements col_labels;
        col_labels.push_back(text("  ")); // Espacio para alinear con los números de fila

        for (size_t c = 0; c < Columns; ++c)
        {
            // Usamos % 10 para que los números siempre ocupen 1 dígito visual
            //? se hace uso del mmodulo para normalizar las dsitancias y no haya desigualdades 
            std::string label = " " + std::to_string(c % 10); //! revisar esta parte de aca 
            //? size(WIDTH, EQUAL, 2) fuerza a que cada celda de cada numero ocupe 2 colunas 
            col_labels.push_back(text(label) | color(Color::GrayDark) | size(WIDTH, EQUAL, 2));
        }
        // hace las agrupaciones en la parte de filas 
        grid_rows.push_back(hbox(std::move(col_labels)));

        //* se recorre las filas verticalmente
        for (size_t r = 0; r < Rows; ++r)
        {
            Elements row_elements;

            // Número de fila a la izquierda
            std::string row_label = (r % 10 < 10 ? " " : "") + std::to_string(r % 10);
            //? se agrega el indice de las filas 
            row_elements.push_back(text(row_label) | color(Color::GrayDark));

            //? se itera en columna para revisar posiciones
            for (size_t c = 0; c < Columns; ++c)
            {
                //? se mapea las position
                Position current_pos{r, c};

                std::string symbol;

                // Si la posición actual coincide con el agente, lo dibujamos
                if (current_pos == state.agent) //? verificacion de ubicacion de jugador 
                {
                    symbol = (mode_ == RenderMode::emoji) ? "🤖" : " @"; //? se elige el caracter 
                }
                else
                {
                    // Si no, delegamos a glyphFor() para ver qué tipo de celda es
                    //? mediante el glyphFor elegimos el simbolo 
                    //! revisar esta parte 
                    symbol = glyphFor(environment.grid().at(current_pos));
                }

                // Aplicamos el tamaño fijo de 2 columnas de ancho
                //! se convierte a text y se aplica la misma configuracion del size 
                row_elements.push_back(text(symbol) | size(WIDTH, EQUAL, 2));
            }
            //? 
            grid_rows.push_back(hbox(std::move(row_elements)));
        }

        Element grid_box = vbox(std::move(grid_rows)) | border;


        Elements event_elements;
        event_elements.push_back(text("Últimos Eventos:") | bold);
        if (recentEvents.empty())
        {
            event_elements.push_back(text(" - Ninguno") | dim);
        }
        else
        {
            // Mostrar los últimos eventos pasados por std::span
            for (const auto &ev : recentEvents)
            {
                // Ejemplo de formateo rápido según tipo de evento
                event_elements.push_back(text(" • Evento registrado") | color(Color::GrayLight));
            }
        }

        Element footer = vbox({vbox(std::move(event_elements)),
                               separator(),
                               hbox({text(" Controles: ") | dim,
                                     text("[WASD / Flechas]") | bold | color(Color::Cyan),
                                     text(" Mover  |  ") | dim,
                                     text("[E]") | bold | color(Color::Cyan),
                                     text(" Esperar  |  ") | dim,
                                     text("[H]") | bold | color(Color::Yellow),
                                     text(" Ayuda  |  ") | dim,
                                     text("[Q]") | bold | color(Color::Red),
                                     text(" Salir") | dim})}) |
                         border;

        return vbox({header,
                     grid_box,
                     footer});
}
ftxui::Element ConsoleUI::help() const
{
       using namespace ftxui;

    Elements help_lines;

    help_lines.push_back(text(" GUÍA Y CONTROLES DEL JUEGO ") | bold | color(Color::Yellow) | center);
    help_lines.push_back(separator());

    help_lines.push_back(text("Controles de Navegación:") | bold | color(Color::Cyan));
    help_lines.push_back(text("  • W / Flecha Arriba   : Mover hacia arriba"));
    help_lines.push_back(text("  • S / Flecha Abajo    : Mover hacia abajo"));
    help_lines.push_back(text("  • A / Flecha Izquierda: Mover a la izquierda"));
    help_lines.push_back(text("  • D / Flecha Derecha  : Mover a la derecha"));
    help_lines.push_back(text("  • E                   : Esperar un turno"));
    help_lines.push_back(text("  • H                   : Mostrar / Ocultar esta ayuda"));
    help_lines.push_back(text("  • Q                   : Salir del juego"));
    
    help_lines.push_back(separator());

    help_lines.push_back(text("Simbología del Tablero:") | bold | color(Color::Cyan));
    
    if (mode_ == RenderMode::emoji) {
        help_lines.push_back(text("  • 🤖 : Agente / Jugador"));
        help_lines.push_back(text("  • ⬜ : Celda Vacía (Costo normal de energía)"));
        help_lines.push_back(text("  • 🧱 : Pared / Obstáculo Infranqueable"));
        help_lines.push_back(text("  • 🪨 : Terreno Difícil (Mayor costo de energía)"));
        help_lines.push_back(text("  • ⚡ : Batería (Recarga energía)"));
        help_lines.push_back(text("  • 💥 : Trampa (Resta energía o inflige daño)"));
        help_lines.push_back(text("  • 🏁 : Salida / Objetivo"));
    } else {
        help_lines.push_back(text("  •  @ : Agente / Jugador"));
        help_lines.push_back(text("  •  . : Celda Vacía"));
        help_lines.push_back(text("  •  # : Pared / Obstáculo"));
        help_lines.push_back(text("  •  ~ : Terreno Difícil"));
        help_lines.push_back(text("  •  B : Batería"));
        help_lines.push_back(text("  •  T : Trampa"));
        help_lines.push_back(text("  •  S : Salida / Objetivo"));
    }

    return vbox(std::move(help_lines)) 
           | border 
           | size(WIDTH, LESS_THAN, 60);
    }
