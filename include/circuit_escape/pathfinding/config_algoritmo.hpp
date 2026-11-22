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
    
    struct AyudaRuta
    {
        enum class Estado { Alcanza, LlegaConCero, Insuficiente, SinCamino };

        bool conInterruptores{false};
        bool activoA{true};
        bool activoB{true};
        std::optional<int> gastoA;               // desde la posición actual; solo si A está pendiente y hay camino
        std::optional<int> gastoB;
        std::optional<int> gastoSalida;          // solo cuando la salida está habilitada
        std::vector<std::string> recorrido;      // por ejemplo {"B", "A", "salida"}
        int total{0};
        int energia{0};
        Estado estado{Estado::SinCamino};
        std::string inaccesible;                 // destino sin camino, si estado == SinCamino
        std::vector<std::pair<int, int>> camino; // recorrido completo (por si luego se quiere resaltar)
    };

    // Compara los recorridos posición actual -> (A y B pendientes, en ambos órdenes) -> salida.
    // Los interruptores ya activados se omiten; si ambos están activos va directo a la salida.
    template <std::size_t Rows, std::size_t Columns>
    AyudaRuta ayuda(const Grid<Cell, Rows, Columns> &grid, Position from, int energia, std::size_t turnos) const
    {
        AyudaRuta r;
        r.energia = energia;

        // 1) ubicar la salida y los interruptores en el mapa actual (el grid vivo guarda 'active')
        std::optional<Position> salida, posA, posB;
        bool activoA = false, activoB = false;
        for (std::size_t f = 0; f < Rows; ++f)
            for (std::size_t c = 0; c < Columns; ++c)
            {
                const Cell &celda = grid.at({f, c});
                if (std::holds_alternative<Exit>(celda))
                    salida = Position{f, c};
                else if (const auto *s = std::get_if<Switch>(&celda))
                {
                    if (s->id == SwitchId::A) { posA = Position{f, c}; activoA = s->active; }
                    else                      { posB = Position{f, c}; activoB = s->active; }
                }
            }
        if (!salida)
        {
            r.inaccesible = "Salida";
            return r;
        }
        r.conInterruptores = posA.has_value() && posB.has_value();
        r.activoA = !r.conInterruptores || activoA;
        r.activoB = !r.conInterruptores || activoB;

        // 2) un tramo = una ejecución de Dijkstra entre dos casillas
        Constructor_nodos nodos{grid.begin(), Rows, Columns, turnos};
        Disjtrak dijkstra;
        struct Tramo
        {
            int costo{0};
            std::vector<std::pair<int, int>> camino;
        };
        auto tramo = [&](Position a, Position b) -> std::optional<Tramo>
        {
            const auto na = nodos.construir(a.row, a.column);
            const auto nb = nodos.construir(b.row, b.column);
            if (!na || !nb)
                return std::nullopt;
            Tramo t;
            t.costo = dijkstra.buscar(nodos, *na, *nb, t.camino);
            if (t.costo < 0)
                return std::nullopt;
            return t;
        };

        // 3) gastos por destino desde la posición actual (la tabla)
        struct Objetivo
        {
            std::string nombre;
            Position pos;
        };
        std::vector<Objetivo> pendientes;
        if (!r.activoA) pendientes.push_back({"A", *posA});
        if (!r.activoB) pendientes.push_back({"B", *posB});
        for (const Objetivo &o : pendientes)
        {
            const auto t = tramo(from, o.pos);
            (o.nombre == "A" ? r.gastoA : r.gastoB) = t ? std::optional<int>{t->costo} : std::nullopt;
        }
        if (pendientes.empty())
        {
            if (const auto t = tramo(from, *salida))
                r.gastoSalida = t->costo;
        }

        // 4) recorridos completos: ambos órdenes si hay dos pendientes (A primero en caso de empate)
        std::vector<std::vector<Objetivo>> ordenes;
        if (pendientes.size() == 2)
            ordenes = {{pendientes[0], pendientes[1]}, {pendientes[1], pendientes[0]}};
        else
            ordenes = {pendientes};

        std::optional<int> mejor;
        for (const auto &orden : ordenes)
        {
            std::vector<Position> destinos;
            std::vector<std::string> nombres;
            for (const Objetivo &o : orden)
            {
                destinos.push_back(o.pos);
                nombres.push_back(o.nombre);
            }
            destinos.push_back(*salida);
            nombres.push_back("salida");

            Position actual = from;
            int suma = 0;
            bool valido = true;
            std::vector<std::pair<int, int>> completo;
            for (const Position &d : destinos)
            {
                const auto t = tramo(actual, d);
                if (!t) { valido = false; break; }
                suma += t->costo;
                completo.insert(completo.end(), completo.empty() ? t->camino.begin() : t->camino.begin() + 1, t->camino.end());
                actual = d;
            }
            if (valido && (!mejor || suma < *mejor))
            {
                mejor = suma;
                r.recorrido = nombres;
                r.camino = completo;
            }
        }

        // 5) diagnóstico de energía (sin recargas): mayor / igual / menor
        if (!mejor)
        {
            r.estado = AyudaRuta::Estado::SinCamino;
            if (!r.activoA && !r.gastoA)      r.inaccesible = "A";
            else if (!r.activoB && !r.gastoB) r.inaccesible = "B";
            else                              r.inaccesible = "Salida";
            return r;
        }
        r.total = *mejor;
        if (energia > r.total)       r.estado = AyudaRuta::Estado::Alcanza;
        else if (energia == r.total) r.estado = AyudaRuta::Estado::LlegaConCero;
        else                         r.estado = AyudaRuta::Estado::Insuficiente;
        return r;
    }

    // Texto de la pantalla H (una cadena por línea). Solo formatea; no calcula nada.
    static std::vector<std::string> lineasAyuda(const AyudaRuta &r)
    {
        auto col = [](std::string s, std::size_t ancho)
        {
            if (s.size() < ancho) s.append(ancho - s.size(), ' ');
            return s;
        };
        std::vector<std::string> out;
        out.push_back(col("Objetivo", 10) + col("Estado", 18) + "Gasto desde aquí");

        auto gasto = [](const std::optional<int> &g) { return g ? std::to_string(*g) : std::string("inaccesible"); };
        if (r.conInterruptores)
        {
            out.push_back(col("A", 10) + col(r.activoA ? "Activado" : "Pendiente", 18) + (r.activoA ? "—" : gasto(r.gastoA)));
            out.push_back(col("B", 10) + col(r.activoB ? "Activado" : "Pendiente", 18) + (r.activoB ? "—" : gasto(r.gastoB)));
        }
        const bool habilitada = r.activoA && r.activoB;
        std::string requiere = "Requiere A y B";
        if (r.activoA) requiere = "Requiere B";
        else if (r.activoB) requiere = "Requiere A";
        out.push_back(col("Salida", 10) + col(habilitada ? "Habilitada" : requiere, 18) +
                      (habilitada ? gasto(r.gastoSalida) : std::string("—")));
        out.push_back("");

        if (r.estado == AyudaRuta::Estado::SinCamino)
        {
            out.push_back("Destino inaccesible: " + r.inaccesible);
            return out;
        }
        std::string via = "posición actual";
        for (const std::string &n : r.recorrido) via += " → " + n;
        out.push_back("Recorrido: " + via);
        const int saldo = r.energia - r.total;
        out.push_back("Energía disponible: " + std::to_string(r.energia) + "   Gasto total: " + std::to_string(r.total) +
                      (saldo > 0 ? "   Quedarían: " + std::to_string(saldo)
                                 : saldo == 0 ? "   Quedarían: 0" : "   Faltan: " + std::to_string(-saldo)));
        switch (r.estado)
        {
        case AyudaRuta::Estado::Alcanza:      out.push_back("Estado: ALCANZA PARA ESTE RECORRIDO SIN VIGILANTE"); break;
        case AyudaRuta::Estado::LlegaConCero: out.push_back("Estado: LLEGAS CON ENERGIA 0 — NO PERMITE GANAR"); break;
        default:                              out.push_back("Estado: NO ALCANZA SIN RECARGAS"); break;
        }
        out.push_back("No anticipa al vigilante ni las recargas de baterías.");
        return out;
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