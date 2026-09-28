#include <optional>
#include<iostream>
struct Position
{
    std::size_t row{};
    std::size_t column{};
    friend bool operator==(const Position &, const Position &) = default;
    //! completo debido a metodo del propio c++ mediante el =default que genera la comparacion
};

//? los valores pertenecen al enum class por lo que se debe accder mediante ::
//? Por defecto usan int  y no existe una conversion implicita
//? Tienen valores en size_t el enum
enum class Action
{
    up,    // row-1
    down,  // row+1
    left,  // column-1
    right, // column+1
    wait   // cumple la funcion de ser una accion
};

//* se calcula la posicion adyacente a partir de un puntoal tratar de realizar una accion de movimiento
// usos de std::nullopt // sirve para indicar que se llego a un valor invalido en este caso
std::optional<Position> neighbor(Position origin, Action action)
{
    //! segun lo revisado se puede hacer mediante switch para una comprobacion directa y ademas que ofrece advertencias del compilador
    switch (action)
    {
    case Action::up:
        if (origin.row>= 1)return{Position{origin.row - 1, origin.column}}; //* retorno mediante constructor del enum 
        break;
    case Action::down:
        return Position{origin.row + 1, origin.column};
    case Action::left:
        if (origin.column>= 1)return{Position{origin.row, origin.column-1}};
        break;
    case Action::right:
        return Position{origin.row, origin.column+1};
    case Action::wait:
        return origin;
    }
    return std::nullopt;
}
std::string toString(Position position)
{
    //? uso del std::to_string para la conversion de valores numericos a string
    return "("+ std::to_string(position.row)+ "," + std::to_string(position.column)+")" ;
}