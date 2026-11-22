#pragma once
#include <algorithm>
#include <cstdint>
#include <iterator>
#include <memory>
#include <optional>
#include <stdexcept>
#include <type_traits>
#include <vector>
#include "cells.hpp"
#include "controllers/guard_controller.hpp"
#include "events.hpp"
#include "game_rules.hpp"
#include "grid.hpp"
#include "observation.hpp"
#include "overloaded.hpp"
#include "position.hpp"

template <std::size_t Rows, std::size_t Columns>
class NavigationEnvironment {
private:
    // Atributos originales segun el informe
    Grid<Cell, Rows, Columns>  initialGrid;
    Position start;
    int initialEnergy;
    std::size_t turnLimit;
    Grid<Cell, Rows, Columns>  originalGrid_; // Copia del tablero tal como empezó (para reset)

    // --- 5.3 AGENTE: Atributos mínimos exigidos ---
    Position agent_;
    int energy_{};
    int maxEnergy_{};
    int score_{0};
    std::size_t collectedResources_{0};
    bool active_{true};
    std::uint32_t seed_{0}; // Semilla del último reset (simulación reproducible)

    std::size_t turn_{0};

    // --- VIGILANTE (proyecto 2, Cristhian) ---
    // El entorno guarda su posición y es el único que la cambia; el controlador solo propone.
    std::shared_ptr<IGuardController> guardController_; // nullptr: partida sin vigilante
    Position guardStart_{};
    Position guard_{};

    bool hasGuard() const noexcept { return guardController_ != nullptr; }
    // El jugador y el vigilante ocupan la misma casilla
    bool guardCaptures() const noexcept { return hasGuard() && guard_ == agent_; }

    // Movimientos legales del vigilante: wait y los pasos que no chocan con muros ni bordes
    std::vector<Action> guardLegalActions() const {
        std::vector<Action> actions{Action::wait};
        for (const Action action : {Action::up, Action::down, Action::left, Action::right}) {
            const std::optional<Position> next = neighbor(guard_, action);
            if (next.has_value() && initialGrid.contains(*next) && isTraversable(initialGrid.at(*next))) {
                actions.push_back(action);
            }
        }
        return actions;
    }

    // Paso 3 del turno (3.2): en turnos pares el vigilante recalcula y avanza una casilla;
    // en impares permanece quieto. Una acción que no sea legal se convierte en espera.
    void moveGuard(std::vector<NavigationEvent>& events) {
        if (!hasGuard() || turn_ % 2 != 0) {
            return;
        }
        const std::vector<Action> legal = guardLegalActions();
        const Action proposed = guardController_->selectAction(GuardObservation{guard_, agent_, turn_}, legal);
        if (proposed == Action::wait || std::find(legal.begin(), legal.end(), proposed) == legal.end()) {
            return;
        }
        const Position from = guard_;
        guard_ = *neighbor(guard_, proposed);
        appendEvents(events, GuardMovedEvent{from, guard_});
    }

    // La captura termina la partida y tiene prioridad sobre el escape
    void capture(std::vector<NavigationEvent>& events) {
        endReason_ = EndReason::captured;
        active_ = false;
        appendEvents(events, PlayerCapturedEvent{agent_});
    }

    // --- CONDICIONES DE TÉRMINO ---
    // Motivo por el que terminó la partida (none = sigue activa)
    EndReason endReason_{EndReason::none};

    // Evalúa las condiciones de término respetando la precedencia del enunciado:
    // goalReached (en la salida, con energía y con A y B activados) > noEnergy > turnLimit
    // Pisar la salida antes de activar ambos interruptores no termina la partida (proyecto 2, 3.1).
    EndReason evaluateEnd() const {
        if (std::holds_alternative<Exit>(initialGrid.at(agent_)) && energy_ > 0 && switchesReady()) {
            return EndReason::goalReached;
        }
        if (energy_ == 0) {
            return EndReason::noEnergy;
        }
        if (turn_ >= turnLimit) {
            return EndReason::turnLimit;
        }
        return EndReason::none;
    }

