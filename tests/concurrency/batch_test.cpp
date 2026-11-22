#include <atomic>
#include <cassert>
#include <iostream>
#include <stdexcept>
#include <string>
#include <variant>
#include <vector>

#include "circuit_escape/map_generation/map_batch_runner.hpp"

namespace
{
constexpr std::size_t Rows = 5;
constexpr std::size_t Columns = 5;
using TestMap = BuiltMap<Rows, Columns>;

// Costo ficticio y determinista: solo se usa para provocar errores y comprobar el
// orden de fases. El lote determinista usa MazeMapValidator (Dijkstra real).
int fakeCost(std::size_t id)
{
    if (id == 1) return 10;
    if (id == 2) return 18;
    if (id == 3) return 32;
    return static_cast<int>(20 + id);
}

ValidationResult fakeValidation(const TestMap& map)
{
    return ValidationResult{map.id, fakeCost(map.id), true, {}};
}

bool sameWalls(const TestMap& a, const TestMap& b)
{
    for (std::size_t row = 0; row < Rows; ++row) {
        for (std::size_t column = 0; column < Columns; ++column) {
            const bool wallA = std::holds_alternative<Wall>(a.grid.at({row, column}));
            const bool wallB = std::holds_alternative<Wall>(b.grid.at({row, column}));
            if (wallA != wallB) {
                return false;
            }
        }
    }
    return true;
}

bool sameOpenings(const std::vector<Opening>& a, const std::vector<Opening>& b)
{
    if (a.size() != b.size()) {
        return false;
    }
    for (std::size_t i = 0; i < a.size(); ++i) {
        if (a[i].wall != b[i].wall || a[i].cellA != b[i].cellA ||
            a[i].cellB != b[i].cellB || a[i].weight != b[i].weight) {
            return false;
        }
    }
    return true;
}
} // namespace

int main()
{
    MazeMapValidator<Rows, Columns> validator;

    // 1. Un trabajador y cuatro trabajadores deben producir exactamente el mismo lote,
    //    validado con el Dijkstra real sobre los mapas de MapBuilder.
    {
        BatchConfig single{3, 1, 42, 2};
        BatchConfig parallel{3, 4, 42, 2};
        const auto one = run_map_batch<Rows, Columns>(single, validator);
        const auto many = run_map_batch<Rows, Columns>(parallel, validator);

        assert(one.maps.size() == 3 && many.maps.size() == 3);
        assert(one.errors == std::vector<std::string>(3, ""));
        assert(many.errors == std::vector<std::string>(3, ""));

        for (std::size_t i = 0; i < 3; ++i) {
            assert(one.maps[i].id == many.maps[i].id);
            assert(one.maps[i].seed == many.maps[i].seed);
            assert(sameWalls(one.maps[i], many.maps[i]));
            assert(sameOpenings(one.maps[i].treeOpenings, many.maps[i].treeOpenings));
            assert(sameOpenings(one.maps[i].extraOpenings, many.maps[i].extraOpenings));
            assert(one.validations[i].has_value() && many.validations[i].has_value());
            assert(one.validations[i]->id == many.validations[i]->id);
            assert(one.validations[i]->cost > 0);
            assert(one.validations[i]->hasVictory);
            assert(one.validations[i]->cost == many.validations[i]->cost);
        }

        // La clasificación debe ser idéntica con 1 y con 4 trabajadores. Los costos
        // reales dependen del laberinto; si hay tres costos distintos deben quedar ordenados.
        assert(one.selection.status == many.selection.status);
        assert(one.selection.easy.id == many.selection.easy.id);
        assert(one.selection.medium.id == many.selection.medium.id);
        assert(one.selection.hard.id == many.selection.hard.id);
        if (one.selection.status == SelectionStatus::Ok) {
            assert(one.selection.easy.cost < one.selection.medium.cost);
            assert(one.selection.medium.cost < one.selection.hard.cost);
        }
    }

    // 2. Configuraciones inválidas se rechazan con mensaje claro.
    {
        auto rejects = [&validator](const BatchConfig& config) {
            try {
                (void)run_map_batch<Rows, Columns>(config, validator);
            } catch (const std::invalid_argument&) {
                return true;
            }
            return false;
        };
        assert(rejects(BatchConfig{0, 1, 42, 0}));
        assert(rejects(BatchConfig{11, 1, 42, 0}));
        assert(rejects(BatchConfig{3, 0, 42, 0}));
    }

    // 3. Un fallo de validación se captura por mapa y no detiene el lote.
    {
        FunctionMapValidator<Rows, Columns> failingValidator{[](const TestMap& map) {
            if (map.id == 2) {
                throw std::runtime_error("fallo de prueba en el mapa 2");
            }
            return fakeValidation(map);
        }};

        const auto outcome = run_map_batch<Rows, Columns>(BatchConfig{3, 3, 9, 0},
                                                          failingValidator);
        assert(outcome.errors[0].empty() && outcome.errors[2].empty());
        assert(outcome.errors[1].find("validacion: fallo de prueba en el mapa 2") !=
               std::string::npos);
        assert(outcome.validations[0].has_value() && !outcome.validations[1].has_value() &&
               outcome.validations[2].has_value());
        // Quedan los costos 10 y 32: no hay tres costos distintos.
        assert(outcome.selection.status == SelectionStatus::NotEnoughDistinctCosts);
    }

    // 4. La validación no empieza hasta que se han construido todos los mapas.
    {
        std::atomic<std::size_t> built{0};
        BatchConfig config{3, 3, 7, 0};
        config.onMapBuilt = [&built](std::size_t) { built.fetch_add(1); };

        FunctionMapValidator<Rows, Columns> orderedValidator{[&built](const TestMap& map) {
            assert(built.load() == 3);
            return fakeValidation(map);
        }};

        const auto outcome = run_map_batch<Rows, Columns>(config, orderedValidator);
        assert(built.load() == 3);
        assert(outcome.selection.status == SelectionStatus::Ok);
    }

    std::cout << "batch_test: todas las pruebas pasaron\n";
    return 0;
}
