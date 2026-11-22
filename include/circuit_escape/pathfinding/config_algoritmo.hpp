#pragma once
#include "logica_algoritmo.hpp"
#include "circuit_escape/overloaded.hpp"
#include <algorithm>
#include <exception>
#include <optional>
#include <string>
#include <variant>
#include <vector>

class Config_algoritmo
{
private:
    struct CasillaInf
    {
        int peso_;
        bool pisable;
        int recarga;
        std::size_t x; // fila
        std::size_t y; // columna

        std::size_t id(std::size_t columnas) const { return x * columnas + y; }
        Position posicion() const { return Position{x, y}; }
    };
    template <typename T1>
    class Constructor_nodos
    {
    private:
        T1 inicio_;
        std::size_t filas_, columnas_, turnos_;
        CasillaInf peso(const Cell &casilla_, std::size_t x, std::size_t y) const
        {
            bool facil = (turnos_ == 240);
            bool dificil = (turnos_ == 140);
            int recarga = facil ? 5 : (dificil ? 2 : 3);
            int elevado = dificil ? 3 : 2;
            int trampa = facil ? 2 : (dificil ? 4 : 3);
            return std::visit(Overloaded{[&](const Empty &)
                                         { return CasillaInf{1, true, 0, x, y}; },
                                         [&](const Wall &)
                                         { return CasillaInf{1, false, 0, x, y}; },
                                         [&](const RoughTerrain &)
                                         { return CasillaInf{elevado, true, 0, x, y}; },
                                         [&](const Battery &b)
                                         { return CasillaInf{1, true, b.consumed ? 0 : recarga, x, y}; }, 
                                         [&](const ResourceCell<int> &)
                                         { return CasillaInf{1, true, 0, x, y}; },
                                         [&](const Trap &)
                                         { return CasillaInf{trampa, true, 0, x, y}; },
                                         [&](const Exit &)
                                         { return CasillaInf{1, true, 0, x, y}; },
                                         [&](const Switch &)
                                         { return CasillaInf{1, true, 0, x, y}; }},
                              casilla_);
        }

    public:
        Constructor_nodos(T1 inicio, std::size_t filas, std::size_t columnas, std::size_t turnos)
            : inicio_(inicio), filas_(filas), columnas_(columnas), turnos_(turnos) {}

        std::optional<CasillaInf> construir(std::size_t x, std::size_t y) const
        {
            if (x >= filas_ || y >= columnas_)
                return std::nullopt;
            return peso(*(inicio_ + (x * columnas_ + y)), x, y);
        }
        std::size_t filas() const { return filas_; }
        std::size_t columnas() const { return columnas_; }
    };

public:
    Config_algoritmo() {}
    template <std::size_t Rows, std::size_t Columns>
    std::string calculo(const Grid<Cell, Rows, Columns> &grid, Position from, Position to, std::size_t turnos, int energia)
    {
        Constructor_nodos nodos(grid.begin(), Rows, Columns, turnos);
        const auto inicio = nodos.construir(from.row, from.column);
        const auto fin = nodos.construir(to.row, to.column);
        if (!inicio || !fin)
            return "Posicion fuera del tablero";
        Disjtrak algoritmo;
        std::vector<std::pair<int, int>> valores;           // camino que se debe seguir
        int costo = algoritmo.buscar(nodos,*inicio, *fin, valores); // vector para el armado
        if (costo < 0)
        {
            return "No se puede llegar a la salida";
        }
        if (costo >= energia)
        {
            return "La ruta mas corta tiene el costo de: " + std::to_string(costo) + ", pero no alcanza energia";
        }
        if (valores.size() < 2)
        {
            return "Ya estas en la salida";
        }
        std::string camino = "";
        for (std::size_t i = 1; i < valores.size() && i <= 3; ++i)
        {
            if (i > 1){
                camino += ", ";
            }
            camino += "(" + std::to_string(valores[i].first) + "," + std::to_string(valores[i].second) + ")";
        }

        return "El camino a seguir es : " + camino;
    }
    template <std::size_t Rows, std::size_t Columns>
    std::string validacion_mapas(const std::string& direccion){
        try
        {
            const auto escenario = loadScenario<Rows, Columns>(direccion);
            const auto &grid = escenario.grid;

            Position salida{};
            for (std::size_t f = 0; f < Rows; ++f)
                for (std::size_t c = 0; c < Columns; ++c)
                    if (std::holds_alternative<Exit>(grid.at({f, c})))
                        salida = Position{f, c};

            Constructor_nodos nodos{grid.begin(), Rows, Columns, 180};
            const auto inicio = nodos.construir(escenario.start.row, escenario.start.column);
            const auto fin = nodos.construir(salida.row, salida.column);

            Disjtrak algoritmo;
            std::vector<std::pair<int, int>> camino;
            if (algoritmo.buscar(nodos, *inicio, *fin, camino) < 0)
                return "Mapa sin solucion: la salida no es alcanzable";
            return "Mapa valido";
        }
        catch (const std::exception &e)
        {
            return std::string("Mapa invalido: ") + e.what();
        }
    }
};