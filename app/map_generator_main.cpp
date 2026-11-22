// Generador de mapas del proyecto 2 (enunciado 5.3 y 8.1).
// Usa MapBatchRunner: Kruskal (MapBuilder) -> Dijkstra (MazeMapValidator) ->
// MapSelector, y guarda los tres niveles como escenarios .txt reproducibles.

#include <algorithm>
#include <array>
#include <filesystem>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <string>

#include "circuit_escape/map_generation/map_batch_runner.hpp"
#include "circuit_escape/map_generation/map_layout.hpp"
#include "circuit_escape/map_generation/map_writer.hpp"
#include "circuit_escape/scenario.hpp"
#include "executable_path.hpp"

namespace {

struct Options
{
    unsigned seed{42};
    std::size_t mapCount{10};
    std::size_t workers{4};
    std::size_t extraOpenings{2};
    std::filesystem::path output{mapsDirectory()}; // la carpeta de mapas que usa el juego
    bool help{false};
};

unsigned long parseNumber(const std::string& text, const std::string& option)
{
    std::size_t consumed = 0;
    const unsigned long value = std::stoul(text, &consumed);
    if (consumed != text.size()) {
        throw std::invalid_argument(option + " esperaba un numero entero");
    }
    return value;
}

Options parseOptions(int argc, char* argv[])
{
    Options options;
    for (int i = 1; i < argc; ++i) {
        const std::string argument = argv[i];
        const auto takeValue = [&]() -> std::string {
            if (i + 1 >= argc) {
                throw std::invalid_argument(argument + " necesita un valor");
            }
            return argv[++i];
        };

        if (argument == "--help" || argument == "-h") {
            options.help = true;
        } else if (argument == "--seed") {
            const unsigned long value = parseNumber(takeValue(), argument);
            if (value > std::numeric_limits<unsigned>::max()) {
                throw std::invalid_argument("--seed fuera de rango");
            }
            options.seed = static_cast<unsigned>(value);
        } else if (argument == "--maps") {
            options.mapCount = static_cast<std::size_t>(parseNumber(takeValue(), argument));
        } else if (argument == "--workers") {
            options.workers = static_cast<std::size_t>(parseNumber(takeValue(), argument));
        } else if (argument == "--extra") {
            options.extraOpenings = static_cast<std::size_t>(parseNumber(takeValue(), argument));
        } else if (argument == "--output") {
            options.output = takeValue();
        } else {
            throw std::invalid_argument("opcion desconocida: " + argument);
        }
    }
    return options;
}

void printUsage()
{
    std::cout
        << "Uso: map_generator [opciones]\n"
        << "  --seed N       semilla base (por defecto 42)\n"
        << "  --maps N       cantidad de mapas a preparar, hasta 10 (por defecto 10)\n"
        << "  --workers N    trabajadores concurrentes (por defecto 4)\n"
        << "  --extra N      aperturas adicionales por mapa (por defecto 2)\n"
        << "  --output DIR   carpeta donde se escriben los .txt (por defecto la carpeta assets/maps\n"
        << "                 que usa el juego: la de la instalacion o la del proyecto)\n"
        << "  --help         muestra esta ayuda\n";
}

} // namespace

int main(int argc, char* argv[])
{
    // Igual que escape_circuit: el programa instalado usa la carpeta assets/maps de la
    // instalación, así los niveles que genera son los que carga el juego (proyecto 2, 8.1).
    // Sin instalar (CLion) es la carpeta assets/maps del proyecto.
    if (const auto installedMaps = findMapsNextTo(executablePath(argv[0])); installedMaps.has_value()) {
        setMapsDirectory(installedMaps->string());
    }

    Options options;
    try {
        options = parseOptions(argc, argv);
    } catch (const std::exception& error) {
        std::cerr << "map_generator: " << error.what() << '\n';
        printUsage();
        return 2;
    }

    if (options.help) {
        printUsage();
        return 0;
    }

    constexpr std::size_t Rows = 20;
    constexpr std::size_t Columns = 30;

    MazeMapValidator<Rows, Columns> validator;
    const BatchConfig config{options.mapCount, options.workers, options.seed,
                             options.extraOpenings};
    const BatchOutcome<Rows, Columns> outcome =
        run_map_batch<Rows, Columns>(config, validator);

    if (outcome.selection.status != SelectionStatus::Ok) {
        std::cerr << "map_generator: " << outcome.selection.message << '\n';
        for (std::size_t i = 0; i < outcome.errors.size(); ++i) {
            if (!outcome.errors[i].empty()) {
                std::cerr << "  mapa " << (i + 1) << ": " << outcome.errors[i] << '\n';
            }
        }
        return 1;
    }

    std::error_code directoryError;
    std::filesystem::create_directories(options.output, directoryError);
    if (directoryError) {
        std::cerr << "map_generator: no se pudo crear " << options.output << '\n';
        return 1;
    }

    const std::array<const ValidatedMap*, 3> chosen{
        &outcome.selection.easy, &outcome.selection.medium, &outcome.selection.hard};
    const std::array<const char*, 3> levels{"facil", "medio", "dificil"};
    const std::array<const char*, 3> fileNames{
        "mapa_facil.txt", "mapa_medio.txt", "mapa_dificil.txt"};

    try {
        for (std::size_t index = 0; index < chosen.size(); ++index) {
            const auto it = std::find_if(outcome.maps.begin(), outcome.maps.end(),
                                         [id = chosen[index]->id](const auto& candidate) {
                                             return candidate.id == id;
                                         });
            if (it == outcome.maps.end()) {
                throw std::runtime_error("no se encontro el mapa elegido para " +
                                         std::string(levels[index]));
            }

            BuiltMap<Rows, Columns> map = *it; // placeEntities modifica la copia
            const map_layout::MapEntities entities = map_layout::placeEntities(map);
            const std::filesystem::path path = options.output / fileNames[index];
            map_writer::saveScenario(path, map, entities, levels[index], chosen[index]->cost);

            std::cout << levels[index] << ": " << path.string()
                      << " (id " << map.id << ", semilla " << map.seed
                      << ", costo " << chosen[index]->cost << ")\n";
        }
    } catch (const std::exception& error) {
        std::cerr << "map_generator: " << error.what() << '\n'
                  << "Sugerencia: indique otra carpeta con --output (por ejemplo, en Documentos).\n";
        return 1;
    }

    std::cout << "Construccion: " << outcome.constructionTime.count() << " ms | Validacion: "
              << outcome.validationTime.count() << " ms\n";
    return 0;
}
