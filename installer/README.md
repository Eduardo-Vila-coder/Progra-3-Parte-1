# Instalador de Circuito de Escape (Windows)

El instalador distribuye **los dos ejecutables** del proyecto y los escenarios:

| Se instala en | Contenido |
|---|---|
| `bin\escape_circuit.exe` | El juego |
| `bin\map_generator.exe` | El generador de mapas (`MapBatchRunner`: Kruskal + Dijkstra + clasificación) |
| `assets\maps\` | Escenarios `.txt` que carga el juego, incluidos `mapa_facil/medio/dificil.txt` |
| `README.md` | Instrucciones del proyecto |

En Release los programas se enlazan de forma **estática** (MinGW) y FTXUI es una biblioteca
estática, así que la PC de destino **no necesita compilar ni instalar nada más**. En Debug
se usa enlazado dinámico para poder compilar y probar con el MinGW de CLion.
El juego busca los mapas en `bin\..\assets\maps`, por eso funciona desde cualquier carpeta.

## 1. Generar el instalador (en la PC de desarrollo)

Requisitos: CMake, un compilador C++20 (el MinGW de CLion sirve) y **NSIS 3**
(<https://nsis.sourceforge.io>), todos en el `PATH`.

```bat
installer\build_installer.bat
```

El script compila en Release y deja en `dist\` (las pruebas se ejecutan antes en Debug, desde CLion o con `ctest`):

- `CircuitoDeEscape-2.0.0-Windows.exe`: instalador con asistente.
- `CircuitoDeEscape-2.0.0-Windows.zip`: versión portable.

Equivalente manual:

```bat
cmake -S . -B build-release -DCMAKE_BUILD_TYPE=Release
cmake --build build-release --config Release
cpack --config build-release\CPackConfig.cmake -C Release -B dist
```

## 2. Instalar (en la PC de destino)

1. Ejecutar `CircuitoDeEscape-2.0.0-Windows.exe` y seguir el asistente
   (carpeta por defecto: `C:\Program Files\Circuito de Escape`).
2. Opcional: marcar "Add to PATH" para usar `escape_circuit` desde cualquier terminal.

## 3. Ejecutar

- Menú Inicio → **Circuito de Escape**, o desde una terminal:

```bat
"C:\Program Files\Circuito de Escape\bin\escape_circuit.exe" --scenario 2
```

## 4. Desinstalar

- Configuración → Aplicaciones → **Circuito de Escape** → Desinstalar, o
- ejecutar `Uninstall.exe` dentro de la carpeta de instalación.

Se eliminan los programas, los mapas y los accesos directos.
