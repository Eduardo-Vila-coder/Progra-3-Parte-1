#pragma once
#include <concepts>
#include <cstddef>
#include <cstdint>
#include <iterator>
#include <memory>
#include <optional>
#include <random>
#include <span>
#include <string_view>
#include <utility>
#include "observation.hpp"
#include "position.hpp"

// --- TEMPLATE DE FUNCIÓN (enunciado 6.1) ---
// Selecciona el mejor elemento de un rango según una función de costo: el mejor es el de MENOR costo.
// Si hay empate gana el primero del rango, así el resultado es determinista.
// Recibe el rango mediante iteradores, por eso sirve igual para vector, list u otro contenedor.
// Si el rango está vacío retorna last (como los algoritmos de <algorithm>).
template<typename Policy>
concept NavigationPolicy = requires(Policy& policy,
    const Observation& observation,
    std::span<const Action> actions) {
    { policy.selectAction(observation, actions) } -> std::same_as<Action>;
};

template <std::forward_iterator Iterator, typename Cost>
Iterator bestBy(Iterator first, Iterator last, Cost cost) {
    if (first == last) {
        return last;
    }

    Iterator best = first;
    auto bestCost = cost(*first);

    for (++first; first != last; ++first) {
        auto currentCost = cost(*first);
        if (currentCost < bestCost) { // "<" estricto: en empate se queda el primero
            best = first;
            bestCost = currentCost;
        }
    }
    return best;
}

// Distancia Manhattan entre dos posiciones: |fila1 - fila2| + |columna1 - columna2|
std::size_t manhattanDistance(Position a, Position b);

// --- POLÍTICA ALEATORIA (enunciado 5.6 y 5.7) ---
// Elige una acción legal al azar. Posee su propio generador con semilla controlable,
// así la misma semilla produce siempre la misma secuencia de acciones.
class RandomPolicy {
public:
    explicit RandomPolicy(std::uint32_t seed);

    Action selectAction(const Observation& observation, std::span<const Action> legalActions);

private:
    std::mt19937 engine_;
};

// --- POLÍTICA HEURÍSTICA (enunciado 5.6 y 5.7) ---
// Elige el movimiento legal que deja al agente más cerca de la salida (distancia Manhattan).
// - Solo considera acciones legales, por eso nunca choca con un muro ni sale del tablero.
// - No elige wait si existe algún movimiento (esperar gasta energía sin acercarse).
// - Evita volver a la casilla anterior, salvo que sea la única opción (reduce rebotes en callejones).
class HeuristicPolicy {
public:
    Action selectAction(const Observation& observation, std::span<const Action> legalActions);

private:
    std::optional<Position> previous_; // casilla donde estaba el agente antes del último movimiento
};

static_assert(NavigationPolicy<RandomPolicy>);
static_assert(NavigationPolicy<HeuristicPolicy>);


// =====================================================================================
// --- POLIMORFISMO DINÁMICO: IController (enunciado 5.6, 5.7 y 6.5) ---
// Interfaz común para cualquier controlador. El juego guarda un std::unique_ptr<IController>
// y llama a selectAction sin saber qué controlador es: el despacho dinámico ocurre en
// la llamada virtual. No se usa typeid, dynamic_cast ni condicionales según el tipo.
class IController {
public:
    virtual ~IController() = default;
    virtual Action selectAction(const Observation& observation, std::span<const Action> legalActions) = 0;
};

// --- ADAPTADOR GENÉRICO: PolicyController<Policy> ---
// Convierte cualquier política que cumpla el concept NavigationPolicy (se verifica al compilar)
// en un IController (se puede cambiar en ejecución). Así se combinan templates, concepts y
// una interfaz virtual: RandomPolicy y HeuristicPolicy no heredan de nada.
template <NavigationPolicy Policy>
class PolicyController final : public IController {
public:
    explicit PolicyController(Policy policy) : policy_(std::move(policy)) {}

    Action selectAction(const Observation& observation, std::span<const Action> legalActions) override {
        return policy_.selectAction(observation, legalActions);
    }

private:
    Policy policy_;
};

// --- CONTROLADOR HUMANO ---
// Recibe la decisión que le entrega la interfaz (provide) y la devuelve en selectAction.
// No lee std::cin: la consola traduce la tecla y se la pasa. Puede devolver una acción
// que no es legal (por ejemplo, chocar contra un muro): el entorno aplica la regla de rechazo.
class HumanController final : public IController {
public:
    void provide(Action action) noexcept { pending_ = action; }
    [[nodiscard]] bool hasDecision() const noexcept { return pending_.has_value(); }

    // Lanza std::logic_error si la interfaz no entregó ninguna decisión
    Action selectAction(const Observation& observation, std::span<const Action> legalActions) override;

private:
    std::optional<Action> pending_;
};

// --- FÁBRICA DE CONTROLADORES AUTOMÁTICOS ---
enum class ControllerKind { random, heuristic };

// Crea el controlador pedido. La semilla solo la usa el aleatorio (simulación reproducible).
std::unique_ptr<IController> makeController(ControllerKind kind, std::uint32_t seed);

// Convierte "random" / "heuristic" en ControllerKind; std::nullopt si el texto no corresponde
std::optional<ControllerKind> controllerKindFrom(std::string_view name);
