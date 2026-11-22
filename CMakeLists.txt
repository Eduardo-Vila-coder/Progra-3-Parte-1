cmake_minimum_required(VERSION 3.20)
project(circuit_escape CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Los fuentes tienen emojis y tildes: MSVC (Visual Studio) debe leerlos como UTF-8
if(MSVC)
    add_compile_options(/utf-8)
endif()

# Indicar a CMake dónde buscar las cabeceras (.hpp)
include_directories(include)

# Integración de FTXUI fijada a una versión concreta (enunciado, sección 5.8)
include(FetchContent)
# Si existe una copia local de FTXUI en external/ftxui (carpeta ignorada por git), se usa esa
# en lugar de descargarla. Útil sin internet o con una conexión lenta a GitHub.
if(NOT FETCHCONTENT_SOURCE_DIR_FTXUI AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/external/ftxui/CMakeLists.txt")
    set(FETCHCONTENT_SOURCE_DIR_FTXUI "${CMAKE_CURRENT_SOURCE_DIR}/external/ftxui")
endif()
FetchContent_Declare(ftxui
        GIT_REPOSITORY https://github.com/ArthurSonzogni/FTXUI
        GIT_TAG v7.0.3
        GIT_SHALLOW TRUE
)
# El instalador solo debe llevar nuestros programas, no las cabeceras ni las bibliotecas de FTXUI
set(FTXUI_ENABLE_INSTALL OFF CACHE BOOL "No instalar FTXUI" FORCE)
FetchContent_MakeAvailable(ftxui)

# Biblioteca central (Core)
add_library(circuit_escape_core
        include/circuit_escape/grid.hpp
        include/circuit_escape/cells.hpp
        include/circuit_escape/environment.hpp
        include/circuit_escape/events.hpp
        include/circuit_escape/controllers.hpp
        include/circuit_escape/game_rules.hpp
        include/circuit_escape/graph/graph.hpp
        include/circuit_escape/map_generation/map_builder.hpp
        include/circuit_escape/map_generation/map_layout.hpp
        include/circuit_escape/map_generation/map_writer.hpp
        include/circuit_escape/map_generation/map_selector.hpp
        include/circuit_escape/pathfinding/search_result.hpp
        include/circuit_escape/pathfinding/bfs.hpp
        include/circuit_escape/pathfinding/logica_algoritmo.hpp
        include/circuit_escape/pathfinding/config_algoritmo.hpp
        include/circuit_escape/map_generation/map_batch_runner.hpp
        include/circuit_escape/controllers/guard_controller.hpp
        src/environment.cpp
        src/controllers.cpp
        src/graph/graph.cpp
        src/map_generation/map_selector.cpp
        src/controllers/guard_controller.cpp
)
# Ruta absoluta de los mapas: los escenarios se cargan igual desde cualquier carpeta de compilación
target_compile_definitions(circuit_escape_core PUBLIC
        CIRCUIT_ESCAPE_MAPS_DIR="${CMAKE_CURRENT_SOURCE_DIR}/assets/maps")

# Capa de presentación (consola): la ÚNICA que depende de FTXUI; el core y sus pruebas no la usan
add_library(circuit_escape_ui
        include/circuit_escape/console_ui.hpp
        src/console_ui.cpp
)
target_link_libraries(circuit_escape_ui PUBLIC circuit_escape_core ftxui::screen ftxui::dom ftxui::component)

# Aplicación ejecutable principal: el juego (app/main.cpp). Nombre exigido por el proyecto 2 (5.3)
add_executable(escape_circuit app/main.cpp)
target_link_libraries(escape_circuit PRIVATE circuit_escape_ui)

# Generador de mapas (proyecto 2, 5.3): ejecuta MapBatchRunner y escribe los tres .txt.
# Solo usa el core (Kruskal, Dijkstra, guardado de mapas), no la consola con FTXUI.
add_executable(map_generator app/map_generator_main.cpp)
target_link_libraries(map_generator PRIVATE circuit_escape_core)

# Pruebas Unitarias explícitas (tests/)
enable_testing()

add_executable(grid_test tests/grid_test.cpp)
target_link_libraries(grid_test PRIVATE circuit_escape_core)
add_test(NAME GridTest COMMAND grid_test)

add_executable(environment_test tests/environment_test.cpp)
target_link_libraries(environment_test PRIVATE circuit_escape_core)
add_test(NAME EnvironmentTest COMMAND environment_test)

add_executable(interactions_test tests/interactions_test.cpp)
target_link_libraries(interactions_test PRIVATE circuit_escape_core)
add_test(NAME InteractionsTest COMMAND interactions_test)

add_executable(controllers_test tests/controllers_test.cpp)
target_link_libraries(controllers_test PRIVATE circuit_escape_core)
add_test(NAME ControllersTest COMMAND controllers_test)

add_executable(scenarios_test tests/scenarios_test.cpp)
target_link_libraries(scenarios_test PRIVATE circuit_escape_core)
add_test(NAME ScenariosTest COMMAND scenarios_test)

add_executable(events_test tests/events_test.cpp)
target_link_libraries(events_test PRIVATE circuit_escape_core)
add_test(NAME EventsTest COMMAND events_test)

add_executable(simulation_test tests/simulation_test.cpp)
target_link_libraries(simulation_test PRIVATE circuit_escape_core)
add_test(NAME SimulationTest COMMAND simulation_test)

# La prueba de la interfaz es la única que se enlaza con FTXUI (a través de circuit_escape_ui)
add_executable(ui_test tests/ui_test.cpp)
target_link_libraries(ui_test PRIVATE circuit_escape_ui)
add_test(NAME UiTest COMMAND ui_test)

add_executable(switches_test tests/switches_test.cpp)
target_link_libraries(switches_test PRIVATE circuit_escape_core)
add_test(NAME SwitchesTest COMMAND switches_test)

add_executable(bfs_test tests/pathfinding/bfs_test.cpp)
target_link_libraries(bfs_test PRIVATE circuit_escape_core)
add_test(NAME BfsTest COMMAND bfs_test)

add_executable(guard_test tests/guard_test.cpp)
target_link_libraries(guard_test PRIVATE circuit_escape_core)
add_test(NAME GuardTest COMMAND guard_test)

add_executable(map_generation_test tests/map_generation/map_generation_test.cpp)
target_link_libraries(map_generation_test PRIVATE circuit_escape_core)
add_test(NAME MapGenerationTest COMMAND map_generation_test)

add_executable(map_writer_test tests/map_generation/map_writer_test.cpp)
target_link_libraries(map_writer_test PRIVATE circuit_escape_core)
add_test(NAME MapWriterTest COMMAND map_writer_test)

add_executable(map_batch_runner_test tests/concurrency/batch_test.cpp)
target_link_libraries(map_batch_runner_test PRIVATE circuit_escape_core)
add_test(NAME MapBatchRunnerTest COMMAND map_batch_runner_test)

add_executable(dijkstra_test tests/pathfinding/dijkstra_test.cpp)
target_link_libraries(dijkstra_test PRIVATE circuit_escape_core)
add_test(NAME DijkstraTest COMMAND dijkstra_test)

add_executable(help_test tests/help_test.cpp)
target_link_libraries(help_test PRIVATE circuit_escape_ui)
add_test(NAME HelpTest COMMAND help_test)

add_executable(full_game_test tests/full_game_test.cpp)
target_link_libraries(full_game_test PRIVATE circuit_escape_core)
add_test(NAME FullGameTest COMMAND full_game_test)

# Objetivo único para compilar y ejecutar TODAS las pruebas.
# No depende de escape_circuit: verificar las pruebas no requiere enlazar el juego.
# En Windows con MinGW los ejecutables necesitan el directorio del compilador en PATH
# (libstdc++-6.dll, libgcc_s_seh-1.dll y libwinpthread-1.dll); si no, CTest devuelve 0xc0000135.
get_filename_component(CIRCUIT_ESCAPE_COMPILER_DIR "${CMAKE_CXX_COMPILER}" DIRECTORY)
add_custom_target(check
        COMMAND ${CMAKE_COMMAND} -E env
                "PATH=${CIRCUIT_ESCAPE_COMPILER_DIR};$ENV{PATH}"
                ${CMAKE_CTEST_COMMAND} --output-on-failure -C $<CONFIG>
        DEPENDS grid_test environment_test interactions_test controllers_test scenarios_test
                events_test simulation_test ui_test switches_test bfs_test guard_test
                map_generation_test map_writer_test map_batch_runner_test dijkstra_test
                help_test full_game_test
        WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
        COMMENT "Compila y ejecuta todas las pruebas con CTest"
        USES_TERMINAL)

# Instalador de Windows con CPack + NSIS (proyecto 2, 8.1). Ver installer/README.md
include(installer/installer.cmake)
