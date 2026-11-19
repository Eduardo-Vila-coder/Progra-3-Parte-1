#pragma once
#include <cstddef>
#include <string>
#include <vector>

// Lo que entrega la validación por cada mapa
struct ValidatedMap
{
    std::size_t id;   // número del mapa (el mismo de BuiltMap)
    int cost;         // menor gasto de energía de los dos recorridos completos
    bool hasVictory;  // true si tiene una partida ganadora comprobada
};

// Cómo terminó la selección
enum class SelectionStatus
{
    Ok,                     // se eligieron los 3 niveles
    NoWinnableMaps,         // ningún mapa tiene victoria comprobada
    NotEnoughDistinctCosts  // hay menos de 3 costos distintos
};

// Lo que devuelve MapSelector
struct SelectionResult
{
    SelectionStatus status;
    std::string message;   // explicación para mostrar al usuario
    ValidatedMap easy{};
    ValidatedMap medium{};
    ValidatedMap hard{};
};

class MapSelector
{
public:
    // Función principal: recibe todos los mapas validados y elige uno por nivel
    [[nodiscard]] static SelectionResult select(const std::vector<ValidatedMap> &maps);

private:
    // Se queda solo con los mapas que tienen victoria comprobada
    static std::vector<ValidatedMap> keepWinnable(const std::vector<ValidatedMap> &maps);

    // Cuenta cuántos costos distintos hay
    static std::size_t countDistinctCosts(const std::vector<ValidatedMap> &maps);

    // Promedio de los costos: suma de costos / cantidad de mapas
    static double averageCost(const std::vector<ValidatedMap> &maps);

    // El de menor costo (si empatan, el de id más bajo)
    static ValidatedMap pickEasy(const std::vector<ValidatedMap> &maps);

    // El de mayor costo (si empatan, el de id más bajo)
    static ValidatedMap pickHard(const std::vector<ValidatedMap> &maps);

    // Sin contar los costos mínimo y máximo, el más cercano al promedio
    // (si empatan en distancia, el de id más bajo)
    static ValidatedMap pickMedium(const std::vector<ValidatedMap> &maps, double average, int minCost, int maxCost);
};