    // Se llama al final de step, después de aplicar los efectos de la celda destino
    void checkEnd(std::vector<NavigationEvent>& events) {
        endReason_ = evaluateEnd();
        if (endReason_ == EndReason::none) {
            return;
        }

        active_ = false; // La partida terminó: el agente ya no puede actuar

        if (endReason_ == EndReason::goalReached) {
            appendEvents(events, GoalReachedEvent{agent_});
        }
    }

    // Busca la posición de la salida recorriendo el tablero en orden por filas
    Position goalPosition() const {
        auto it = std::find_if(initialGrid.begin(), initialGrid.end(), [](const Cell& cell) {
            return std::holds_alternative<Exit>(cell);
        });
        std::size_t index = static_cast<std::size_t>(std::distance(initialGrid.begin(), it));
        return Position{index / Columns, index % Columns};
    }

    // --- INTERRUPTORES A Y B (proyecto 2, Cristhian) ---
    // Cuántas casillas del tablero tienen el interruptor indicado
    std::size_t countSwitches(SwitchId id) const {
        return static_cast<std::size_t>(std::count_if(initialGrid.begin(), initialGrid.end(), [id](const Cell& cell) {
            const Switch* candidate = std::get_if<Switch>(&cell);
            return candidate != nullptr && candidate->id == id;
        }));
    }
    // Los mapas del proyecto 1 no tienen interruptores: ahí la salida funciona como antes
    bool hasSwitches() const {
        return countSwitches(SwitchId::A) > 0;
    }
    // La salida permite escapar solo con A y B activados (o si el mapa no tiene interruptores)
    bool switchesReady() const {
        return !hasSwitches() || (isSwitchActive(SwitchId::A) && isSwitchActive(SwitchId::B));
    }
    // Paso 4 del turno (proyecto 2, 3.2): si el agente terminó el turno sobre un interruptor
    // desactivado, se activa y queda así el resto de la partida. Cuando se agregue el vigilante,
    // su movimiento y la comprobación de captura van ANTES de esta llamada.
    // Con la energía agotada el turno termina en derrota y ya no se activa nada.
    void activateSwitch(std::vector<NavigationEvent>& events) {
        if (energy_ == 0) {
            return;
        }
        if (Switch* current = std::get_if<Switch>(&initialGrid.at(agent_)); current != nullptr && !current->active) {
            current->active = true;
            appendEvents(events, SwitchActivatedEvent{agent_, current->id});
        }
    }

    // --- ENERGÍA E INTERACCIONES (Cristhian) ---
    // Reglas de la partida (costos, recompensas, penalizaciones). Por ahora estándar.
    GameRules<> rules_{};

    // INVARIANTE: La energía se mantiene entre 0 y maxEnergy_.
    // Solo se emite EnergyChangedEvent si la energía cambió de verdad.
    void changeEnergy(int delta, std::vector<NavigationEvent>& events) {
        const int previous = energy_;
        energy_ = std::clamp(energy_ + delta, 0, maxEnergy_);
        if (energy_ != previous) {
            appendEvents(events, EnergyChangedEvent{previous, energy_});
        }
    }

    // Gastar energía es cambiarla en negativo
    void spendEnergy(int cost, std::vector<NavigationEvent>& events) {
        changeEnergy(-cost, events);
    }

    // Costo de entrar a una celda: el terreno elevado cuesta más, el resto cuesta lo general
    int entryCost(const Cell& cell) const {
        return std::visit(Overloaded{
            [this](const RoughTerrain&) { return rules_.energyCostRoughTerrain; },
            [this](const auto&) { return rules_.energyCostGeneral; },
        }, cell);
    }

    // Efecto de la celda a la que acaba de entrar el agente.
    // Se aplica aunque la energía haya quedado en 0 (una batería todavía puede recargar).
    void applyCellEffect(std::vector<NavigationEvent>& events) {
        // CellTraits (enunciado 6.3): un consumible ya usado se comporta como espacio libre
        if (isSpent(initialGrid.at(agent_))) {
            return;
        }
        std::visit(Overloaded{
            [](Empty&) {},
            [](Wall&) {},          // nunca se entra a un muro
            [](RoughTerrain&) {},  // su costo ya se cobró al entrar
            [](Exit&) {},          // la victoria la decide checkEnd
            [](Switch&) {},        // se activa al cerrar el turno (activateSwitch)
            [this, &events](ResourceCell<int>& resource) {
                resource.collected = true;               // se recoge una sola vez
                score_ += rules_.reward;
                ++collectedResources_;
                appendEvents(events, ResourceCollectedEvent{agent_, rules_.reward});
            },
            [this, &events](Battery& battery) {
                battery.consumed = true;                 // se consume una sola vez
                changeEnergy(rules_.energy, events);
            },
            [this, &events](Trap&) {                     // se activa CADA vez que se entra
                appendEvents(events, TrapTriggeredEvent{agent_});
                changeEnergy(-rules_.energyPenalty, events);
                score_ -= rules_.scorePenalty;           // el puntaje puede ser negativo
            },
        }, initialGrid.at(agent_));
    }

