cmake_minimum_required(VERSION 3.20)
project(circuit_escape CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Indicar a CMake dónde buscar las cabeceras (.hpp)
include_directories(include)

# Integración de FTXUI fijada a una versión concreta (enunciado, sección 5.8)
include(FetchContent)
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
        include/circuit_escape/controllers.hpp
        include/circuit_escape/game_rules.hpp
        src/environment.cpp
        src/controllers.cpp
)

# Aplicación ejecutable principal (app/main.cpp)
add_executable(navigation_game
        app/main.cpp
        include/circuit_escape/cells.hpp
        include/circuit_escape/controllers.hpp
        include/circuit_escape/environment.hpp
        include/circuit_escape/grid.hpp
        include/circuit_escape/observation.hpp
        include/circuit_escape/position.hpp
)
# FTXUI se limita al ejecutable (capa de presentación); el core y las pruebas no dependen de ella
target_link_libraries(navigation_game PRIVATE circuit_escape_core ftxui::screen ftxui::dom ftxui::component)

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