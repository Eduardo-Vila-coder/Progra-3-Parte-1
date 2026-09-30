# Contribuciones

Resumen de responsabilidades y aportes de cada integrante. Cada aporte se puede verificar en el
historial del repositorio (`git log --author="<nombre>"`) y en los pull requests integrados en `main`.

> Los códigos de alumno y el nombre completo del integrante `MQS144` están pendientes de completar.

| Integrante | Usuario de GitHub | Área principal |
|---|---|---|
| Eduardo Raúl Vila Castellares | Eduardo-Vila-coder | Integración, reglas y perfiles de dificultad |
| Yerik Vega | Yerik-Vega | Agente, resolución de acciones, reset, concepts y simulación |
| Cristhian Gabriel Jinchuña Cama | cristhianjinchuna-dev | Energía, interacciones, eventos, interfaz de consola y documentación |
| Mathias Cavalcanti | MatCavUTEC | Controladores automáticos, escenarios y condiciones de término |
| _(completar)_ | hfuv (MQS144) | Tablero, celdas y posición |

## Detalle por integrante

### Eduardo Raúl Vila Castellares — Integración, reglas y perfiles
- Creación del repositorio e integración de los pull requests en `main`.
- `NavigationEnvironment::availableActions()` y sus casos borde (esquinas, muros).
- `GameRules` con los perfiles `easy`, `standard` y `hard` (`game_rules.hpp`) y sus pruebas.
- Pruebas de consulta y realización de movimientos en `environment_test.cpp`.
- Primera versión de `observation.hpp`.

### Yerik Vega — Agente, acciones, reset, concepts y simulación
- Atributos del agente e invariantes (5.3) y resolución de acciones en `step`.
- `reset(seed)` con copia del tablero original y semilla (`environment.hpp`).
- Concept `NavigationPolicy` y políticas de navegación en `controllers.hpp`.
- Simulación reproducible `runSimulation` (`simulation.hpp`) y `simulation_test.cpp`.
- Prueba negativa de compilación del concept (`tests/negative/bad_policy.cpp`).

### Cristhian Gabriel Jinchuña Cama — Energía, eventos, interfaz y documentación
- Estructura inicial de carpetas, `CMakeLists.txt` e integración de FTXUI v7.0.3.
- Energía e interacciones con celdas: costos, recurso, batería, trampa y límites de energía
  (`environment.hpp`) con `interactions_test.cpp`.
- Eventos, `std::variant` y templates variádicos: `events.hpp`, `Overloaded`, `appendEvents`,
  `holdsAnyOf`, `countEvents`, `describe` y `events_test.cpp`.
- Interfaz de consola con FTXUI: `ConsoleUI`, `GameSession`, modos emoji y ASCII, opciones de línea de
  comandos (`app/`) y `ui_test.cpp`.
- `IController`, `PolicyController<Policy>`, `HumanController` y `makeController` con `polymorphism_test.cpp`.
- Pruebas de `Grid` (`grid_test.cpp`), uso de `CellTraits` en el entorno, `static_assert` de dimensiones
  y precondiciones del constructor.
- `README.md` y `docs/design.md`.

### Mathias Cavalcanti — Controladores, escenarios y término
- Condiciones de término y su precedencia (`goalReached` > `noEnergy` > `turnLimit`).
- Política aleatoria con semilla y política heurística (distancia Manhattan, sin retroceder), template
  `bestBy` y `controllers_test.cpp`.
- Escenarios de 20 × 30 (`assets/maps/`), cargador `scenario.hpp` y `scenarios_test.cpp` con las rutas
  comprobadas.

### _(completar)_ (hfuv / MQS144) — Tablero y dominio
- `Grid<CellType, Rows, Columns>` con `std::array`, acceso validado e iteradores.
- Tipos de celda y `CellTraits` (`cells.hpp`).
- `Position`, `Action`, `neighbor` y `toString` (`position.hpp`).

## Revisión de pull requests

Los pull requests se integraron en `main` desde ramas por tarea (por ejemplo `desarrollo_game_rules`,
`Condiciones-de-termino`, `energia_interacciones`, `Concepts`, `eventos-variant-variadicos`,
`ordenar-atributos-y-metodos-de-environment`).

_(Completar: quién revisó cada pull request. Cada integrante debe haber revisado al menos uno ajeno.)_

## Uso de herramientas de IA generativa

Algunas partes se desarrollaron con ayuda de asistentes de IA, como se indica en los comentarios del
código (`[Se usó IA …]`). La interfaz de consola, los controladores polimórficos, las pruebas de `Grid`
y la documentación se hicieron con asistencia de Claude; todo el código fue revisado, compilado y probado
por el grupo.