    // Precondiciones del constructor (enunciado 5.7). Lanza std::invalid_argument si alguna falla.
    void validate() const {
        if (energy_ <= 0) {
            throw std::invalid_argument("Error: La energia inicial debe ser positiva");
        }
        if (turnLimit == 0) {
            throw std::invalid_argument("Error: El limite de turnos debe ser positivo");
        }
        // INVARIANTE: La posición del agente siempre pertenece al tablero
        if (!initialGrid.contains(agent_) || !initialGrid.contains(start)) {
            throw std::invalid_argument("Error: El agente inicia fuera del tablero");
        }
        // INVARIANTE: El agente nunca ocupa una celda bloqueada
        if (!isTraversable(initialGrid.at(agent_)) || !isTraversable(initialGrid.at(start))) {
            throw std::invalid_argument("Error: El agente inicia en una celda no transitable");
        }
        // Existe exactamente una salida (y por estar en el Grid, pertenece al tablero)
        const auto exits = std::count_if(initialGrid.begin(), initialGrid.end(), [](const Cell& cell) {
            return std::holds_alternative<Exit>(cell);
        });
        if (exits != 1) {
            throw std::invalid_argument("Error: El tablero debe tener exactamente una salida");
        }
        // Proyecto 2: un interruptor A y uno B (o ninguno, en los mapas del proyecto 1)
        const std::size_t switchesA = countSwitches(SwitchId::A);
        const std::size_t switchesB = countSwitches(SwitchId::B);
        if (switchesA > 1 || switchesB > 1 || switchesA != switchesB) {
            throw std::invalid_argument("Error: El tablero debe tener un interruptor A y uno B");
        }
    }

    // Calculamos la posición destino. Retorna nullopt si el movimiento choca o se sale.
    // [Se usó IA para la lógica]
    std::optional<Position> targetOf(Action action) const {
        std::optional<Position> candidate = neighbor(agent_, action);

        // Si neighbor falló, o si se sale del tablero:
        if (!candidate.has_value() || !initialGrid.contains(*candidate)) {
            return std::nullopt;
        }

        // Si la celda destino no se puede atravesar (muro), según CellTraits:
        if (!isTraversable(initialGrid.at(*candidate))) {
            return std::nullopt;
        }

        return candidate; // El movimiento es válido
    }

public:
    // Constructor: Configura el entorno y verifica las invariantes
    // (los miembros se inicializan en el mismo orden en que están declarados)
    NavigationEnvironment(Grid<Cell, Rows, Columns> initialGrid, Position start, Position startPos, int startEnergy, std::size_t turnLimit)
        : initialGrid(initialGrid), start(start), initialEnergy(startEnergy), turnLimit(turnLimit),
          originalGrid_(initialGrid), agent_(startPos), energy_(startEnergy), maxEnergy_(startEnergy) {
        validate(); // precondiciones e invariantes iniciales (enunciado 5.7)
    }
    NavigationEnvironment(Grid<Cell, Rows, Columns> initialGrid, Position start, int initialEnergy, std::size_t turnLimit)
        : initialGrid(initialGrid), start(start), initialEnergy(initialEnergy), turnLimit(turnLimit),
          originalGrid_(initialGrid), agent_(start), energy_(initialEnergy), maxEnergy_(initialEnergy) {     // Constructor original
        validate(); // precondiciones e invariantes iniciales (enunciado 5.7)
    }

