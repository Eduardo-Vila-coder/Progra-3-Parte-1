#pragma once
#include "circuit_escape/cells.hpp"
#include "circuit_escape/grid.hpp"
#include "circuit_escape/overloaded.hpp"
#include "circuit_escape/position.hpp"
#include "circuit_escape/scenario.hpp"
#include <algorithm>
#include <exception>
#include <optional>
#include <string>
#include <variant>
#include <vector>
#include <queue>
#include <array>
#include <limits>
// comentario: disculpen el error ya me dio miedo cambiar el nombre
class Disjtrak 
{
public:
    // Devuelve el costo mínimo de inicio a fin, o -1 si no hay ruta.
    template <typename Nodos, typename Nodo>
    int buscar(const Nodos &nodos, const Nodo &inicio, const Nodo &fin,
               std::vector<std::pair<int, int>> &valores) const
    {
        const int INF = std::numeric_limits<int>::max() / 2;
        std::size_t columnas = nodos.columnas();
        std::size_t total = nodos.filas() * columnas;

        std::vector<int> dist(total, INF); // todos las distancias se inician en inf/2 por seguridad
        std::vector<int> prev(total, -1);  // todos los previos con -1 para marcar el final
        using Par = std::pair<int, std::size_t>;  // costo, id
        std::priority_queue<Par, std::vector<Par>, std::greater<Par>> cola; // guardaremos los pendientes | aprovecharemos el priority para sacar el de mmenor costo 
        // como priority saca el de mayor costo con greater lo invertimos respecto de Par que es pair por el int 
        std::size_t idInicio = inicio.id(columnas); // ubicacion en grid
        std::size_t idFin = fin.id(columnas); // ubicacion de a donde queremos llegar
        dist[idInicio] = 0; // inicializamos con 0 el inicial 
        cola.push(Par{0, idInicio}); // recordemos que solo se guarda Pares nada mas 

        while (!cola.empty())
        {
            // d es costo e i es ubicacion en grid
            const auto [d, i] = cola.top(); //? casilla mas barata con costo y id o posiciones como quiera llamarsele
            cola.pop(); // es seguro hacer pop ya que no estamos leyendo antes con referencia 
            if (d > dist[i]){// por si el costo es antiguo
                continue;
            }  
            if (i == idFin){ // por si llega a la salida 
                break;                         
            }
                
            std::size_t x = i / columnas; // posicion en una casilla
            std::size_t y = i % columnas; // posicion en una casilla
            std::array<std::optional<Nodo>, 4> vecinos{
                nodos.construir(x - 1, y), nodos.construir(x + 1, y),
                nodos.construir(x, y - 1), nodos.construir(x, y + 1)}; // lo que verificaremos 

            for (const auto &v : vecinos)
            {
                if (!v || !v->pisable) // validamos existencia y lo que es pisable
                    continue;                  
                std::size_t j = v->id(columnas);
                int nuevo = d + v->peso_; // ponemos costo para los vecinos
                if (nuevo < dist[j]) // si el nuevo peso es menor que el que habia actualizamos
                {
                    dist[j] = nuevo;
                    prev[j] = static_cast<int>(i);
                    cola.push({nuevo, j});
                }
            }
        }

        valores.clear(); // limpiamos valores 
        if (dist[idFin] >= INF){ // por si no encontramos
            return -1;
        }

        const int ancho = static_cast<int>(columnas);
        int actual = static_cast<int>(idFin);
        while (actual != -1) // hasta llegar a inicio
        {
        valores.push_back({actual / ancho, actual % ancho});
        actual = prev[actual];
        }
        std::reverse(valores.begin(), valores.end()); // invertimos por el orden
        return dist[idFin];
    }
};