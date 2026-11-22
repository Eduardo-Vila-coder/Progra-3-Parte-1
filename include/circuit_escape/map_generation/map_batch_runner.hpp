#pragma once
#include <algorithm>
#include <atomic>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <optional>
#include <stdexcept>
#include <string>
#include <thread>
#include <utility>
#include <vector>

#include "circuit_escape/map_generation/map_builder.hpp"
#include "circuit_escape/map_generation/map_layout.hpp"
#include "circuit_escape/map_generation/map_selector.hpp"
#include "circuit_escape/pathfinding/logica_algoritmo.hpp"

// Resultado de validar un mapa. Es el contrato que deberá devolver el MapValidator
// de Mathius; mientras tanto permite probar el coordinador con un validador de prueba.
struct ValidationResult
{
    std::size_t id{};      // el mismo identificador del BuiltMap
    int cost{};            // menor gasto de los dos recorridos completos
    bool hasVictory{};     // existe partida ganadora comprobada
    std::string message;   // diagnóstico opcional (recargas, objetivo inaccesible, ...)
};

// Interfaz del validador. Mathius puede implementarla cuando termine su parte.
template <std::size_t Rows, std::size_t Columns>
class IMapValidator
{
public:
    virtual ~IMapValidator() = default;
    [[nodiscard]] virtual ValidationResult validate(const BuiltMap<Rows, Columns>& map) = 0;
};

// Adaptador para usar una lambda como validador (útil en las pruebas y mientras
// no exista la implementación definitiva).
template <std::size_t Rows, std::size_t Columns>
class FunctionMapValidator : public IMapValidator<Rows, Columns>
{
public:
    using Callback = std::function<ValidationResult(const BuiltMap<Rows, Columns>&)>;

    explicit FunctionMapValidator(Callback callback) : callback_(std::move(callback)) {
        if (!callback_) {
            throw std::invalid_argument("El validador necesita una funcion");
        }
    }

    [[nodiscard]] ValidationResult validate(const BuiltMap<Rows, Columns>& map) override {
        return callback_(map);
    }

private:
    Callback callback_;
};

// Validador real para los mapas de MapBuilder: usa el Dijkstra de Mathius sobre el
// grafo implícito del tablero. Como el generador solo produce celdas Empty/Wall,
// entrar en cualquier casilla cuesta 1; el costo devuelto es el de ir de la primera
// sala (0,0) a la última sala (fila/columna par), que es donde se coloca la salida.
// Es el adaptador que sustituye al validador falso de las pruebas.
template <std::size_t Rows, std::size_t Columns>
class MazeMapValidator : public IMapValidator<Rows, Columns>
{
public:
    [[nodiscard]] ValidationResult validate(const BuiltMap<Rows, Columns>& map) override
    {
        struct Node
        {
            int peso_{1};
            bool pisable{true};
            std::size_t row_{};
            std::size_t column_{};

            std::size_t id(std::size_t columns) const noexcept
            {
                return row_ * columns + column_;
            }
        };

        struct Nodes
        {
            const Grid<Cell, Rows, Columns>& grid;

            std::size_t filas() const noexcept { return Rows; }
            std::size_t columnas() const noexcept { return Columns; }

            std::optional<Node> construir(std::size_t row, std::size_t column) const
            {
                if (row >= Rows || column >= Columns) {
                    return std::nullopt;
                }
                return Node{1, isTraversable(grid.at({row, column})), row, column};
            }
        };

        Nodes nodes{map.grid};
        const auto start = nodes.construir(0, 0);
        const auto goal = nodes.construir(map_layout::lastRoomIndex(Rows),
                                          map_layout::lastRoomIndex(Columns));
        if (!start || !start->pisable || !goal || !goal->pisable) {
            return ValidationResult{map.id, -1, false, "inicio o salida no transitables"};
        }

        std::vector<std::pair<int, int>> path;
        const int cost = Disjtrak{}.buscar(nodes, *start, *goal, path);
        if (cost < 0) {
            return ValidationResult{map.id, -1, false, "la salida no es alcanzable"};
        }
        return ValidationResult{map.id, cost, true, {}};
    }
};

// Configuración del lote. Las dimensiones Rows/Columns son parámetros de plantilla
// del BatchOutcome, por lo que no se repiten aquí.
struct BatchConfig
{
    std::size_t mapCount{3};      // hasta 10 alternativas (enunciado §2.3)
    std::size_t workers{1};       // número configurable de std::jthread, incluido 1
    unsigned baseSeed{1};         // de aquí se derivan las semillas por mapa
    std::size_t extraOpenings{0}; // aperturas adicionales a las de Kruskal

    // Instrumentación opcional: se llama tras construir cada mapa, desde el hilo
    // que lo construyó. Permite comprobar que la validación empieza después.
    std::function<void(std::size_t index)> onMapBuilt{};
};

// Resultado de una preparación completa.
template <std::size_t Rows, std::size_t Columns>
struct BatchOutcome
{
    std::vector<BuiltMap<Rows, Columns>> maps;                  // indexado por posición del lote
    std::vector<std::optional<ValidationResult>> validations;   // vacío si esa tarea falló
    std::vector<std::string> errors;                            // vacío si esa tarea funcionó
    SelectionResult selection{};                                // clasificación por nivel
    std::chrono::milliseconds constructionTime{};
    std::chrono::milliseconds validationTime{};
};