    // Constructor que recibe cualquier perfil de Eduardo: GameRules<Easy>, <Standard> o <Hard>.
    // Toma del perfil la energía inicial/máxima y el límite de turnos, y copia sus costos y recompensas.
    template <Difficulty Level>
    NavigationEnvironment(Grid<Cell, Rows, Columns> initialGrid, Position start, GameRules<Level> rules)
        : NavigationEnvironment(initialGrid, start, rules.initialMaxEnergy, rules.turnLimit) {
        rules_ = {rules.initialMaxEnergy, rules.turnLimit, rules.energyCostGeneral,
                  rules.energyCostRoughTerrain, rules.energyCostOthers, rules.reward,
                  rules.energy, rules.energyPenalty, rules.scorePenalty};
    }

    // Métodos para consultar el estado del agente
    Position getAgentPosition() const { return agent_; }
    int getEnergy() const { return energy_; }
    bool isActive() const { return active_; }
    int getScore() const { return score_; }
    std::size_t getCollectedResources() const { return collectedResources_; }


    // Metodos descritos en el informe

    // Vuelve la partida a su estado inicial: tablero original (recursos y baterías disponibles otra vez),
    // agente en el inicio, energía máxima, puntaje, recursos y turno en cero. Las reglas no cambian.
    // La semilla se guarda para que la simulación sepa con qué semilla se reinició.
    void reset(std::uint32_t seed) {
        seed_ = seed;
        initialGrid = originalGrid_;
        agent_ = start;
        energy_ = maxEnergy_;
        score_ = 0;
        collectedResources_ = 0;
        turn_ = 0;
        active_ = true;
        endReason_ = EndReason::none;
        guard_ = guardStart_; // el vigilante vuelve a su casilla inicial
    }

    [[nodiscard]] std::uint32_t seed() const noexcept { return seed_; }

    // Proyecto 2: agrega el vigilante. Debe empezar en una casilla libre del tablero,
    // distinta del inicio del jugador. El controlador decide cómo perseguir; el entorno valida.
    void setGuard(Position start, std::shared_ptr<IGuardController> controller) {
        if (controller == nullptr) {
            throw std::invalid_argument("Error: El vigilante necesita un controlador");
        }
        if (!originalGrid_.contains(start) || !std::holds_alternative<Empty>(originalGrid_.at(start))) {
            throw std::invalid_argument("Error: El vigilante debe empezar en una casilla libre");
        }
        if (start == this->start) {
            throw std::invalid_argument("Error: El vigilante no puede empezar en el inicio del jugador");
        }
        guardController_ = std::move(controller);
        guardStart_ = start;
        guard_ = start;
    }
    // Posición actual del vigilante (nullopt si la partida no tiene vigilante)
    [[nodiscard]] std::optional<Position> guardPosition() const {
        if (!hasGuard()) {
            return std::nullopt;
        }
        return guard_;
    }
    // true si el interruptor indicado existe y ya fue activado
    [[nodiscard]] bool isSwitchActive(SwitchId id) const {
        const auto it = std::find_if(initialGrid.begin(), initialGrid.end(), [id](const Cell& cell) {
            const Switch* candidate = std::get_if<Switch>(&cell);
            return candidate != nullptr && candidate->id == id;
        });
        return it != initialGrid.end() && std::get<Switch>(*it).active;
    }

    // Límite de turnos de la partida (la consola lo muestra en la barra de estado: "Turno 27/180")
    [[nodiscard]] std::size_t maxTurns() const noexcept { return turnLimit; }

    [[nodiscard]] Observation state() const {
        Observation observation{agent_, goalPosition(), energy_, maxEnergy_, score_,
                                collectedResources_, turn_, availableActions()};
        observation.hasSwitches = hasSwitches();
        observation.switchAActive = isSwitchActive(SwitchId::A);
        observation.switchBActive = isSwitchActive(SwitchId::B);
        observation.hasGuard = hasGuard();
        observation.guard = guard_;
        observation.guardMovesNextTurn = hasGuard() && !isFinished() && (turn_ + 1) % 2 == 0;
        // Mapa de solo lectura: se entrega una copia del tablero actual (enunciado, sección 4)
        observation.rows = Rows;
        observation.columns = Columns;
        observation.map.assign(initialGrid.begin(), initialGrid.end());
        return observation;
    }

