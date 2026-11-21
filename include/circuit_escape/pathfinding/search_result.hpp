#pragma once
#include <cstddef>
#include <vector>

// --- RESULTADO DE BÚSQUEDA (proyecto 2, sección 5) ---
// Lo devuelven BFS (vigilante) y Dijkstra (validación y ayuda H).
// Las casillas se identifican con su número (fila * columnas + columna), igual que en MapGraph.
struct SearchResult
{
    bool found{false};                 // true si existe un camino del origen al destino
    std::vector<std::size_t> path;     // casillas del camino, desde el origen hasta el destino (incluidos)
    int cost{0};                       // BFS: cantidad de movimientos; Dijkstra: energía total
    std::vector<std::size_t> examined; // casillas examinadas, en el orden en que se sacaron de la cola
};
