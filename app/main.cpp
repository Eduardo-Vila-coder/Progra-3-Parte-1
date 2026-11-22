//
// Created by LucasMCgamer on 13/09/2026.
//
// Circuito de Escape: aplicación de consola (enunciado 5.8). Coordina la partida:
// pide una acción (humana o automática), llama a step y vuelve a dibujar.
//
// Uso:
//   escape_circuit [--ascii] [--difficulty easy|standard|hard] [--scenario 1|2] [--level facil|medio|dificil]
//                   [--controller human|random|heuristic] [--seed N] [--headless]
//
//   --ascii       dibuja con caracteres ASCII en vez de emojis
//   --difficulty  perfil de reglas (por defecto standard)
//   --scenario    mapa de 20 x 30 (1: ejemplo del enunciado, 2: dos rutas)
//   --controller  human (teclado, por defecto), random o heuristic (automáticos)
//   --seed        semilla de la partida y del controlador aleatorio (por defecto 42)
//   --headless    juega la partida automática completa sin interfaz e imprime el resultado

#include <cstdint>
#include <exception>
#include <filesystem>
#include <iostream>
#include <memory>
#include <optional>
#include <stdexcept>
#include <string>
#include <variant>
#include <vector>

#include <ftxui/component/component.hpp>
#include <ftxui/component/screen_interactive.hpp>
#include <ftxui/dom/elements.hpp>

#include "circuit_escape/console_ui.hpp"
#include "circuit_escape/controllers.hpp"
#include "circuit_escape/controllers/guard_controller.hpp"
#include "circuit_escape/graph/graph.hpp"
#include "circuit_escape/environment.hpp"
#include "circuit_escape/game_rules.hpp"
#include "circuit_escape/scenario.hpp"

#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#endif

namespace {

using DemoEnvironment = NavigationEnvironment<20, 30>;

enum class ControllerKind { human, random, heuristic };

// Opciones de la línea de comandos
struct Options {
    RenderMode mode{RenderMode::emoji};
    Difficulty difficulty{Difficulty::Standard};
    std::string scenarioFile{scenario01File};
    ControllerKind controller{ControllerKind::human};
    std::uint32_t seed{42};
    bool headless{false};
};

void printUsage() {
    std::cout << "Uso: escape_circuit [--ascii] [--difficulty easy|standard|hard] [--scenario 1|2]\n"
                 "                      [--level facil|medio|dificil] [--controller human|random|heuristic]\n"
                 "                      [--seed N] [--headless]\n";
}

// Lee los argumentos. Lanza std::invalid_argument si alguno no es válido.
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
        } else if (argument == "--level" && hasValue) {
            const std::string level = argv[++i];
            if (level == "facil") options.scenarioFile = "mapa_facil.txt";
            else if (level == "medio") options.scenarioFile = "mapa_medio.txt";
            else if (level == "dificil") options.scenarioFile = "mapa_dificil.txt";
            else throw std::invalid_argument("nivel desconocido: " + level);
        } else if (argument == "--controller" && hasValue) {
            const std::string name = argv[++i];
            if (name == "human") options.controller = ControllerKind::human;
            else if (name == "random") options.controller = ControllerKind::random;
            else if (name == "heuristic") options.controller = ControllerKind::heuristic;
            else throw std::invalid_argument("controlador desconocido: " + name);
        } else if (argument == "--seed" && hasValue) {
            options.seed = static_cast<std::uint32_t>(std::stoul(argv[++i]));
        } else if (argument == "--headless") {
            options.headless = true;
        } else {
            throw std::invalid_argument("opcion desconocida: " + argument);
        }
    }
    if (options.headless && options.controller == ControllerKind::human) {
        throw std::invalid_argument("--headless necesita --controller random o heuristic");
    }
    return options;
}

