#pragma once
#include <type_traits>
#include <variant>
struct Empty // espacio libre 
{};
struct Wall // muro 
{};
struct RoughTerrain // terreno elevado por ende el costo
{
    int energyCost{2};
};
template <typename Reward>
struct ResourceCell
{   //? respecto al costo que se indica supongo que habra una parte dedicada a eso para que sea modular 
    Reward reward; // como dice su nombre es la recompensa 
    bool collected{false}; // indicando que es de un solo uso 
};
struct Battery
{
    int energy{3}; // energia que da supongo 
    bool consumed{false}; // indicaria que es de un solo uso 
};
struct Trap
{
    int energyPenalty{2}; // se resta energia
    int scorePenalty{1};  // se agrega punto 
};
struct Exit // salida
{
};
// std::variant ayudara a almacenar valores que son de varios tipos de datos 
using Cell = std::variant<Empty, Wall, RoughTerrain, ResourceCell<int>, Battery, Trap, Exit>;
/*
especializaciones
*/

//* Celltraits describe cómo se comporta cada tipo de celda (enunciado 6.3)
//? el static hace que se asocie al tipo y el constexpr hace que sea constante
//  - traversable: si el agente puede entrar a la celda
//  - consumable:  si la celda se usa una sola vez (después se comporta como espacio libre)
//  - spent(c):    si esa celda consumible ya fue usada
// Los costos y recompensas NO están aquí: son datos de GameRules (perfiles de dificultad).

// Plantilla general: celdas permanentes y transitables (Empty, RoughTerrain, Trap, Exit)
template <typename CellType>
struct CellTraits
{
    static constexpr bool traversable = true;
    static constexpr bool consumable = false;
    static constexpr bool spent(const CellType &) noexcept { return false; }
};

// Especialización TOTAL: el muro es el único tipo que bloquea el paso
template <>
struct CellTraits<Wall>
{
    static constexpr bool traversable = false;
    static constexpr bool consumable = false;
    static constexpr bool spent(const Wall &) noexcept { return false; }
};

// Especialización TOTAL: la batería se consume la primera vez que se entra
template <>
struct CellTraits<Battery>
{
    static constexpr bool traversable = true;
    static constexpr bool consumable = true;
    static constexpr bool spent(const Battery &battery) noexcept { return battery.consumed; }
};

// Especialización PARCIAL: toda la familia de recursos (cualquier tipo de recompensa)
// es consumible y se marca como recogida con el campo collected
template <typename Reward>
struct CellTraits<ResourceCell<Reward>>
{
    static constexpr bool traversable = true;
    static constexpr bool consumable = true;
    static constexpr bool spent(const ResourceCell<Reward> &resource) noexcept { return resource.collected; }
};

// Consultas sobre una Cell (variant): std::visit obtiene el tipo concreto y se consulta su rasgo
inline bool isTraversable(const Cell &cell)
{
    return std::visit([](const auto &concrete)
                      { return CellTraits<std::decay_t<decltype(concrete)>>::traversable; },
                      cell);
}

// Un consumible ya usado (recurso recogido o batería consumida) se comporta como espacio libre
inline bool isSpent(const Cell &cell)
{
    return std::visit([](const auto &concrete)
                      { return CellTraits<std::decay_t<decltype(concrete)>>::spent(concrete); },
                      cell);
}