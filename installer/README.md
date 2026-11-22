# Instalador de Circuito de Escape (Windows)

Es **un solo instalador** que distribuye **los dos ejecutables** del proyecto y los escenarios
(enunciado, sección 8.1: *"Incluir map_generator y escape_circuit en el mismo instalador"*):

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

Requisitos (solo en la PC donde se genera):

1. **CLion** (trae CMake, Ninja y el compilador MinGW con C++20), o CMake + un compilador C++20.
2. **NSIS 3**: descargar `nsis-3.xx-setup.exe` desde <https://nsis.sourceforge.io> e instalarlo con
   las opciones por defecto.

**No hace falta configurar el `PATH`.** Si `cmake` o `makensis` no están en el `PATH`, el script los
busca en las carpetas de instalación por defecto:

- CLion: `%LOCALAPPDATA%\Programs\CLion*` y `C:\Program Files\JetBrains\CLion*`
  (usa su `cmake`, `ninja` y `mingw`).
- NSIS: `C:\Program Files (x86)\NSIS` y `C:\Program Files\NSIS`.

Desde la raíz del repositorio, en PowerShell o en la terminal de CLion:

```powershell
.\installer\build_installer.bat
```

(En `cmd.exe` se escribe `installer\build_installer.bat`.) La primera vez tarda unos minutos porque
compila todo en Release y descarga FTXUI.

El script compila `escape_circuit` y `map_generator` en Release y deja en `dist\` (las pruebas se
ejecutan antes en Debug, desde CLion o con `ctest`):

- `CircuitoDeEscape-2.0.0-Windows.exe`: instalador con asistente.
- `CircuitoDeEscape-2.0.0-Windows.zip`: versión portable.

Equivalente manual (con `cmake`, `ninja`, el compilador y `makensis` en el `PATH`; si CLion está en
otra carpeta, agregar al `PATH` sus subcarpetas `bin\cmake\win\x64\bin`, `bin\mingw\bin` y
`bin\ninja\win\x64`):

```bat
cmake -S . -B build-release -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build-release --config Release --target escape_circuit map_generator
cpack --config build-release\CPackConfig.cmake -C Release -B dist
```

Si aparece un error sobre el generador (*"Ninja"* o *"Makefiles"* no coincide), borrar la carpeta
`build-release` y volver a ejecutar el script.

## 2. Instalar (en la PC de destino)

1. Ejecutar `CircuitoDeEscape-2.0.0-Windows.exe` y seguir el asistente
   (carpeta por defecto: `C:\Program Files\Circuito de Escape`).
2. Opcional: marcar "Add to PATH" para usar `escape_circuit` desde cualquier terminal.

## 3. Ejecutar

- Juego: menú Inicio → **Circuito de Escape**, o desde una terminal:

```powershell
& "C:\Program Files\Circuito de Escape\bin\escape_circuit.exe" --level facil
```

- Generador de mapas: desde una terminal, con una carpeta de salida en la que el usuario pueda
  escribir (Windows no permite escribir en `C:\Program Files` sin permisos de administrador):

```powershell
& "C:\Program Files\Circuito de Escape\bin\map_generator.exe" --seed 42 --workers 4 --output "$env:USERPROFILE\Documents\mapas"
```

Con la semilla 42 genera los mismos tres niveles que vienen instalados.

## 4. Desinstalar

- Configuración → Aplicaciones → **Circuito de Escape** → Desinstalar, o
- ejecutar `Uninstall.exe` dentro de la carpeta de instalación.

Se eliminan los programas, los mapas y los accesos directos.
