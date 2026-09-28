#include<iostream>
template <typename CellType, size_t Rows, size_t Columns>
class Grid
{
public:
    using value_type = CellType;
    using iterator = std::array<CellType, Rows * Columns>::const_iterator; //? iterador para hacer cambios
    using const_iterator = std::array<CellType, Rows * Columns>::const_iterator; //? iterador para la lectura de los elememntos
    static constexpr size_t rows() noexcept { return Rows; }
    static constexpr size_t columns() noexcept { return Columns; }
    
    //* se verifica si una posicion este en un lugar valido asi cubriendo los dos bordes faltantes
    //? nodiscard: ayuda a que no se ignore el valor de retorno // constexpr : evaluacion en tiemp ode compilacion // noexcept : para garantizar que no lanzara una excepcion 
    [[nodiscard]] constexpr bool contains(Position position) const noexcept{
        return position.row< Rows && position.columns<Columns;
    }

    //* se encarga del acceso  
    CellType &at(Position position){
        if(contains(position))return cells_[position.row*Columns+position.column];
        throw std::out_of_range("Rango invalido");
    }
    const CellType &at(Position position) const{
        if(contains(position))return cells_[position.row*Columns+position.column];
        throw std::out_of_range("Rango invalido");
    }
    //? conjunto de escritura
    iterator begin() noexcept{return cells_.begin();}
    iterator end() noexcept{return cells_.end();}
    //? conjunto de lectura implicito // para cuando fue pasado como constante el objeto 
    const_iterator begin() const noexcept{return cells_.begin();}
    const_iterator end() const noexcept{return cells_.end();}
    //? conjunto de lectura explicito
    const_iterator cbegin() const noexcept{return cells_.cbegin();}
    const_iterator cend() const noexcept{return cells_.cend();}
private:
    std::array<CellType, Rows * Columns> cells_{};
};