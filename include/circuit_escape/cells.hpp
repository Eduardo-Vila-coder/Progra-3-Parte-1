#pragma once
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

//* Celltraits es como una configuracion del juego 
//? el static hace que se asocie al tipo y el constexpr hace que sea constante 
template <typename Cell>
struct CellTraits
{ 
    static constexpr bool traversable = true;
    static constexpr int energyCost = 1;
};
template <>
struct CellTraits<Wall>
{
    static constexpr bool traversable = false;
    static constexpr int energyCost = 0;
};
template <typename Reward>
struct CellTraits<ResourceCell<Reward>>
{
    static constexpr bool traversable = true;
    static constexpr int energyCost = 1;
};