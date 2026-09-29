#pragma once

#include <cstddef>

enum class Difficulty {
    Standard,
    Easy,
    Hard
};

template <Difficulty = Difficulty::Standard>
struct GameRules {
    int initialMaxEnergy{60};   // Energia inicial y maxima
    std::size_t turnLimit{180};
    int energyCostGeneral{1};   // Para Cells Empty, ResourceCell, Battery, Trap, Exit
    int energyCostRoughTerrain{2};  // Para Cell RoughTerrain
    int energyCostOthers{1};    // Para Action::wait e intento invalido
    int reward{10};     // Recompensa de ResourceCell
    int energy{3};      // Energia de Battery
    int energyPenalty{2};   // Penalizacion de energia en Trap
    int scorePenalty{1};    // Penalizacion de puntaje en Trap
};

template <>
struct GameRules<Difficulty::Easy> {
    int initialMaxEnergy{80};   // Energia inicial y maxima
    std::size_t turnLimit{240};
    int energyCostGeneral{1};   // Para Cells Empty, ResourceCell, Battery, Trap, Exit
    int energyCostRoughTerrain{2};  // Para Cell RoughTerrain
    int energyCostOthers{1};    // Para Action::wait e intento invalido
    int reward{15};     // Recompensa de ResourceCell
    int energy{5};      // Energia de Battery
    int energyPenalty{1};   // Penalizacion de energia en Trap
    int scorePenalty{0};    // Penalizacion de puntaje en Trap
};

template <>
struct GameRules<Difficulty::Hard> {
    int initialMaxEnergy{40};   // Energia inicial y maxima
    std::size_t turnLimit{140};
    int energyCostGeneral{1};   // Para Cells Empty, ResourceCell, Battery, Trap, Exit
    int energyCostRoughTerrain{3};  // Para Cell RoughTerrain
    int energyCostOthers{1};    // Para Action::wait e intento invalido
    int reward{8};     // Recompensa de ResourceCell
    int energy{2};      // Energia de Battery
    int energyPenalty{3};   // Penalizacion de energia en Trap
    int scorePenalty{2};    // Penalizacion de puntaje en Trap
};