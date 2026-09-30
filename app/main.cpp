// Circuito de Escape - ejecutable interactivo (enunciado 5.8)
//
// Uso:
//   navigation_game [--ascii] [--difficulty easy|standard|hard] [--scenario 1|2]
//                   [--controller human|random|heuristic] [--seed N] [--headless]
//
// Controles (humano): W A S D (o flechas) para moverse, E esperar, H ayuda, Q salir.
// Controles (automático): Espacio, Enter o N avanza un turno; H ayuda; Q salir.
// --headless: juega la partida completa con el controlador automático sin abrir la interfaz
//             e imprime el resultado (simulación reproducible con la misma semilla).

#include <cstdint>
#include <cstring>
#include <memory>
#include <optional>
#include <exception>
#include <iostream>
#include <string>

#include <ftxui/component/component.hpp>
#include <ftxui/component/screen_interactive.hpp>

#include "circuit_escape/controllers.hpp"
#include "circuit_escape/environment.hpp"
#include "circuit_escape/game_rules.hpp"
#include "circuit_escape/scenario.hpp"
#include "console_ui.hpp"

namespace {

struct Options {
    RenderMode mode{RenderMode::emoji};
    Difficulty difficulty{Difficulty::Standard};
    std::string scenarioFile{scenario01File};
    std::optional<ControllerKind> controller;  // nullopt = humano
    std::uint32_t seed{42};
    bool headless{false};
};

void printUsage() {
    std::cout << "Uso: navigation_game [--ascii] [--difficulty easy|standard|hard] [--scenario 1|2]\n"
                 "                     [--controller human|random|heuristic] [--seed N] [--headless]\n";
}

// Lee los argumentos de la línea de comandos. Lanza std::invalid_argument si alguno no es válido.
Options parseOptions(int argc, char* argv[]) {
    Options options;
    for (int i = 1; i < argc; ++i) {
        const std::string argument = argv[i];
        const bool hasValue = i + 1 < argc;

        if (argument == "--ascii") {
            options.mode = RenderMode::ascii;
        } else if (argument == "--difficulty" && hasValue) {
            const std::string level = argv[++i];
            if (level == "easy") options.difficulty = Difficulty::Easy;
            else if (level == "standard") options.difficulty = Difficulty::Standard;
            else if (level == "hard") options.difficulty = Difficulty::Hard;
            else throw std::invalid_argument("dificultad desconocida: " + level);
        } else if (argument == "--scenario" && hasValue) {
            const std::string number = argv[++i];
            if (number == "1") options.scenarioFile = scenario01File;
            else if (number == "2") options.scenarioFile = scenario02File;
            else throw std::invalid_argument("escenario desconocido: " + number);
        } else if (argument == "--controller" && hasValue) {
            const std::string name = argv[++i];
            if (name == "human") {
                options.controller.reset();
            } else if (auto kind = controllerKindFrom(name)) {
                options.controller = *kind;
            } else {
                throw std::invalid_argument("controlador desconocido: " + name);
            }
        } else if (argument == "--seed" && hasValue) {
            options.seed = static_cast<std::uint32_t>(std::stoul(argv[++i]));
        } else if (argument == "--headless") {
            options.headless = true;
        } else {
            throw std::invalid_argument("opción desconocida: " + argument);
        }
    }
    if (options.headless && !options.controller) {
        throw std::invalid_argument("--headless necesita --controller random o heuristic");
    }
    return options;
}

// Partida automática completa sin FTXUI: solo usa la interfaz IController
int runHeadless(DemoEnvironment& environment, IController& controller) {
    StepResult last{};
    while (!environment.isFinished()) {
        const Observation observation = environment.state();
        last = environment.step(controller.selectAction(observation, observation.availableActions));
    }
    std::cout << finalSummary(last.reason, last.observation) << "\n";
    return 0;
}

}  // namespace

int main(int argc, char* argv[]) {
    Options options;
    try {
        options = parseOptions(argc, argv);
    } catch (const std::exception& error) {
        std::cerr << "Error: " << error.what() << "\n";
        printUsage();
        return 1;
    }

    try {
        const auto scenario = loadScenario<kRows, kColumns>(scenarioPath(options.scenarioFile));
        DemoEnvironment environment = makeEnvironment(scenario.grid, scenario.start, options.difficulty);
        environment.reset(options.seed);

        // Controlador automático elegido en ejecución (nullptr = jugador humano)
        std::unique_ptr<IController> automatic;
        if (options.controller) {
            automatic = makeController(*options.controller, options.seed);
        }

        if (options.headless) {
            return runHeadless(environment, *automatic);
        }

        const ConsoleUI ui{options.mode};
        GameSession session{environment, ui, std::move(automatic)};

        auto screen = ftxui::ScreenInteractive::FitComponent();

        // La UI solo dibuja lo que produce el entorno
        auto component = ftxui::Renderer([&] { return session.render(); });

        // Cada pulsación llega como un ftxui::Event, sin prompt ni Enter
        component = ftxui::CatchEvent(component, [&](const ftxui::Event& event) {
            if (session.handle(event)) {
                screen.ExitLoopClosure()();
            }
            return true;
        });

        screen.Loop(component);
    } catch (const std::exception& error) {
        std::cerr << "Error: " << error.what() << "\n";
        return 1;
    }
    return 0;
}
