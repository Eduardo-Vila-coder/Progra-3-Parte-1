#include "circuit_escape/map_generation/map_selector.hpp"
#include <set>
#include <utility>
#include <cmath>

SelectionResult MapSelector::select(const std::vector<ValidatedMap> &maps) {
    std::vector<ValidatedMap> filteringMaps = keepWinnable(maps);
    if (filteringMaps.empty()) {
        return {SelectionStatus::NoWinnableMaps, "No hay ningun mapa ganable"};
    }

    if (countDistinctCosts(filteringMaps) < 3) {
        return {SelectionStatus::NotEnoughDistinctCosts,
            "No hay suficientes costos distintos, genere un nuevo lote con otra semilla"};
    }

    auto easy = pickEasy(filteringMaps);
    auto hard = pickHard(filteringMaps);
    auto medium = pickMedium(filteringMaps, averageCost(filteringMaps), easy.cost, hard.cost);

    return {SelectionStatus::Ok, "Mapas seleccionados con exito", easy, medium, hard};
}

std::vector<ValidatedMap> MapSelector::keepWinnable(const std::vector<ValidatedMap> &maps) {
    std::vector<ValidatedMap> validatedMaps;

    for (const auto &map : maps) {
        if (map.hasVictory) {
            validatedMaps.push_back(map);
        }
    }
    return validatedMaps;
}

std::size_t MapSelector::countDistinctCosts(const std::vector<ValidatedMap> &maps) {
    std::set<int> costos{};

    for (const auto &map : maps) {
        if (!costos.contains(map.cost)) {
            costos.insert(map.cost);
        }
    }
    return costos.size();
}

double MapSelector::averageCost(const std::vector<ValidatedMap> &maps) {
    double average = 0.0;

    for (const auto &map : maps) {
        average += map.cost;
    }
    average /= static_cast<double>(maps.size());
    return average;
}

ValidatedMap MapSelector::pickEasy(const std::vector<ValidatedMap> &maps) {
    ValidatedMap best = maps.front();   // empieza con el primero

    for (const auto &map : maps) {
        // menor costo, o mismo costo pero id más bajo
        if (map.cost < best.cost || (map.cost == best.cost && map.id < best.id)) {
            best = map;
        }
    }
    return best;
}

ValidatedMap MapSelector::pickHard(const std::vector<ValidatedMap> &maps) {
    ValidatedMap best = maps.front();

    for (const auto &map : maps) {
        // mayor costo, o mismo costo pero id más bajo
        if (map.cost > best.cost || (map.cost == best.cost && map.id < best.id)) {
            best = map;
        }
    }
    return best;
}

ValidatedMap MapSelector::pickMedium(const std::vector<ValidatedMap> &maps, double average, int minCost, int maxCost) {
    std::pair<double, ValidatedMap> parValido {-1,{}};

    for (const auto &map : maps) {
        if (!(map.cost == minCost || map.cost == maxCost)) {
            auto distancia = std::abs(map.cost - average);
            if (parValido.first == -1 || distancia < parValido.first ||
                (distancia == parValido.first && map.id < parValido.second.id)) {
                parValido.first = distancia;
                parValido.second = map;
            }
        }
    }
    return parValido.second;
}