# Diseño — Circuito de Escape

Este documento explica cómo está organizado el proyecto y **dónde se aplica cada tema obligatorio**
del enunciado. Las rutas son relativas a la raíz del repositorio.

## 1. Arquitectura por capas

```text
ConsoleUI  ──UiCommand──►  main (runInteractive / runHeadless)  ◄──Action──  IController
dibuja con FTXUI            coordina el ciclo                               decide (política)
                                     │
                       step(Action) / StepResult
                                     ▼
                     NavigationEnvironment<Rows, Columns>
                       estado y reglas del juego
                                     │ posee y actualiza
                                     ▼
                         Grid<Cell, Rows, Columns>
```

| Pieza | Archivo | Responsabilidad | No hace |
|---|---|---|---|
| `Grid<CellType, Rows, Columns>` | `include/circuit_escape/grid.hpp` | almacenar celdas, validar límites, iteradores | no conoce turnos, energía ni reglas |
| `NavigationEnvironment<Rows, Columns>` | `include/circuit_escape/environment.hpp` | estado de la partida, validar y ejecutar acciones, producir eventos | no lee teclado, no imprime, no incluye FTXUI |
| `GameRules<Difficulty>` | `include/circuit_escape/game_rules.hpp` | costos y recompensas como **datos** | no contiene lógica |
| Eventos | `include/circuit_escape/events.hpp` | describir lo que ya ocurrió en un paso | no modifican el estado |
| `IController`, políticas | `include/circuit_escape/controllers.hpp`, `src/controllers.cpp` | elegir una acción a partir de una `Observation` | no modifican el entorno ni hacen E/S |
| Escenarios | `include/circuit_escape/scenario.hpp`, `assets/maps/` | cargar mapas de texto | — |
| Simulación | `include/circuit_escape/simulation.hpp` | jugar una partida completa con una política y una semilla | no usa consola |
| `ConsoleUI` | `include/circuit_escape/console_ui.hpp`, `src/console_ui.cpp` | dibujar con FTXUI y traducir teclas a `UiCommand` | no aplica energía ni mueve celdas |
| `main` | `app/main.cpp` | opciones, bucle de FTXUI, modo automático y `--headless` | no duplica reglas |

`NavigationEnvironment` es la **única** pieza que modifica el estado del juego. La interfaz y los
controladores reciben copias (`Observation`) o referencias `const` (`grid()`) y solo proponen una `Action`.

En CMake esto se refleja en dos bibliotecas: `circuit_escape_core` (motor, sin FTXUI) y
`circuit_escape_ui` (la única enlazada a FTXUI). Todas las pruebas del motor se enlazan solo al core.

## 2. Resolución de un paso (`step`)

Orden obligatorio del enunciado (5.5):

1. Si la partida terminó, `step` lanza `std::logic_error`.
2. Se incrementa el turno.
3. `wait`: se cobra `energyCostOthers` y **no** se activa la celda actual.
4. Movimiento inválido (muro o fuera del tablero): no cambia la posición, se genera
   `MovementRejectedEvent` y se cobra `energyCostOthers`.
5. Movimiento válido: se actualiza la posición, se genera `MovedEvent` y se cobra el costo de entrada
   (`energyCostRoughTerrain` para terreno elevado, `energyCostGeneral` para el resto).
6. Se aplica el efecto de la celda destino **aunque** el costo de entrada haya dejado la energía en 0
   (una batería todavía puede recargar al agente).
7. Se comprueban las condiciones de término con la precedencia `goalReached` > `noEnergy` > `turnLimit`.

Toda modificación de energía pasa por `changeEnergy`, que usa `std::clamp(…, 0, maxEnergy)` y solo
genera `EnergyChangedEvent` si el valor cambió. Los eventos se agregan en el orden en que ocurren.

**Consumibles.** Recurso y batería no se reemplazan por `Empty`: guardan un indicador
(`ResourceCell::collected`, `Battery::consumed`), así `reset` puede restaurar el mapa original desde una
copia. La trampa se aplica en cada entrada. El puntaje puede ser negativo.

**Precondiciones del constructor** (`validate()` en `environment.hpp`, la llaman todos los constructores):
energía inicial y límite de turnos positivos, inicio dentro del tablero y en una celda transitable, y
exactamente una salida. Si alguna falla se lanza `std::invalid_argument`.

**Reglas por datos.** Los valores de cada perfil están en `GameRules<Difficulty>`; `makeEnvironment`
elige el perfil según la dificultad pedida en ejecución y el entorno solo aplica las reglas que recibe.
Los valores numéricos que las celdas traen en sus campos (por ejemplo `Battery::energy`) no se usan:
manda siempre `GameRules`.

