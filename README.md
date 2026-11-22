# Circuito de Escape

**CS2013 — Programación III · UTEC · 2026-2 · Proyecto 2**

Motor de un juego de navegación por turnos en C++20. Un agente recorre una cuadrícula de 20 × 30,
evita obstáculos, administra su energía, recoge recursos y debe llegar a la salida antes de quedarse
sin energía o superar el límite de turnos. El motor está separado de la interfaz: se puede jugar en
consola (FTXUI) o dejar que juegue un controlador automático, y todas las reglas se prueban sin consola.

**Proyecto 2** amplía el juego: el jugador debe activar los **interruptores A y B** (en cualquier orden)
antes de escapar, un **vigilante** lo persigue con **BFS**, los mapas se generan con **Kruskal** y se
validan con **Dijkstra**, y un **instalador** distribuye el juego y el generador de mapas.
Por indicación del profesor, el tablero mantiene un tamaño fijo y no se exige reproducir una
secuencia ganadora guardada.

## Integrantes

| Nombre | Código        | Usuario de GitHub |
|---|---------------|---|
| Eduardo Raúl Vila Castellares | 202520004     | [Eduardo-Vila-coder](https://github.com/Eduardo-Vila-coder) |
| Yerik Dylan Vega Santillan | 202520144 | [Yerik-Vega](https://github.com/Yerik-Vega) |
| Cristhian Gabriel Jinchuña Cama | 202520002 | [cristhianjinchuna-dev](https://github.com/cristhianjinchuna-dev) |
| Mathias Alonso Cavalcanti Estacio | 202210384 | [MatCavUTEC](https://github.com/MatCavUTEC) |
| Mathius Edgar Quispe Sicha | 202520058 | [hfuv](https://github.com/hfuv) (MQS144) |

Qué hizo cada integrante: [`docs/contributions.md`](docs/contributions.md).
Decisiones de diseño y dónde se aplica cada tema: [`docs/design.md`](docs/design.md).

## Requisitos

- **CMake 3.20** o superior.
- Un compilador con **C++20**: GCC 11+, Clang 16+, MSVC 2022 o MinGW-w64 con GCC 11+
  (el MinGW que trae CLion sirve).
- **Git**: la primera vez CMake descarga FTXUI v7.0.3 con `FetchContent`.
  Si la descarga falla o es muy lenta, se puede copiar FTXUI v7.0.3 en `external/ftxui/`
  (carpeta ignorada por git) y CMake la usa en lugar de descargarla.
- Una terminal con UTF-8 para ver los emojis: Windows Terminal, PowerShell o Command Prompt en
  Windows 10+, o cualquier terminal moderna en Linux y macOS. Si los emojis se ven mal, usa `--ascii`.

FTXUI es la única dependencia externa y solo la usa la capa de presentación.

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

Con Visual Studio los ejecutables quedan en `build\Debug\`; con MinGW o Ninja, en `build\`.
En CLion basta con abrir la carpeta (compila en `cmake-build-debug\`).

## Reglas del proyecto 2

**Objetivo:** activar los interruptores A y B, en cualquier orden, y después llegar a la salida con
energía positiva, sin ser capturado y dentro del límite de turnos.

| Elemento | Regla |
|---|---|
| Interruptores A y B | Entrar a su casilla los activa sin acción adicional y quedan activados toda la partida. |
| Salida | Se puede pisar antes, pero solo permite escapar con A y B activados. |
| Vigilante | Conoce la posición actual del jugador y lo persigue por el camino con menos movimientos (BFS). Respeta muros y bordes, no consume baterías ni recursos y no gasta energía del jugador. Si no hay camino, espera. |

**Orden de cada turno** (los turnos se cuentan desde 1):

1. Actúa el jugador (mueve o espera; el motor aplica costos e interacciones).
2. Si el jugador entró a la casilla del vigilante, es capturado.
3. Actúa el vigilante: en turnos **pares** recalcula con BFS y avanza una casilla; en impares se queda quieto.
4. Captura o energía agotada: derrota. En otro caso se activa el interruptor de destino y se comprueba el escape.

La captura tiene prioridad sobre el escape y el último turno permitido cuenta. El vigilante desempata
rutas con un orden fijo de vecinos (derecha, arriba, abajo, izquierda), así que la persecución es reproducible.
Los mapas sin interruptores ni vigilante (los del proyecto 1) se juegan con las reglas originales.

## Pruebas

```bash
cmake --build build --target check --config Debug     # compila y ejecuta todas las pruebas
ctest --test-dir build --output-on-failure            # Linux, macOS, MinGW
ctest --test-dir build -C Debug --output-on-failure   # Windows con Visual Studio
```

Si está en CLion puede simplemente dirigirse a la barra superior y cambiar el ejecutable a probar (es un menu
deplegable a la izquierda inmediata del símbolo del martillo). En el menú desplegable seleccione el ejecutable `check`
 y presione el martillo (Build). Esto compila y ejecuta todas las pruebas.

| Prueba | Qué cubre |
|---|---|
| `grid_test` | acceso válido e inválido, bordes y esquinas, versiones const, iteradores y orden por filas |
| `environment_test` | acciones disponibles, movimientos, perfiles, término y precedencia, precondiciones del constructor, `CellTraits` (transitable y consumible) |
| `interactions_test` | costos, recurso, batería, trampa y límites de energía |
| `events_test` | los tipos de evento, `std::visit`, fold expressions, `countEvents` con varios contenedores |
| `controllers_test` | `bestBy`, políticas aleatoria y heurística (que en el proyecto 2 va primero a los interruptores pendientes), concept `NavigationPolicy`, `HumanController` y colección polimórfica de `IController` |
| `scenarios_test` | carga de mapas, los dos escenarios de 20 × 30 y sus rutas comprobadas |
| `simulation_test` | `reset(seed)` y simulación reproducible con semilla fija |
| `switches_test` | **proyecto 2:** activación de A y B en ambos órdenes, salida anticipada sin victoria, persistencia, `reset`, energía agotada, último turno, validación del tablero, símbolos `a`/`b` y mapa de solo lectura de la observación |
| `bfs_test` | **proyecto 2:** BFS propio: menor cantidad de movimientos, desempate fijo, muros, destino inalcanzable u origen igual al destino |
| `guard_test` | **proyecto 2:** vigilante: avanza solo en turnos pares, recalcula hacia el jugador, capturas, prioridad de la captura sobre el escape, no consume baterías, espera sin camino, `reset` y validaciones |
| `map_generation_test` | **proyecto 2:** `DisjointSet`, `MapGraph`, Kruskal (árbol y aperturas adicionales, desempate, semilla) |
| `map_writer_test` | **proyecto 2:** guardado del mapa generado como `.txt`, metadatos y recarga con `loadScenario` |
| `validator_test` | **proyecto 2:** validación del lote: rechazo por energía insuficiente y por interruptor encerrado |
| `dijkstra_test` | **proyecto 2:** `Disjtrak` y `Config_algoritmo`: costo mínimo, reconstrucción del camino, muros, energía y validación de escenarios |
| `map_batch_runner_test` | **proyecto 2:** concurrencia: determinismo 1 vs 4 trabajadores, errores por mapa y orden de fases |
| `guard_placement_test` | **proyecto 2:** ubicación de la salida y del vigilante: la comprobación de partida ganable coincide con el motor real (entorno + vigilante con BFS) y los tres niveles entregados se pueden ganar en `hard` (y por lo tanto en `standard` y `easy`) |
| `ui_test` | traducción de teclas, `GameSession` (tecla desconocida sin modificar el entorno, modo humano y automático, resultado final visible), emoji/ASCII de cada celda, render de 20 × 30, pie con el último evento relevante |
| `help_test` | **proyecto 2:** ayuda `H` con tabla dinámica de energía y guía de controles; `P` abre la tabla; ninguna consume turno, energía ni posición |
| `full_game_test` | **proyecto 2:** partida completa reproducible: misma secuencia y estado final tras `reset(seed)` |

Las pruebas del motor no leen `std::cin` ni escriben en `std::cout`; solo `ui_test` y `help_test` se enlazan con FTXUI.

**Prueba negativa de compilación (concept).** `tests/negative/bad_policy.cpp` **no debe compilar**, por
eso no está en CMake. Muestra el diagnóstico cuando una política no cumple `NavigationPolicy`:
```bash
g++ -std=c++20 -Iinclude -fsyntax-only tests/negative/bad_policy.cpp
```
El compilador rechaza la llamada a `runSimulation` porque `BadPolicy::selectAction` devuelve `int` en
lugar de `Action`, así que `BadPolicy` no satisface el concept.

## Ejecución (más enfocada en la primera parte del proyecto)
Para acceder a la instalación de la 2da parte dirijase a la sección **Instalación (proyecto 2)**

```text
escape_circuit [--ascii] [--difficulty easy|standard|hard] [--scenario 1|2] [--level facil|medio|dificil]
                [--controller human|random|heuristic] [--seed N] [--headless]
```

| Opción | Por defecto | Descripción |
|---|---|---|
| `--ascii` | emoji | Dibuja con caracteres ASCII en lugar de emojis |
| `--difficulty` | `standard` | Perfil de reglas (ver tabla de perfiles) |
| `--scenario` | `1` | Mapa de 20 × 30 que se carga desde `assets/maps/` |
| `--level` | — | Mapa generado por `map_generator` (`mapa_facil.txt`, `mapa_medio.txt` o `mapa_dificil.txt`, en `assets/maps/`). Reemplaza a `--scenario`; si se usan las dos, vale la última |
| `--controller` | `human` | Quién decide: el jugador, la política aleatoria o la heurística |
| `--seed` | `42` | Semilla de la partida y del controlador aleatorio |
| `--headless` | — | Juega la partida completa con el controlador automático, sin interfaz, e imprime el resultado |

### Controles

| Tecla | Modo humano | Modo automático (`random` / `heuristic`) |
|---|---|---|
| `W` `A` `S` `D` o flechas | mover arriba / izquierda / abajo / derecha | — |
| `E` | esperar un turno | — |
| `Espacio` o `Enter` | — | el controlador decide el siguiente turno |
| `H` | mostrar u ocultar la ayuda | igual |
| `Q` | salir | igual |

Las letras no distinguen mayúsculas y actúan al instante, sin presionar Enter. Una tecla que no es un
comando muestra un mensaje y no consume turno.

### Ejemplos

**Linux / macOS**
```bash
./build/escape_circuit                                          # jugar: emoji, standard, escenario 1
./build/escape_circuit --ascii --difficulty hard --scenario 2   # jugar en ASCII
./build/escape_circuit --controller heuristic --scenario 2      # ver jugar a la heurística
./build/escape_circuit --controller random --seed 7 --headless  # simulación reproducible
./build/escape_circuit --level medio                            # jugar el mapa generado de nivel medio
```

**Windows (PowerShell o Command Prompt)**
```powershell
.\build\escape_circuit.exe
.\build\escape_circuit.exe --controller heuristic --scenario 2 --headless
```
(con Visual Studio: `.\build\Debug\escape_circuit.exe`; con CLion: `.\cmake-build-debug\escape_circuit.exe`)

El generador de mapas `map_generator` ejecuta el `MapBatchRunner` (Kruskal → Dijkstra →
clasificación) y escribe `assets/maps/mapa_facil.txt`, `mapa_medio.txt` y `mapa_dificil.txt`:

```powershell
cmake --build build --target map_generator --config Debug
.\build\map_generator.exe --seed 42 --maps 10 --workers 4 --extra 2
```

Opciones: `--seed`, `--maps` (hasta 10), `--workers`, `--extra` (aperturas adicionales),
`--output` y `--help`.

Aclaración: Si los mapas generados son imposibles de resolver debido a la ubicación del vigilante, simplemente genere
de nuevo los mapas pero con una semilla distinta.

Salida del modo `--headless`:
```text
$ escape_circuit --controller heuristic --scenario 2 --headless
Semilla 42: ¡Partida completada! Turnos 31 | Energía 32 | Recursos 0 | Puntaje 0
```

Después de generar los mapas, se juega uno de ellos con `--level`:

```powershell
.\build\escape_circuit.exe --level facil
.\build\escape_circuit.exe --level dificil --difficulty hard
```

`--level` elige **el mapa** y `--difficulty` elige **las reglas** (energía, costos y turnos); se pueden
combinar libremente.

**Notas para Windows**
- Desde CLion, la ventana *Run* no es una terminal real y FTXUI solo dibuja la primera línea. Activa
  *Run in external console* en la configuración de `escape_circuit` o ejecútalo desde una terminal.
- Si el `.exe` se cierra sin mostrar nada al ejecutarlo desde PowerShell, probablemente hay otro MinGW
  en el `PATH` con DLL de otra versión. Pon primero en el `PATH` la carpeta `bin` del compilador
  con el que se construyó el proyecto.

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
| 🏁 | `S` | salida (requiere A y B activados) |
| 🔒 / 🔓 | `a` `b` / `*` | interruptor A o B pendiente / activado (proyecto 2) |
| 👮 | `V` | vigilante (proyecto 2) |

La pantalla muestra la barra de estado (turno, energía, puntaje, recursos y, en el proyecto 2, el estado
de A y B y si el vigilante avanza en el próximo turno), la regla de coordenadas,
las 20 filas del tablero y un pie con el último evento relevante y la ayuda breve (cabe en 80 columnas). Cada celda y cada coordenada
ocupa exactamente 2 columnas, así que cada fila mide 62 columnas.

`H` abre la ayuda completa: primero la tabla dinámica de energía (gasto a A, a B y a la
salida, energía disponible y estado) y debajo la guía de controles; se cierra con `H`, `P`
o `Esc`. `P` abre directamente esa misma tabla. Ninguna de estas teclas consume turno.

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

Las pruebas de estos valores están en `environment_test` ("carga perfiles").

## Escenarios

Mapas de texto de 20 líneas × 30 caracteres en `assets/maps/`, con los símbolos ASCII de la tabla
anterior (`@` marca el inicio del agente).

**Símbolos del proyecto 2:** `a` y `b` son los interruptores A y B (en minúscula, porque `B` es la
batería) y `V` marca el inicio del vigilante (su casilla es un espacio libre). Un mapa debe tener un
`a` y un `b`, o ninguno, y como máximo un `V`. Si el mapa tiene `V`, el juego crea el vigilante con BFS.

| Escenario | Descripción | Solución comprobada |
|---|---|---|
| `scenario_01.txt` | Reproducción del ejemplo del enunciado (página 11) | `easy` y `standard`, por dos rutas distintas. **No** es posible en `hard`: la ruta más corta necesita 44 movimientos y solo hay 40 + 2 de energía |
| `scenario_02.txt` | Diseño propio con un paso norte (batería) y un paso sur (terreno elevado) | `easy`, `standard` y `hard` |
| `mapa_facil.txt` | Nivel fácil generado por `map_generator` (costo mínimo 24) | `easy`, `standard` y `hard`, con el vigilante |
| `mapa_medio.txt` | Nivel medio generado (costo mínimo 30) | `easy`, `standard` y `hard`, con el vigilante |
| `mapa_dificil.txt` | Nivel difícil generado (costo mínimo 38) | `easy`, `standard` y `hard`, con el vigilante |

**Ubicación de los elementos en los mapas generados** (`map_layout::chooseEntities`):

- El inicio y los interruptores A y B ocupan salas fijas.
- La salida se ubica en una sala cuyo recorrido mínimo Inicio → A → B → Salida (o B → A) se puede pagar
  con la energía del perfil `hard` (40). Entre esas salas se elige la más cercana a un costo objetivo
  que depende de la semilla del mapa, para que el lote tenga costos distintos y `MapSelector` pueda
  clasificar los niveles fácil, medio y difícil.
- El vigilante se prueba en las salas de la más lejana a la más cercana al inicio y se elige la primera
  desde la que **existe al menos una partida ganadora** en `hard` (`map_layout::winnableWithGuard`).
  Como `easy` y `standard` tienen más energía y más turnos con los mismos costos, el nivel también se
  puede ganar en esos perfiles.
- El validador (`MazeMapValidator`) usa la energía de `hard`; si no existe ninguna combinación ganable,
  rechaza el mapa.

No se guarda ni se reproduce una secuencia ganadora (el profesor indicó que no es necesario): solo se
comprueba que existe, para que ningún nivel quede imposible en ninguna dificultad.

Las rutas están verificadas en `tests/scenarios_test.cpp`. La política heurística completa el escenario 2
en `standard`; en el escenario 1 se queda sin energía (el enunciado no exige que encuentre el camino óptimo).

## Instalación (proyecto 2)

El instalador de Windows (NSIS, generado con CPack) incluye `escape_circuit`, `map_generator` y
los escenarios de `assets/maps/`, incluidos los tres mapas generados. Los programas se enlazan
de forma estática en Release, así que la PC de destino no necesita compilar ni instalar nada más.
Detalles en [`installer/README.md`](installer/README.md).

**Generar el instalador** (requiere CMake, el compilador de CLion y NSIS 3 en el `PATH`):
```powershell
.\installer\build_installer.bat
```
Deja `dist\CircuitoDeEscape-2.0.0-Windows.exe` (instalador) y `dist\CircuitoDeEscape-2.0.0-Windows.zip` (portable).

**Instalar:** ejecutar `CircuitoDeEscape-2.0.0-Windows.exe` (carpeta por defecto
`C:\Program Files\Circuito de Escape`).

**Ejecutar:** menú Inicio → *Circuito de Escape*, o desde una terminal:
```powershell
& "C:\Program Files\Circuito de Escape\bin\escape_circuit.exe" --controller heuristic --scenario 2 --headless
```
El programa instalado busca los mapas en `bin\..\assets\maps`, así que funciona desde cualquier carpeta.

**Desinstalar:** Configuración → Aplicaciones → *Circuito de Escape* → Desinstalar, o el acceso
*Uninstall* del menú Inicio. Se eliminan los programas, los mapas y los accesos directos.

## Reutilización del proyecto 1

| Componente original | Adaptación | Uso actual | Prueba |
|---|---|---|---|
| `Cell` (`std::variant`) y `CellTraits` | nueva celda `Switch` (interruptor A o B) | interruptores del mapa | `switches_test` |
| `NavigationEnvironment` (`step`, `reset`, `state`, término) | activación de interruptores, salida condicionada, vigilante, captura y nuevo orden del turno | único que modifica la partida | `switches_test`, `guard_test`, `environment_test` |
| `Observation` y eventos | estado de A y B, posición del vigilante y mapa de solo lectura (`rows`, `columns`, `map`, `cellAt`); eventos `SwitchActivatedEvent`, `GuardMovedEvent`, `PlayerCapturedEvent` | interfaz y controladores | `switches_test`, `guard_test`, `events_test` |
| Concept `NavigationPolicy` + `PolicyController` + `IController` | mismo patrón para el vigilante: `GuardPolicy` + `GuardPolicyController` + `IGuardController` | `BfsGuardPolicy` persigue al jugador | `guard_test` |
| `Grid` y `Position` | conversión a grafo (`MapGraph`) con índices `fila * columnas + columna` | Kruskal, BFS y Dijkstra | `map_generation_test`, `bfs_test` |
| Cargador de escenarios | símbolos `a`, `b` y `V`; búsqueda de los mapas junto al programa instalado | juego instalado | `switches_test`, `guard_test`, `scenarios_test` |
| Consola FTXUI | dibuja interruptores y vigilante, estado en la barra y resultado "Capturado" | partida interactiva | `ui_test` |
| CMake | dos ejecutables (`escape_circuit`, `map_generator`), reglas de instalación y CPack | instalador | instalación y desinstalación probadas |

## Contenedores principales

| Contenedor | Dónde | Por qué |
|---|---|---|
| `std::array<Cell, Rows * Columns>` | `Grid` | tamaño fijo conocido en compilación, memoria contigua, sin asignación dinámica |
| `std::variant` | `Cell`, `NavigationEvent`, `UiCommand` | conjunto cerrado de alternativas sin herencia; `std::visit` obliga a manejar todos los casos |
| `std::vector<Action>` / `std::vector<NavigationEvent>` | acciones legales, eventos de un paso | tamaño variable pequeño, recorrido en orden; se entrega por `std::span` sin copiar |
| `std::optional<Position>` | `neighbor`, `targetOf` | un movimiento puede no tener destino válido |
| `std::unique_ptr<IController>` | controlador automático | dueño único de un objeto polimórfico elegido en ejecución |
| `std::optional<Action>` | `HumanController` | decisión del teclado pendiente o ausente |
| `std::mt19937` | `RandomPolicy` | generador con semilla controlable: simulaciones reproducibles |
| `std::queue<std::size_t>` | BFS (`pathfinding/bfs.hpp`) | cola FIFO: procesa las casillas por niveles de distancia |
| `std::vector<std::size_t>` | BFS: casilla anterior, camino; `MapGraph`: vecinos | acceso por índice de casilla; la casilla anterior permite reconstruir el camino |
| `std::shared_ptr<IGuardController>` | vigilante en `NavigationEnvironment` | controlador polimórfico que se comparte si el entorno se copia |
| `std::set<int>` | `MapSelector` | cuenta los costos distintos de un lote |
| `std::list`, `std::deque` | pruebas | comprueban que los algoritmos genéricos no dependen del contenedor |

No se usa `std::map` ni `std::unordered_map`: las celdas se indexan por posición directamente en el
`std::array` (`fila * columnas + columna`), así que un mapa no aporta nada.

## Estructura del repositorio

```text
include/circuit_escape/   motor (grid, celdas, posición, reglas, eventos, entorno, controladores,
                          escenarios, simulación) y la cabecera de la interfaz (console_ui.hpp)
  graph/                  MapGraph (conversión desde Grid) y DisjointSet
  pathfinding/            BFS y resultado de búsqueda (SearchResult)
  map_generation/         Kruskal, MapBuilder y MapSelector
  controllers/            controlador del vigilante (GuardPolicy, BfsGuardPolicy)
src/                      implementación no genérica: controladores, interfaz FTXUI, grafo y selección
app/main.cpp              juego escape_circuit: opciones, bucle de FTXUI y modo headless
tests/                    pruebas con assert registradas en CTest (+ pathfinding/, map_generation/, negative/)
assets/maps/              escenarios de 20 × 30
installer/                instalador: reglas de CPack/NSIS, script y guía
docs/                     design.md y contributions.md
```

## Créditos y licencias

- **FTXUI v7.0.3**, de Arthur Sonzogni (<https://github.com/ArthurSonzogni/FTXUI>), licencia MIT.
  Se usa solo en la capa de presentación y CMake la descarga con `FetchContent`; su código no se
  incluye en este repositorio.
- **Patrón `Overloaded`** (`include/circuit_escape/overloaded.hpp`): adaptado del ejemplo de
  `std::visit` en cppreference (<https://en.cppreference.com/w/cpp/utility/variant/visit>).
- **Escenario 1** (`assets/maps/scenario_01.txt`): reproduce el ejemplo de 20 × 30 de la página 11
  del enunciado del curso.
- **Asistencia de IA:** partes del código, de las pruebas y de la documentación se hicieron con ayuda
  de Claude (Anthropic), como se detalla en [`docs/contributions.md`](docs/contributions.md) y en los
  comentarios `[Se usó IA …]` del código. Todo fue revisado, compilado y probado por el grupo.

## Entrega

La entrega del proyecto 1 es el tag `proyecto-1-entrega`. La del proyecto 2 es el commit marcado con
el tag anotado `proyecto-2-entrega` en la rama `main`:
```bash
git tag -a proyecto-2-entrega -m "Entrega del proyecto 2"
git push origin proyecto-2-entrega
```
Antes de crear el tag: clonar el repositorio en una carpeta nueva, seguir solo este README, compilar,
correr las pruebas y probar la demostración.
