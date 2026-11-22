// Pruebas del validador del lote (sección 7 del enunciado): rechazo por energía
// insuficiente y por un interruptor encerrado, además del caso válido.
// No modifica map_generation_test.cpp (propiedad de Eduardo).
#include <cassert>
#include <iostream>
#include <string>

#include "circuit_escape/map_generation/map_batch_runner.hpp"

namespace {

constexpr std::size_t Rows = 5;
constexpr std::size_t Columns = 5;
using TestMap = BuiltMap<Rows, Columns>;

bool contains(const std::string& text, const std::string& fragment)
{
    return text.find(fragment) != std::string::npos;
}

// Todas las salas libres: inicio (0,0), A (0,2), B (0,4) y salida (4,4).
TestMap openMap(std::size_t id = 1)
{
    TestMap map;
    map.id = id;
    map.seed = 7;
    return map;
}

} // namespace

int main()
{
    MazeMapValidator<Rows, Columns> standard;

    // Caso base: el recorrido Inicio -> A -> B -> Salida cabe en la energía inicial.
    const ValidationResult valid = standard.validate(openMap());
    assert(valid.id == 1);
    assert(valid.cost > 0);
    assert(valid.hasVictory);
    assert(valid.message.empty());

    // Prueba 13: energía insuficiente. Llegar con energía 0 NO cuenta como victoria,
    // así que el validador exige estrictamente más energía que el mejor recorrido.
    MazeMapValidator<Rows, Columns> lowEnergy{2};
    const ValidationResult poor = lowEnergy.validate(openMap(2));
    assert(!poor.hasVictory);
    assert(poor.cost >= 2);
    assert(contains(poor.message, "energia insuficiente"));

    MazeMapValidator<Rows, Columns> exactEnergy{valid.cost};
    const ValidationResult exact = exactEnergy.validate(openMap(3));
    assert(!exact.hasVictory);
    assert(contains(exact.message, "energia insuficiente"));

    // Prueba 14: un interruptor encerrado. (0,2) sigue siendo una sala transitable,
    // pero sus tres accesos son muros: ninguno de los dos órdenes A/B puede completarse.
    TestMap trapped = openMap(4);
    trapped.grid.at({0, 1}) = Wall{};
    trapped.grid.at({0, 3}) = Wall{};
    trapped.grid.at({1, 2}) = Wall{};
    const ValidationResult isolated = standard.validate(trapped);
    assert(!isolated.hasVictory);
    assert(isolated.cost == -1);
    assert(contains(isolated.message, "objetivo inaccesible"));

    // El frontón no debe afectar a otros mapas: el validador no muta el tablero.
    const ValidationResult again = standard.validate(openMap(1));
    assert(again.hasVictory && again.cost == valid.cost);

    std::cout << "validator_test: todas las pruebas pasaron\n";
    return 0;
}