## 3. Dónde se aplica cada tema obligatorio

### Tipos abstractos de datos e invariantes
- `Grid` oculta su `std::array` y solo permite acceso validado (`at` lanza `std::out_of_range`).
- `NavigationEnvironment` mantiene las invariantes del agente (5.3): posición dentro del tablero y nunca
  en un muro (`validate`, `targetOf`), energía en `[0, max]` (`changeEnergy`), sin acciones después del
  término (`step` lanza `logic_error`), recurso recogido una sola vez (`collected`).

### Copia, movimiento y RAII
- `Grid` y `NavigationEnvironment` tienen semántica de valor (regla del cero): se copian para guardar el
  tablero original (`originalGrid_`) y restaurarlo en `reset`.
- `std::unique_ptr<IController>` administra el controlador automático; no hay `new` ni `delete`.
- `StepResult` se devuelve por valor y los eventos se mueven (`std::move(events)`).
- `loadScenario` usa `std::ifstream`, que cierra el archivo al salir de la función.

### Sobrecarga de funciones y operadores
- `operator==` por defecto en `Position` y en `SimulationResult`.
- `toString(Position)` y `toString(Action)`.
- `Grid::at` y `begin`/`end` en versiones `const` y no `const`.

### Herencia y polimorfismo dinámico
- `IController` (interfaz virtual pura) implementada por `PolicyController<RandomPolicy>` y
  `PolicyController<HeuristicPolicy>` (`controllers.hpp`).
- El despacho dinámico ocurre en `automatic->selectAction(...)` dentro de `runInteractive` y en
  `controller.selectAction(...)` dentro de `runHeadless` (`app/main.cpp`). El código que juega no sabe qué
  controlador usa: no hay `typeid`, `dynamic_cast` ni condicionales según el tipo.
- Jugador humano: como permite el enunciado (5.7), la consola traduce la tecla a una `Action` y la
  entrega directamente a `step`, sin leer `std::cin` desde un controlador.

### Templates de funciones (6.1)
| Template | Archivo | Rango por iteradores | Usado con |
|---|---|---|---|
| `bestBy(first, last, cost)` | `controllers.hpp` | sí | `vector<Action>` (heurística) y `list<Position>` (pruebas) |
| `countEvents<Events...>(first, last)` | `events.hpp` | sí | `vector`, `list` y `deque` de eventos (pruebas) |
| `appendEvents(destination, values...)` | `overloaded.hpp` | — | `vector<NavigationEvent>` (entorno), `vector<int>` (pruebas) |
| `holdsAnyOf<Events...>(event)` | `events.hpp` | — | `countEvents` y pruebas |
| `runSimulation`, `makeEnvironment`, `scenarioFromLines`, `loadScenario` | varios | — | tableros de 3 × 4 y 20 × 30 |

Ningún algoritmo está duplicado por contenedor.

### Templates de clases y parámetros no-tipo (6.2)
- `Grid<CellType, Rows, Columns>`: un parámetro de tipo y dos no-tipo, almacenamiento contiguo, acceso
  con `at` (equivalente a `operator()`), iteradores `const` y no `const`, y
  `static_assert(Rows > 0 && Columns > 0)` que impide tableros vacíos.
- También: `NavigationEnvironment<Rows, Columns>`, `Scenario<Rows, Columns>`, `ResourceCell<Reward>`,
  `PolicyController<Policy>` y `GameRules<Difficulty>` (parámetro no-tipo enumerado).

### Especialización total y parcial (6.3)
- **Total:** `CellTraits<Wall>` (no transitable) y `GameRules<Difficulty::Easy>` / `GameRules<Difficulty::Hard>`.
- **Parcial:** `CellTraits<ResourceCell<Reward>>` reconoce toda la familia de recursos, sea cual sea el
  tipo de la recompensa.
- **Uso en el programa:** `NavigationEnvironment::isTraversable` hace `std::visit` sobre la celda y consulta
  `CellTraits<T>::traversable`; lo usan `availableActions`, `targetOf` y `validate`.
  `makeEnvironment` elige la especialización de `GameRules` según la dificultad.

### Templates variádicos y fold expressions (6.4)
- `Overloaded<Callables...>` (paquete variádico + guía de deducción) para `std::visit` sobre celdas,
  eventos y comandos.
- `appendEvents`: fold con el operador coma; agrega los eventos en orden.
- `holdsAnyOf<Events...>`: fold con `||`; `countEvents<Events...>` lo reutiliza.
- `describe(NavigationEvent)` convierte cada evento en texto para el pie de la consola; si se agrega un tipo
  al `variant` y no se maneja, no compila.

