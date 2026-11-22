// Prueba del escritor de escenarios del MapBatchRunner: kruskal -> Dijkstra ->
// colocación de elementos -> .txt -> carga con scenario.hpp (sin duplicar el test de Eduardo).
#include <cassert>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <sstream>
#include <string>
#include <variant>
#include <vector>

#include "circuit_escape/map_generation/map_batch_runner.hpp"
#include "circuit_escape/map_generation/map_layout.hpp"
#include "circuit_escape/map_generation/map_writer.hpp"
#include "circuit_escape/scenario.hpp"

namespace {

constexpr std::size_t Rows = 5;
constexpr std::size_t Columns = 5;

std::string readFile(const std::filesystem::path& path)
{
    std::ifstream file(path);
    assert(file.good());
    return {std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>()};
}

int countSymbol(const std::string& content, char symbol)
{
    int count = 0;
    for (const char current : content) {
        if (current == symbol) {
            ++count;
        }
    }
    return count;
}

} // namespace

int main()
{
    const std::filesystem::path directory = std::filesystem::temp_directory_path();
    const std::filesystem::path first = directory / "circuit_escape_writer_a.txt";
    const std::filesystem::path second = directory / "circuit_escape_writer_b.txt";
    std::filesystem::remove(first);
    std::filesystem::remove(second);

    const auto built = MapBuilder<Rows, Columns>::build(7, 123, 1);
    MazeMapValidator<Rows, Columns> validator;
    const ValidationResult validation = validator.validate(built);
    assert(validation.id == 7 && validation.hasVictory && validation.cost > 0);

    BuiltMap<Rows, Columns> map = built;
    const map_layout::MapEntities entities = map_layout::placeEntities(map);
    assert((entities.start == Position{0, 0}));
    assert((entities.goal == Position{4, 4}));
    assert(entities.switchA != entities.switchB);
    assert(entities.guard != entities.start && entities.guard != entities.goal &&
           entities.guard != entities.switchA && entities.guard != entities.switchB);

    map_writer::saveScenario(first, map, entities, "prueba", validation.cost);
    map_writer::saveScenario(second, map, entities, "prueba", validation.cost);

    const std::string content = readFile(first);
    assert(content == readFile(second)); // determinismo: misma semilla -> mismo archivo

    // Los metadatos son comentarios ';' y el cargador debe ignorarlos.
    assert(content.find("; id=7\n") != std::string::npos);
    assert(content.find("; semilla=123\n") != std::string::npos);
    assert(content.find("; nivel=prueba\n") != std::string::npos);
    assert(content.find("; filas=5\n") != std::string::npos);
    assert(content.find("; columnas=5\n") != std::string::npos);
    assert(content.find("; costo_minimo=") != std::string::npos);

    const Scenario<Rows, Columns> loaded = loadScenario<Rows, Columns>(first.string());
    assert(loaded.start == entities.start);
    assert(!loaded.guard.has_value() || *loaded.guard == entities.guard);

    const Cell& goal = loaded.grid.at(entities.goal);
    const Cell& switchA = loaded.grid.at(entities.switchA);
    const Cell& switchB = loaded.grid.at(entities.switchB);
    assert(std::holds_alternative<Exit>(goal));
    assert(std::holds_alternative<Switch>(switchA) &&
           std::get<Switch>(switchA).id == SwitchId::A);
    assert(std::holds_alternative<Switch>(switchB) &&
           std::get<Switch>(switchB).id == SwitchId::B);
    assert(std::holds_alternative<Empty>(loaded.grid.at(entities.start)));
    assert(std::holds_alternative<Empty>(loaded.grid.at(entities.guard)));

    // Una sala por símbolo especial y exactamente 5 filas de tablero (sin contar comentarios).
    std::string gridOnly;
    std::istringstream lines(content);
    std::string line;
    while (std::getline(lines, line)) {
        if (!line.empty() && line.front() != ';') {
            gridOnly += line;
        }
    }
    assert(countSymbol(gridOnly, '@') == 1);
    assert(countSymbol(gridOnly, 'S') == 1);
    assert(countSymbol(gridOnly, 'a') == 1);
    assert(countSymbol(gridOnly, 'b') == 1);
    assert(countSymbol(gridOnly, 'V') == 1);

    std::filesystem::remove(first);
    std::filesystem::remove(second);
    return 0;
}
