# Circuito de Escape

**CS2013 — Programación III · UTEC · 2026-2 · Proyecto 1**

Motor de un juego de navegación por turnos en C++20. Un agente recorre una cuadrícula de 20 × 30,
evita obstáculos, administra su energía, recoge recursos y debe llegar a la salida antes de quedarse
sin energía o superar el límite de turnos. El motor está separado de la interfaz: se puede jugar en
consola (FTXUI) o dejar que juegue un controlador automático, y todas las reglas se prueban sin consola.

## Integrantes

| Nombre | Código | Usuario de GitHub |
|---|---|---|
| Eduardo Raúl Vila Castellares | _(completar)_ | [Eduardo-Vila-coder](https://github.com/Eduardo-Vila-coder) |
| Yerik Vega | _(completar)_ | [Yerik-Vega](https://github.com/Yerik-Vega) |
| Cristhian Gabriel Jinchuña Cama | _(completar)_ | [cristhianjinchuna-dev](https://github.com/cristhianjinchuna-dev) |
| Mathias Cavalcanti | _(completar)_ | [MatCavUTEC](https://github.com/MatCavUTEC) |
| _(completar)_ | _(completar)_ | [hfuv](https://github.com/hfuv) (MQS144) |

El detalle de lo que hizo cada integrante está en [`docs/contributions.md`](docs/contributions.md)
y las decisiones de diseño en [`docs/design.md`](docs/design.md).

## Requisitos

- **CMake 3.20** o superior.
- Un compilador con **C++20**: GCC 11+, Clang 16+, MSVC 2022 (17.x) o MinGW-w64 con GCC 11+.
- **Git** (CMake descarga FTXUI v7.0.3 automáticamente con `FetchContent` la primera vez).
- Una terminal con UTF-8 para ver los emojis (Windows Terminal, PowerShell/CMD en Windows 10+,
  cualquier terminal moderna en Linux y macOS). Si los emojis se ven mal, usa `--ascii`.

FTXUI es la única dependencia externa y solo la usa la capa de presentación (`app/`).

## Compilación

Desde una carpeta limpia, en la raíz del repositorio:

**Linux / macOS**
```bash
cmake -S . -B build
cmake --build build -j
```

**Windows (PowerShell o Command Prompt)**
```powershell
cmake -S . -B build
cmake --build build --config Debug
```

Con Visual Studio los ejecutables quedan en `build\Debug\`; con MinGW o Ninja quedan en `build\`.
En CLion basta con abrir la carpeta y recargar el proyecto CMake.

## Pruebas

```bash
ctest --test-dir build --output-on-failure            # Linux, macOS, MinGW
ctest --test-dir build -C Debug --output-on-failure   # Windows con Visual Studio
```

| Prueba | Qué cubre |
|---|---|
| `grid_test` | acceso válido/inválido, bordes y esquinas, versiones const, iteradores y orden por filas |
| `environment_test` | acciones disponibles, movimientos, perfiles, término y precedencia, precondiciones, `CellTraits` |
| `interactions_test` | costos, recurso, batería, trampa, límites de energía |
| `events_test` | los 6 tipos de evento, `std::visit`, fold expressions, `countEvents` con varios contenedores |
| `controllers_test` | `bestBy`, políticas aleatoria y heurística, concept `NavigationPolicy` |
| `polymorphism_test` | `IController`, `PolicyController<Policy>`, `HumanController`, `makeController` |
| `scenarios_test` | carga de mapas, los dos escenarios 20 × 30 y sus rutas comprobadas |
| `simulation_test` | `reset(seed)` y simulación reproducible con semilla fija |
| `ui_test` | traducción de teclas, comando desconocido, emoji/ASCII de cada celda, render de 20 × 30 |

Ninguna prueba del motor lee `std::cin` ni escribe en `std::cout` para decidir el resultado.

**Prueba negativa de compilación (concept).** `tests/negative/bad_policy.cpp` **no debe compilar** y
por eso no está en CMake. Muestra el diagnóstico cuando una política no cumple `NavigationPolicy`:
```bash
g++ -std=c++20 -Iinclude -fsyntax-only tests/negative/bad_policy.cpp
```
El compilador rechaza `runSimulation` porque `BadPolicy::selectAction` devuelve `int` en lugar de `Action`.

## Ejecución

```text
navigation_game [--ascii] [--difficulty easy|standard|hard] [--scenario 1|2]
                [--controller human|random|heuristic] [--seed N] [--headless]
```

| Opción | Valor por defecto | Descripción |
|---|---|---|
| `--ascii` | emoji | Dibuja con caracteres ASCII en lugar de emojis |
| `--difficulty` | `standard` | Perfil de reglas (ver tabla de perfiles) |
| `--scenario` | `1` | Mapa 20 × 30 a cargar desde `assets/maps/` |
| `--controller` | `human` | Quién decide: el jugador, la política aleatoria o la heurística |
| `--seed` | `42` | Semilla del controlador aleatorio (misma semilla, misma partida) |
| `--headless` | — | Juega la partida completa con el controlador automático, sin interfaz, e imprime el resultado |

### Controles

| Tecla | Modo humano | Modo automático |
|---|---|---|
| `W` `A` `S` `D` o flechas | mover arriba / izquierda / abajo / derecha | — |
| `E` | esperar un turno | — |
| `Espacio`, `Enter` o `N` | — | el controlador decide el siguiente turno |
| `H` | mostrar u ocultar la ayuda | igual |
| `Q` | salir | igual |

Las letras no distinguen mayúsculas. Cada tecla actúa al instante, sin presionar Enter. Una tecla que no es
un comando muestra un mensaje y no consume turno.

### Ejemplos

```bash
./build/navigation_game                                          # jugar en emoji, standard, escenario 1
./build/navigation_game --ascii --difficulty hard --scenario 2   # jugar en ASCII
./build/navigation_game --controller heuristic --scenario 2      # ver jugar a la heurística (Espacio avanza)
./build/navigation_game --controller random --seed 7 --headless  # simulación automática reproducible
```
En Windows reemplaza `./build/navigation_game` por `.\build\navigation_game.exe`
(o `.\build\Debug\navigation_game.exe` con Visual Studio).

Salida de ejemplo en modo `--headless`:
```text
$ navigation_game --controller heuristic --scenario 2 --headless
¡Completado! Llegaste a la salida. Turnos 31 | Energía 32 | Recursos 0 | Puntaje 0
```

### Convención visual

| Emoji | ASCII | Significado |
|---|---|---|
| 🤖 | `@` | agente |
| ⬜ | `.` | espacio libre (también recurso recogido y batería consumida) |
| ⬛ | `#` | muro |
| 🟫 | `~` | terreno de costo elevado |
| 💎 | `R` | recurso |
| ⚡ | `B` | batería |
| 💥 | `T` | trampa |
| 🏁 | `S` | salida |

La pantalla muestra una barra de estado (turno, energía, puntaje, recursos), la regla de coordenadas,
las 20 filas del tablero y un pie con el último evento y la ayuda breve. Cada celda y cada coordenada
ocupa exactamente 2 columnas, así que cada fila mide 62 columnas.

## Perfiles de dificultad

Los valores están en `include/circuit_escape/game_rules.hpp` (`GameRules<Difficulty>`) y son los sugeridos
por el enunciado, sin cambios. La demostración evaluada usa `standard`.

| Parámetro | Easy | Standard | Hard |
|---|---|---|---|
| Energía inicial y máxima | 80 | 60 | 40 |
| Límite de turnos | 240 | 180 | 140 |
| Costo de espacio libre, recurso, batería, trampa o salida | 1 | 1 | 1 |
| Costo de terreno elevado | 2 | 2 | 3 |
| Costo de `wait` o intento inválido | 1 | 1 | 1 |
| Puntos por recurso | +15 | +10 | +8 |
| Recarga de batería | +5 | +3 | +2 |
| Penalización de energía por trampa | −1 | −2 | −3 |
| Penalización de puntaje por trampa | 0 | −1 | −2 |

## Escenarios

Los mapas son archivos de texto de 20 líneas × 30 caracteres en `assets/maps/`, con los símbolos ASCII
de la tabla anterior (`@` marca el inicio del agente).

| Escenario | Descripción | Solución comprobada |
|---|---|---|
| `scenario_01.txt` | Reproducción del ejemplo del enunciado (página 11) | easy y standard, por dos rutas distintas. **No** es posible en hard: la ruta más corta necesita 44 movimientos y solo hay 40 + 2 de energía |
| `scenario_02.txt` | Diseño propio con un paso norte (batería) y un paso sur (terreno elevado) | easy, standard y hard |

Las rutas están verificadas en `tests/scenarios_test.cpp`.

## Contenedores principales

| Contenedor | Dónde | Por qué |
|---|---|---|
| `std::array<Cell, Rows * Columns>` | `Grid` | tamaño fijo conocido en compilación, memoria contigua, sin asignación dinámica |
| `std::variant` | `Cell`, `NavigationEvent`, `UiCommand` | conjunto cerrado de alternativas, sin herencia; `std::visit` obliga a manejar todos los casos |
| `std::vector<Action>` / `std::vector<NavigationEvent>` | acciones legales, eventos de un paso | tamaño variable pequeño, recorrido en orden, se entrega por `std::span` sin copiar |
| `std::optional<Position>` | `neighbor`, `targetOf` | un movimiento puede no tener destino válido |
| `std::unique_ptr<IController>` | controladores | dueño único de un objeto polimórfico; el tipo concreto se elige en ejecución |
| `std::mt19937` | `RandomPolicy` | generador con semilla controlable: simulaciones reproducibles |

No se usa `std::map` ni `std::unordered_map`: las celdas se indexan por posición directamente en el
`std::array` (`fila * columnas + columna`), así que un mapa no aporta nada.

## Estructura del repositorio

```text
include/circuit_escape/   motor: grid, celdas, posición, reglas, eventos, entorno, controladores, escenarios
src/                      implementación no genérica de controladores
app/                      interfaz FTXUI (console_ui) y main
tests/                    pruebas con assert, registradas en CTest (+ tests/negative/)
assets/maps/              escenarios 20 × 30
docs/                     design.md y contributions.md
```

## Entrega

La entrega es el commit marcado con el tag anotado `proyecto-1-entrega` en la rama `main`:
```bash
git tag -a proyecto-1-entrega -m "Entrega del proyecto 1"
git push origin proyecto-1-entrega
```
Antes de crear el tag: clonar el repositorio en una carpeta nueva, seguir solo este README, compilar,
correr las pruebas y probar la demostración.