### Concepts y polimorfismo dinámico combinados (6.5)
- **Qué verifica el concept:** `NavigationPolicy<P>` exige que `p.selectAction(observation, span<const Action>)`
  sea válido y devuelva exactamente `Action`.
- **Clase genérica:** `PolicyController<NavigationPolicy Policy>`, que adapta cualquier política que cumpla el
  concept a la interfaz virtual `IController`. Las políticas no heredan de nada.
- **Dónde ocurre el despacho dinámico:** en la llamada virtual `IController::selectAction`.
- **Por qué no hay condicionales por tipo:** el controlador se elige una sola vez al iniciar (`makeController`
  en `app/main.cpp`, según la opción `--controller`) y después solo se usa la interfaz.
- Concepts de la biblioteca: `std::forward_iterator` (`bestBy`), `std::input_iterator` y `std::sentinel_for`
  (`countEvents`).
- Prueba negativa: `tests/negative/bad_policy.cpp` no compila porque su política devuelve `int`.

### Biblioteca estándar (6.6)
`std::array` (tablero), `std::vector` (acciones, eventos), `std::list` y `std::deque` (pruebas de
algoritmos genéricos), `std::variant` + `std::visit` (celdas, eventos, comandos), `std::optional`
(vecinos, destino de un movimiento, comando traducido), `std::span` (acciones legales sin copiar),
`std::unique_ptr` (controlador), `<algorithm>` (`find_if`, `count_if`, `clamp`, `equal`, `fill`, `all_of`)
y `<random>` (`mt19937` con semilla). La justificación de los contenedores está en el README.

### Separación entre estado, reglas, controlador e interfaz
- Estado y reglas: `NavigationEnvironment` + `GameRules`.
- Controlador: `IController` y políticas (solo ven `Observation`).
- Interfaz: `ConsoleUI` y `app/main.cpp` (FTXUI solo en `circuit_escape_ui`). Las teclas llegan como
  `ftxui::Event` a `CatchEvent`, sin prompt ni Enter.

### Pruebas y simulaciones reproducibles
- Todas las pruebas usan `assert` y están registradas en CTest (ver README).
- `RandomPolicy` recibe la semilla en su constructor y `reset(seed)` reinicia el entorno;
  `simulation_test` comprueba que la misma semilla produce la misma partida.

## 4. Interfaz de consola

- `ConsoleUI::render` arma con elementos FTXUI independientes de 2 columnas (`size(WIDTH, EQUAL, 2)`) la
  barra de estado, la regla horizontal (keycaps `0️⃣`…`9️⃣`), las 20 filas y el pie. Nunca se mide el ancho
  de un emoji con `std::string::size()`.
- `--ascii` cambia los símbolos a `@ . # ~ R B T S` y los dígitos a normales.
- En `app/main.cpp`, una tecla que `translate` no reconoce muestra un mensaje y **no** llama a `step`;
  `H` alterna la ayuda y `Q` sale. En modo automático, Espacio o Enter piden la acción al controlador.
- En Windows, `main` llama a `SetConsoleOutputCP(CP_UTF8)` para que PowerShell y `cmd.exe` muestren UTF-8.

## 5. Preparación para el segundo proyecto

El entorno ya expone la API pedida en la sección 7:
```cpp
environment.reset(seed);
auto state = environment.state();              // Observation
auto actions = environment.availableActions();
auto result = environment.step(action);       // StepResult {observation, events, finished, reason}
bool finished = environment.isFinished();
```
`StepResult` trae los eventos y el motivo de término, suficientes para calcular después una recompensa:
penalización por turno, por movimiento inválido (`MovementRejectedEvent`), recompensa por recurso
(`ResourceCollectedEvent`), por llegar a la salida (`reason == goalReached`) y penalización por agotar la
energía (`reason == noEnergy`). Un agente de Q-learning sería otra política que cumpla `NavigationPolicy`,
envuelta en `PolicyController`, sin tocar el entorno.

## 6. Decisiones y limitaciones conocidas

- Los valores de los perfiles son los sugeridos por el enunciado, sin ajustes.
- El escenario 1 reproduce el ejemplo del enunciado y por eso **no** se puede completar en `hard`
  (hacen falta 44 movimientos y hay 42 de energía como máximo). El escenario 2 se completa en los tres perfiles.
- La heurística (distancia Manhattan, sin retroceder salvo en callejones) completa el escenario 2 pero se
  queda sin energía en el escenario 1; el enunciado no exige que encuentre siempre el camino.
- No se implementó el historial opcional `FixedHistory`.
- `NavigationEnvironment` conserva un constructor de cinco parámetros (inicio para `reset` y posición
  actual por separado) que usan algunas pruebas de movimiento.
