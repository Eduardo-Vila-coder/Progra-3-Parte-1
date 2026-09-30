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
FetchContent_MakeAvailable(ftxui)

# Biblioteca central (Core)
add_library(circuit_escape_core
        include/circuit_escape/grid.hpp
        include/circuit_escape/cells.hpp
        include/circuit_escape/environment.hpp
        include/circuit_escape/events.hpp
        include/circuit_escape/controllers.hpp
        include/circuit_escape/game_rules.hpp
        src/environment.cpp
        src/controllers.cpp
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

# Aplicación ejecutable principal (app/main.cpp)
add_executable(navigation_game app/main.cpp)
target_link_libraries(navigation_game PRIVATE circuit_escape_ui)

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
