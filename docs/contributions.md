# Contribuciones

Resumen de responsabilidades y aportes de cada integrante. Cada aporte se puede verificar en el
historial del repositorio (`git log --author="<nombre>"`) y en los pull requests integrados en `main`.

> Pendiente de completar: códigos de alumno, nombre completo del integrante `MQS144` y revisores de cada PR.

| Integrante | Usuario de GitHub | Área principal |
|---|---|---|
| Eduardo Raúl Vila Castellares | Eduardo-Vila-coder | Repositorio, reglas y perfiles de dificultad |
| Yerik Vega | Yerik-Vega | Agente, resolución de acciones, reset, concepts y simulación |
| Cristhian Gabriel Jinchuña Cama | cristhianjinchuna-dev | Energía, interacciones, eventos, pruebas y documentación |
| Mathias Cavalcanti | MatCavUTEC | Controladores, escenarios, término e integración del juego |
| _(completar)_ | hfuv (MQS144) | Tablero, celdas, posición e interfaz de consola |

## Detalle por integrante

### Eduardo Raúl Vila Castellares — Repositorio, reglas y perfiles
- Creación del repositorio (los pull requests se abren desde su cuenta).
- `NavigationEnvironment::availableActions()` y sus casos borde (esquinas, muros).
- `GameRules` con los perfiles `easy`, `standard` y `hard` (`game_rules.hpp`) y sus pruebas.
- Pruebas de consulta y realización de movimientos en `environment_test.cpp`.
- Primera versión de `observation.hpp`.

### Yerik Vega — Agente, acciones, reset, concepts y simulación
- Atributos del agente e invariantes (5.3) y resolución de acciones en `step`.
- `reset(seed)` con copia del tablero original y semilla.
- Concept `NavigationPolicy` y políticas de navegación en `controllers.hpp`.
- Simulación reproducible `runSimulation` (`simulation.hpp`) y `simulation_test.cpp`.
- Prueba negativa de compilación del concept (`tests/negative/bad_policy.cpp`).
- Revisión e integración de la mayoría de los pull requests.

### Cristhian Gabriel Jinchuña Cama — Energía, eventos, pruebas y documentación
- Estructura inicial de carpetas, `CMakeLists.txt` e integración de FTXUI v7.0.3.
- Energía e interacciones con celdas: costos, recurso, batería, trampa y límites de energía, con
  `interactions_test.cpp`.
- Eventos, `std::variant` y templates variádicos: `events.hpp`, `appendEvents`, `holdsAnyOf`,
  `countEvents`, `describe` y `events_test.cpp`.
- Precondiciones del constructor (`validate`), `CellTraits` con rasgos transitable y consumible y su uso
  en el entorno, iteradores no const y `static_assert` de `Grid`.
- `HumanController`, `GameSession` (ciclo de la partida interactiva, probado en `ui_test`) y pie de la
  consola con el último evento relevante.
- Pruebas `grid_test.cpp` y `ui_test.cpp`, y registro de `simulation_test` en CTest.
- `README.md` y `docs/design.md`.

### Mathias Cavalcanti — Controladores, escenarios, término e integración
- Condiciones de término y su precedencia (`goalReached` > `noEnergy` > `turnLimit`).
- Política aleatoria con semilla y política heurística (distancia Manhattan, sin retroceder), template
  `bestBy` y `controllers_test.cpp`.
- Escenarios de 20 × 30 (`assets/maps/`), cargador `scenario.hpp` y `scenarios_test.cpp`.
- `IController` y `PolicyController<Policy>`, y la aplicación `app/main.cpp` (opciones de línea de
  comandos, modo automático y `--headless`).

### _(completar)_ (hfuv / MQS144) — Tablero, dominio e interfaz
- `Grid<CellType, Rows, Columns>` con `std::array`, acceso validado e iteradores.
- Tipos de celda y `CellTraits` (`cells.hpp`).
- `Position`, `Action`, `neighbor` y `toString` (`position.hpp`).
- Interfaz de consola con FTXUI (`console_ui.hpp`, `src/console_ui.cpp`): render emoji/ASCII,
  coordenadas y ayuda.

## Revisión de pull requests

Los cambios se integraron en `main` mediante pull requests desde ramas por tarea (por ejemplo
`desarrollo_game_rules`, `Condiciones-de-termino`, `energia_interacciones`, `Concepts`,
`eventos-variant-variadicos`, `interfaz_consolaa`, `arreglando-errores`).

| Pull request | Autor | Revisor |
|---|---|---|
| _(completar)_ | | |

## Uso de herramientas de IA generativa

Algunas partes se desarrollaron con ayuda de asistentes de IA, como se indica en los comentarios del
código (`[Se usó IA …]`). Las pruebas `grid_test` y `ui_test`, las precondiciones del constructor,
`CellTraits`, `HumanController` y la documentación se hicieron con asistencia de Claude. Todo el código fue revisado, compilado y probado
por el grupo.