// Controlador automático elegido en ejecución. Para el jugador humano devuelve nullptr:
// en ese caso la partida usa un HumanController al que la consola le entrega cada tecla.
std::unique_ptr<IController> makeController(ControllerKind kind, std::uint32_t seed) {
    switch (kind) {
    case ControllerKind::random:
        return std::make_unique<PolicyController<RandomPolicy>>(RandomPolicy{seed});
    case ControllerKind::heuristic:
        return std::make_unique<PolicyController<HeuristicPolicy>>(HeuristicPolicy{});
    case ControllerKind::human:
        break;
    }
    return nullptr;
}

// Partida automática completa sin interfaz: solo usa la interfaz IController (despacho dinámico)
int runHeadless(DemoEnvironment& environment, IController& controller, std::uint32_t seed) {
    StepResult last{};
    last.observation = environment.state();
    while (!environment.isFinished()) {
        const Observation observation = environment.state();
        last = environment.step(controller.selectAction(observation, observation.availableActions));
    }
    std::cout << "Semilla " << seed << ": " << finalSummary(last.reason, last.observation) << "\n";
    return 0;
}

// Partida interactiva con FTXUI: GameSession coordina el ciclo y FTXUI solo entrega teclas y dibuja
int runInteractive(DemoEnvironment& environment, std::unique_ptr<IController> automatic, RenderMode mode) {
    const ConsoleUI ui{mode};
    GameSession session{environment, ui, std::move(automatic)};

    auto screen = ftxui::ScreenInteractive::FitComponent();
    auto component = ftxui::Renderer([&] { return session.render(); });

    // Cada pulsación llega como un ftxui::Event, sin prompt ni Enter (lectura inmediata)
    component = ftxui::CatchEvent(component, [&](const ftxui::Event& event) {
        const KeyResult result = session.handle(event);
        if (result == KeyResult::quit) {
            screen.Exit();
        }
        return result != KeyResult::ignored;
    });

    screen.Loop(component);
    return 0;
}

// Ruta completa del ejecutable que se está ejecutando (para ubicar los mapas instalados)
std::filesystem::path executablePath(const char* argv0) {
#ifdef _WIN32
    wchar_t buffer[MAX_PATH];
    const DWORD length = GetModuleFileNameW(nullptr, buffer, MAX_PATH);
    if (length > 0 && length < MAX_PATH) {
        return std::filesystem::path(buffer);
    }
#endif
    std::error_code error;
    const std::filesystem::path path = std::filesystem::weakly_canonical(argv0, error);
    return error ? std::filesystem::path(argv0) : path;
}

}  // namespace

int main(int argc, char* argv[]) {
#ifdef _WIN32
    SetConsoleOutputCP(CP_UTF8);  // emojis y tildes en PowerShell / cmd.exe
#endif

    // Juego instalado: usar los mapas que el instalador copió junto al programa (proyecto 2, 8.1)
    if (const auto installedMaps = findMapsNextTo(executablePath(argv[0])); installedMaps.has_value()) {
        setMapsDirectory(installedMaps->string());
    }

    Options options;
    try {
        options = parseOptions(argc, argv);
    } catch (const std::exception& error) {
        std::cerr << "Error: " << error.what() << "\n";
        printUsage();
        return 1;
    }

    try {
        const auto scenario = loadScenario<20, 30>(scenarioPath(options.scenarioFile));
        DemoEnvironment environment = makeEnvironment(scenario.grid, scenario.start, options.difficulty);
        // Proyecto 2: si el mapa marca un vigilante ('V'), persigue al jugador con BFS sobre el grafo del mapa
        if (scenario.guard.has_value()) {
            using GuardBfs = BfsGuardPolicy<20, 30>;
            environment.setGuard(*scenario.guard,
                                 std::make_shared<GuardPolicyController<GuardBfs>>(
                                     GuardBfs{MapGraph<20, 30>{scenario.grid, GameRules<>{}}}));
        }
        environment.reset(options.seed);

        std::unique_ptr<IController> automatic = makeController(options.controller, options.seed);

        if (options.headless) {
            return runHeadless(environment, *automatic, options.seed);
        }
        return runInteractive(environment, std::move(automatic), options.mode);
    } catch (const std::exception& error) {
        std::cerr << "Error: " << error.what() << "\n";
        return 1;
    }
}
