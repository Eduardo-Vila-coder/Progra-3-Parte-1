# Diseño — Circuito de Escape

Este documento explica cómo está organizado el proyecto, por qué, y **dónde se aplica cada tema
obligatorio** del enunciado. Las rutas son relativas a la raíz del repositorio.

## 1. Arquitectura por capas

```text
ConsoleUI (app/console_ui)  ──UiCommand──►  GameSession / main  ◄──Action──  IController
   dibuja con FTXUI                          coordina el ciclo              decide (humano / política)
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
| `ConsoleUI`, `GameSession` | `app/console_ui.hpp/.cpp` | dibujar y traducir teclas; coordinar el ciclo | no aplican energía ni mueven celdas |
| `main` | `app/main.cpp` | opciones de línea de comandos y bucle de FTXUI | no duplica reglas |

`NavigationEnvironment` es la **única** pieza que modifica el estado del juego. La UI y los controladores
reciben copias (`Observation`) o referencias `const` (`grid()`), y solo proponen una `Action`.

En CMake esto se refleja en dos bibliotecas: `circuit_escape_core` (motor, sin FTXUI) y
`circuit_escape_ui` (la única enlazada a FTXUI). Todas las pruebas del motor se enlazan solo al core.

## 2. Resolución de un paso (`step`)

El orden es el obligatorio del enunciado (5.5):

1. Si la partida terminó, `step` lanza `std::logic_error`.
2. Se incrementa el turno.
3. `wait`: se cobra `energyCostOthers` y **no** se activa la celda actual.
4. Movimiento inválido (muro o fuera del tablero): no cambia la posición, se genera
   `MovementRejectedEvent` y se cobra `energyCostOthers`.
5. Movimiento válido: se actualiza la posición, se genera `MovedEvent` y se cobra el costo de entrada
   (`energyCostRoughTerrain` para terreno elevado, `energyCostGeneral` para el resto).
6. Se aplica el efecto de la celda destino **aunque** el costo de entrada haya dejado la energía en 0
   (así una batería todavía puede salvar al agente).
7. Se comprueban las condiciones de término con la precedencia `goalReached` > `noEnergy` > `turnLimit`.

Toda modificación de energía pasa por `changeEnergy`, que usa `std::clamp(…, 0, maxEnergy)` y solo
genera `EnergyChangedEvent` si el valor cambió. Los eventos se agregan en el mismo orden en que ocurren.

**Consumibles.** Recurso y batería no se reemplazan por `Empty`: guardan un indicador
(`ResourceCell::collected`, `Battery::consumed`). Así el tablero conserva la historia y `reset` puede
restaurar el mapa original. La trampa no es consumible y se aplica en cada entrada. El puntaje puede ser
negativo.

**Precondiciones del constructor** (`validate()` en `environment.hpp`): energía inicial y límite de turnos
positivos, inicio dentro del tablero y en una celda transitable, exactamente una salida. Si alguna falla
se lanza `std::invalid_argument`.

## 3. Dónde se aplica cada tema obligatorio

### Tipos abstractos de datos e invariantes
- `Grid` oculta su `std::array` y solo permite acceso validado (`at` lanza `std::out_of_range`).
- `NavigationEnvironment` mantiene las invariantes del agente (5.3): posición dentro del tablero, nunca
  en un muro (`targetOf` y `validate`), energía en `[0, max]` (`changeEnergy`), sin acciones después
  del término (`step` lanza `logic_error`), recurso recogido una sola vez (`collected`).

### Copia, movimiento y RAII
- `Grid` y `NavigationEnvironment` tienen semántica de valor (regla del cero): se copian para guardar el
  tablero original (`originalGrid_`) y restaurarlo en `reset`.
- `std::unique_ptr<IController>` administra los controladores polimórficos; no hay `new` ni `delete`.
- `StepResult` se devuelve por valor y los eventos se mueven (`std::move(events)`).
- `loadScenario` usa `std::ifstream`, que cierra el archivo al salir de la función.

### Sobrecarga de funciones y operadores
- `operator==` por defecto en `Position` (y en `SimulationResult`).
- `toString(Position)` y `toString(Action)`.
- `Grid::at` y `begin`/`end` en versiones `const` y no `const`.

### Herencia y polimorfismo dinámico
- `IController` (interfaz virtual pura) con tres implementaciones: `PolicyController<RandomPolicy>`,
  `PolicyController<HeuristicPolicy>` y `HumanController` (`controllers.hpp`).
- El despacho dinámico ocurre en `controller.selectAction(...)` dentro de `GameSession::advance`
  (`app/console_ui.cpp`) y de `runHeadless` (`app/main.cpp`). El juego no sabe qué controlador usa:
  no hay `typeid`, `dynamic_cast` ni condicionales según el tipo.

### Templates de funciones (6.1)
| Template | Archivo | Rango por iteradores | Usado con |
|---|---|---|---|
| `bestBy(first, last, cost)` | `controllers.hpp` | sí | `vector<Action>` (heurística), `list<Position>` (pruebas) |
| `countEvents<Events...>(first, last)` | `events.hpp` | sí | `vector`, `list` y `deque` de eventos |
| `appendEvents(destination, values...)` | `overloaded.hpp` | — | `vector<NavigationEvent>`, `vector<int>` |
| `holdsAnyOf<Events...>(event)` | `events.hpp` | — | pie de la UI, pruebas |
| `runSimulation`, `makeEnvironment`, `loadScenario` | varios | — | tableros de 3 × 4 y 20 × 30 |

Ningún algoritmo está duplicado por contenedor.

### Templates de clases y parámetros no-tipo (6.2)
- `Grid<CellType, Rows, Columns>`: un parámetro de tipo, dos no-tipo, almacenamiento contiguo,
  acceso con `at`, iteradores `const` y no `const`, y `static_assert(Rows > 0 && Columns > 0)`.
- `NavigationEnvironment<Rows, Columns>`, `Scenario<Rows, Columns>`, `ResourceCell<Reward>`,
  `PolicyController<Policy>` y `GameRules<Difficulty>` (parámetro no-tipo enumerado).

### Especialización total y parcial (6.3)
- **Total:** `CellTraits<Wall>` (no transitable), y `GameRules<Difficulty::Easy>` /
  `GameRules<Difficulty::Hard>` (valores de cada perfil).
- **Parcial:** `CellTraits<ResourceCell<Reward>>` reconoce toda la familia de recursos, sin importar el
  tipo de la recompensa.
- **Uso en el programa:** `NavigationEnvironment::isTraversable` hace `std::visit` sobre la celda y
  consulta `CellTraits<T>::traversable`; lo usan `availableActions`, `targetOf` y `validate`.
  `makeEnvironment` elige la especialización de `GameRules` según la dificultad pedida en ejecución.

### Templates variádicos y fold expressions (6.4)
- `Overloaded<Callables...>` (paquete variádico + guía de deducción) para `std::visit` sobre celdas,
  eventos y comandos.
- `appendEvents`: fold con el operador coma, agrega eventos en orden.
- `holdsAnyOf<Events...>`: fold con `||`; `countEvents<Events...>` lo reutiliza.
- `describe(NavigationEvent)` convierte cada evento en texto; si se agrega un tipo al `variant` y no se
  maneja, no compila.

### Concepts y polimorfismo dinámico combinados (6.5)
- **Condición del concept:** `NavigationPolicy<P>` exige que `p.selectAction(observation, span<const Action>)`
  sea válido y devuelva exactamente `Action`.
- **Clase genérica:** `PolicyController<NavigationPolicy Policy>`, que adapta cualquier política que cumpla
  el concept a la interfaz virtual `IController`. Las políticas no heredan de nada.
- **Despacho dinámico:** en la llamada virtual `IController::selectAction`.
- **Sin condicionales por tipo:** el controlador se elige una sola vez (`makeController`) y después solo se
  usa la interfaz.
- También se usan los concepts de la biblioteca: `std::forward_iterator` (`bestBy`),
  `std::input_iterator` / `std::sentinel_for` (`countEvents`).
- Prueba negativa: `tests/negative/bad_policy.cpp` no compila porque su política devuelve `int`.

### Biblioteca estándar (6.6)
`std::array` (tablero), `std::vector` (acciones, eventos), `std::list` y `std::deque` (pruebas de
algoritmos genéricos), `std::variant` + `std::visit` (celdas, eventos, comandos), `std::optional`
(vecinos, destino de un movimiento, decisión pendiente del humano), `std::span` (acciones legales sin
copiar), `std::unique_ptr` (controladores), `<algorithm>` (`find_if`, `count_if`, `clamp`, `equal`,
`fill`, `all_of`), `<random>` (`mt19937` con semilla). La justificación de los contenedores está en el README.

### Separación entre estado, reglas, controlador e interfaz
- Estado y reglas: `NavigationEnvironment` + `GameRules`.
- Controlador: `IController` y políticas (no ven el estado interno, solo `Observation`).
- Interfaz: `ConsoleUI` (FTXUI solo en `app/`). La lectura de teclado no bloquea: FTXUI entrega cada
  pulsación como un `ftxui::Event` a `CatchEvent`.

### Pruebas y simulaciones reproducibles
- Todas las pruebas usan `assert` y se registran en CTest (ver README).
- `RandomPolicy` recibe la semilla en su constructor; `reset(seed)` reinicia el entorno.
  `simulation_test` y `polymorphism_test` comprueban que la misma semilla produce la misma partida.

## 4. Interfaz de consola

- `ConsoleUI::render` arma, con elementos FTXUI independientes de 2 columnas (`size(WIDTH, EQUAL, 2)`),
  la barra de estado, la regla horizontal (keycaps `0️⃣`…`9️⃣` repetidos), las 20 filas y el pie.
  Nunca se mide el ancho de un emoji con `std::string::size()`.
- `--ascii` cambia los símbolos a `@ . # ~ R B T S` y los dígitos a normales.
- `GameSession` recibe cada tecla: si `translate` no la reconoce muestra un mensaje y **no** llama a
  `step`; `H` alterna la ayuda; `Q` sale. En modo humano la acción se entrega a `HumanController`;
  en modo automático, Espacio/Enter/N le piden la acción al controlador.

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
penalización por turno (cada paso), por movimiento inválido (`MovementRejectedEvent`), recompensa por
recurso (`ResourceCollectedEvent`), por llegar a la salida (`reason == goalReached`) y penalización por
agotar la energía (`reason == noEnergy`). Un agente de Q-learning sería otra política que cumpla
`NavigationPolicy`, envuelta en `PolicyController`, sin tocar el entorno.

## 6. Decisiones y limitaciones conocidas

- Los valores de los perfiles son los sugeridos por el enunciado, sin ajustes.
- El escenario 1 reproduce exactamente el ejemplo del enunciado y por eso **no** se puede completar en
  `hard` (hacen falta 44 movimientos y hay 42 de energía como máximo). El escenario 2 sí se completa en
  los tres perfiles.
- La heurística (distancia Manhattan, sin retroceder salvo en callejones) completa el escenario 2 pero
  se queda sin energía en el escenario 1; el enunciado no exige que encuentre siempre el camino.
- No se implementó el historial opcional `FixedHistory`.
- `NavigationEnvironment` conserva un constructor de cinco parámetros (inicio para `reset` y posición
  actual por separado) que usan algunas pruebas de movimiento.
