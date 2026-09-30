// PRUEBA NEGATIVA DE COMPILACIÓN (enunciado 8)
// Este archivo NO debe compilar: a propósito NO está en CMakeLists.txt.
// Muestra el diagnóstico del compilador cuando una política no satisface el concept NavigationPolicy.
// Para verlo (desde la raíz del proyecto):
//   g++ -std=c++20 -Iinclude -c tests/negative/concept_negative.cpp -o NUL
#include "../../include/circuit_escape/environment.hpp"
#include "../../include/circuit_escape/simulation.hpp"

// Política incorrecta: devuelve int en lugar de Action
struct BadPolicy {
    int selectAction(const Observation&, std::span<const Action>) { return 0; }
};

int main() {
    Grid<Cell, 3, 4> grid{};
    grid.at({0, 1}) = Exit{};
    NavigationEnvironment<3, 4> environment{grid, Position(0, 0), 10, 20};
    BadPolicy policy;
    (void)runSimulation(environment, policy, 42); // error: BadPolicy no satisface NavigationPolicy
}