namespace batch_detail
{
// Semilla distinta y reproducible por mapa: depende solo de la semilla base y del id.
inline unsigned seedFor(unsigned baseSeed, std::size_t id) noexcept
{
    std::uint64_t z = static_cast<std::uint64_t>(baseSeed) +
                      0x9e3779b97f4a7c15ULL * static_cast<std::uint64_t>(id);
    z = (z ^ (z >> 30)) * 0xbf58476d1ce4e5b9ULL;
    z = (z ^ (z >> 27)) * 0x94d049bb133111ebULL;
    return static_cast<unsigned>(z ^ (z >> 31));
}

// Reparte índices con un contador atómico. Cada tarea escribe solo su posición.
template <class Task>
void runWorkers(std::size_t workers, std::size_t taskCount, Task& task)
{
    std::atomic<std::size_t> next{0};
    std::vector<std::jthread> threads;
    threads.reserve(workers);

    for (std::size_t worker = 0; worker < workers; ++worker) {
        threads.emplace_back([&next, taskCount, &task] {
            for (;;) {
                const std::size_t index = next.fetch_add(1, std::memory_order_relaxed);
                if (index >= taskCount) {
                    return;
                }
                task(index);
            }
        });
    }
    // Los jthread se unen al destruirse, antes de volver de esta función.
}
} // namespace batch_detail

// Nombres sugeridos por el enunciado (§6.5). build_map usa el MapBuilder de Eduardo.
template <std::size_t Rows, std::size_t Columns>
BuiltMap<Rows, Columns> build_map(std::size_t id, unsigned seed, std::size_t extraCount)
{
    return MapBuilder<Rows, Columns>::build(id, seed, extraCount);
}

template <std::size_t Rows, std::size_t Columns>
ValidationResult validate_map(IMapValidator<Rows, Columns>& validator,
                              const BuiltMap<Rows, Columns>& map)
{
    return validator.validate(map);
}

// Construye todas las alternativas en paralelo, espera, valida todas en paralelo,
// espera y clasifica en orden. Los errores se capturan por mapa y no detienen el lote.
template <std::size_t Rows, std::size_t Columns>
BatchOutcome<Rows, Columns> run_map_batch(const BatchConfig& config,
                                          IMapValidator<Rows, Columns>& validator)
{
    if (config.mapCount == 0) {
        throw std::invalid_argument("El lote debe tener al menos un mapa");
    }
    if (config.mapCount > 10) {
        throw std::invalid_argument("El lote admite como maximo diez mapas");
    }
    if (config.workers == 0) {
        throw std::invalid_argument("El numero de trabajadores debe ser al menos uno");
    }

    BatchOutcome<Rows, Columns> outcome;
    outcome.maps.resize(config.mapCount);
    outcome.validations.assign(config.mapCount, std::nullopt);
    outcome.errors.assign(config.mapCount, {});

    // Identificadores y semillas se preparan antes de repartir trabajo.
    std::vector<std::size_t> ids(config.mapCount);
    std::vector<unsigned> seeds(config.mapCount);
    for (std::size_t i = 0; i < config.mapCount; ++i) {
        ids[i] = i + 1; // identificador estable, no depende del orden de finalización
        seeds[i] = batch_detail::seedFor(config.baseSeed, ids[i]);
    }

    const std::size_t threadCount = std::min(config.workers, config.mapCount);

    // --- Fase 1: construcción con Kruskal, en paralelo ---
    const auto buildStart = std::chrono::steady_clock::now();
    auto buildTask = [&](std::size_t index) {
        try {
            outcome.maps[index] = build_map<Rows, Columns>(ids[index], seeds[index],
                                                           config.extraOpenings);
            if (config.onMapBuilt) {
                config.onMapBuilt(index);
            }
        } catch (const std::exception& error) {
            outcome.errors[index] = std::string("construccion: ") + error.what();
        } catch (...) {
            outcome.errors[index] = "construccion: error desconocido";
        }
    };
    batch_detail::runWorkers(threadCount, config.mapCount, buildTask);
    outcome.constructionTime = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now() - buildStart);

    // --- Fase 2: validación con Dijkstra, en paralelo, solo tras la fase 1 ---
    const auto validateStart = std::chrono::steady_clock::now();
    auto validateTask = [&](std::size_t index) {
        if (!outcome.errors[index].empty()) {
            return; // un mapa que no se pudo construir no se valida
        }
        try {
            outcome.validations[index] = validate_map(validator, outcome.maps[index]);
        } catch (const std::exception& error) {
            outcome.errors[index] = std::string("validacion: ") + error.what();
        } catch (...) {
            outcome.errors[index] = "validacion: error desconocido";
        }
    };
    batch_detail::runWorkers(threadCount, config.mapCount, validateTask);
    outcome.validationTime = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now() - validateStart);

    // --- Fase 3: clasificación en orden, sin hilos ---
    std::vector<ValidatedMap> candidates;
    candidates.reserve(config.mapCount);
    for (std::size_t i = 0; i < config.mapCount; ++i) {
        if (!outcome.errors[i].empty() || !outcome.validations[i].has_value()) {
            continue;
        }
        const ValidationResult& validation = *outcome.validations[i];
        if (validation.hasVictory) {
            candidates.push_back(ValidatedMap{validation.id, validation.cost, true});
        }
    }
    outcome.selection = MapSelector::select(candidates);
    return outcome;
}