    [[nodiscard]] std::vector<Action> availableActions() const {
        std::vector<Action> actions;

        // Después del término no hay acciones legales
        if (isFinished()) {
            return actions;
        }

        // Iteramos sobre cada accion existente [se usó IA para saber cómo iterar sobre Enum]
        for (std::size_t i = 0; i <= 4; i++) {
            Action action = static_cast<Action>(i);

            std::optional<Position> optionalPosition{neighbor(agent_, action)};

            // Ignoramos la accion si esta produce una posicion con indices negativos
            if (!optionalPosition.has_value()) {
                continue;
            }

            // Como sabemos que si hay una posicion, se la asignamos a una variable no opcional
            Position position(optionalPosition.value());

            // Ignoramos la accion si esta produce una posicion que no se encuentre dentro del tablero
            if (!initialGrid.contains(position)) {
                continue;
            }

            // Ignoramos la accion si esta produce una posicion no transitable (muro), según CellTraits
            if (!isTraversable(initialGrid.at(position))) {
                continue;
            }

            // Ya hemos verificado que se trate de una accion valida, la aniadimos al vector de acciones validas
            actions.emplace_back(action);
        }

        return actions;
    }

    [[nodiscard]] bool isFinished() const noexcept {
        return endReason_ != EndReason::none;
    }

    [[nodiscard]] StepResult step(Action action) {
        // INVARIANTE: Un agente sin energía no puede ejecutar otra acción
        // (con energía 0 la partida termina por noEnergy y se lanza esta excepción)
        if (!active_) {
            throw std::logic_error("Error: la partida ya termino");
        }

        std::vector<NavigationEvent> events;

        //Incrementamos el turno
        turn_++;

        //Validamos la acción
        if (action == Action::wait) {
            // Esperar no mueve al agente ni activa la celda donde está. Solo gasta energía.
            spendEnergy(rules_.energyCostOthers, events);

        } else {
            std::optional<Position> target = targetOf(action);

            if (target.has_value()) {
                // Movimiento válido: se mueve, paga la entrada y se aplica el efecto de la celda
                const Position from = agent_;
                agent_ = *target;
                const int cost = entryCost(initialGrid.at(agent_));
                appendEvents(events, MovedEvent{from, agent_, cost});
                spendEnergy(cost, events);
                applyCellEffect(events);

            } else {
                // MOVIMIENTO RECHAZADO (chocó contra muro o borde): no se mueve, pero gasta energía
                appendEvents(events, MovementRejectedEvent{agent_, action});
                spendEnergy(rules_.energyCostOthers, events);
            }
        }

        // --- Proyecto 2: resto del turno (3.2) ---
        // 2. Captura inicial: el jugador entró a la casilla del vigilante
        bool captured = guardCaptures();
        // 3. Actúa el vigilante (solo en turnos pares)
        if (!captured) {
            moveGuard(events);
            captured = guardCaptures();
        }
        // 4. Captura o energía agotada: derrota. En otro caso, se activa el interruptor
        //    de destino y se comprueba el escape. La captura tiene prioridad sobre el escape.
        if (captured) {
            capture(events);
        } else {
            activateSwitch(events);
            checkEnd(events);
        }

        return StepResult{state(), std::move(events), isFinished(), endReason_};
    }

    [[nodiscard]] const Grid<Cell, Rows, Columns>& grid() const noexcept {
        return initialGrid;
    }

};

// Crea el entorno con la dificultad elegida al ejecutar (por ejemplo, desde --difficulty en la consola).
// Es el único lugar donde se decide según el nivel; el entorno solo recibe las reglas.
template <std::size_t Rows, std::size_t Columns>
NavigationEnvironment<Rows, Columns> makeEnvironment(Grid<Cell, Rows, Columns> grid, Position start,
                                                     Difficulty difficulty) {
    switch (difficulty) {
    case Difficulty::Easy: return NavigationEnvironment<Rows, Columns>{grid, start, GameRules<Difficulty::Easy>{}};
    case Difficulty::Hard: return NavigationEnvironment<Rows, Columns>{grid, start, GameRules<Difficulty::Hard>{}};
    default:               return NavigationEnvironment<Rows, Columns>{grid, start, GameRules<Difficulty::Standard>{}};
    }
}
