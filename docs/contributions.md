# Contribuciones

Resumen de responsabilidades y aportes de cada integrante. Cada aporte se puede verificar en el
historial del repositorio (`git log --author="<nombre>"`) y en los pull requests integrados en `main`.

| Integrante | Usuario de GitHub | Área principal |
|---|---|---|
| Eduardo Raúl Vila Castellares | Eduardo-Vila-coder | Repositorio, reglas y perfiles de dificultad |
| Yerik Dylan Vega Santillan | Yerik-Vega | Agente, resolución de acciones, reset, concepts y simulación |
| Cristhian Gabriel Jinchuña Cama | cristhianjinchuna-dev | Energía, interacciones, eventos, pruebas y documentación |
| Mathias Alonso Cavalcanti Estacio | MatCavUTEC | Controladores, escenarios, término e integración del juego |
| Mathius Edgar Quispe Sicha | hfuv (MQS144) | Tablero, celdas, posición e interfaz de consola |

## Detalle por integrante

### Eduardo Raúl Vila Castellares — Repositorio, reglas y perfiles
- Creación del repositorio (los pull requests se abren desde su cuenta).
- `NavigationEnvironment::availableActions()` y sus casos borde (esquinas, muros).
- `GameRules` con los perfiles `easy`, `standard` y `hard` (`game_rules.hpp`) y sus pruebas.
- Pruebas de consulta y realización de movimientos en `environment_test.cpp`.
- Primera versión de `observation.hpp`.

### Yerik Dylan Vega Santillan — Agente, acciones, reset, concepts y simulación
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

### Mathias Alonso Cavalcanti Estacio — Controladores, escenarios, término e integración
- Condiciones de término y su precedencia (`goalReached` > `noEnergy` > `turnLimit`).
- Política aleatoria con semilla y política heurística (distancia Manhattan, sin retroceder), template
  `bestBy` y `controllers_test.cpp`.
- Escenarios de 20 × 30 (`assets/maps/`), cargador `scenario.hpp` y `scenarios_test.cpp`.
- `IController` y `PolicyController<Policy>`, y la aplicación `app/main.cpp` (opciones de línea de
  comandos, modo automático y `--headless`).

### Mathius Edgar Quispe Sicha (hfuv / MQS144) — Tablero, dominio e interfaz
- `Grid<CellType, Rows, Columns>` con `std::array`, acceso validado e iteradores.
- Tipos de celda y `CellTraits` (`cells.hpp`).
- `Position`, `Action`, `neighbor` y `toString` (`position.hpp`).
- Interfaz de consola con FTXUI (`console_ui.hpp`, `src/console_ui.cpp`): render emoji/ASCII,
  coordenadas y ayuda.

## Revisión de pull requests

Los cambios se integraron en `main` mediante pull requests desde ramas por tarea (por ejemplo
`desarrollo_game_rules`, `Condiciones-de-termino`, `energia_interacciones`, `Concepts`,
`eventos-variant-variadicos`, `interfaz_consolaa`, `arreglando-errores`).

| Pull request | Rama | Autor | Revisó e integró |
|---|---|---|---|
| #11 | `desarrollo_game_rules` | Eduardo Raúl Vila Castellares | Yerik Dylan Vega Santillan |
| #15 | `Condiciones-de-termino` | Mathias Alonso Cavalcanti Estacio | Yerik Dylan Vega Santillan |
| #16 | `energia_interacciones` | Cristhian Gabriel Jinchuña Cama | Yerik Dylan Vega Santillan |
| #21 | `Concepts` | Yerik Dylan Vega Santillan | Mathias Alonso Cavalcanti Estacio |
| #23 | `ordenar-atributos-y-metodos-de-environment` | Eduardo Raúl Vila Castellares | Yerik Dylan Vega Santillan |
| #25 | `eventos-variant-variadicos` | Cristhian Gabriel Jinchuña Cama | Yerik Dylan Vega Santillan |
| #26 | `eventos-variant-variadicos` | Cristhian Gabriel Jinchuña Cama | Yerik Dylan Vega Santillan |
| #27 | `fix_ordenamiento` | Eduardo Raúl Vila Castellares | Yerik Dylan Vega Santillan |
| #28 | `interfaz_consolaa` | Mathius Edgar Quispe Sicha | Yerik Dylan Vega Santillan |
| #30 | `arreglando-errores` | Mathias Alonso Cavalcanti Estacio | Cristhian Gabriel Jinchuña Cama |
| #31 | `corrigiendo_bugs` | Cristhian Gabriel Jinchuña Cama | Mathias Alonso Cavalcanti Estacio |

## Uso de herramientas de IA generativa

Algunas partes se desarrollaron con ayuda de asistentes de IA, como se indica en los comentarios del
código (`[Se usó IA …]`). Las pruebas `grid_test` y `ui_test`, las precondiciones del constructor,
`CellTraits`, `HumanController` y la documentación se hicieron con asistencia de Claude. Todo el código fue revisado, compilado y probado
por el grupo.
