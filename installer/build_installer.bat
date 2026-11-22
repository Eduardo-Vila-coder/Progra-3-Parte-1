@echo off
REM ====================================================================
REM Genera el instalador de Windows de Circuito de Escape (proyecto 2, 8.1)
REM Un solo instalador con los DOS ejecutables (escape_circuit y map_generator)
REM y los escenarios de assets\maps.
REM
REM Requisitos en la PC donde se GENERA (no en la de destino):
REM   - CLion (trae CMake, Ninja y MinGW) o CMake + un compilador C++20
REM   - NSIS 3 (https://nsis.sourceforge.io)
REM No hace falta configurar el PATH: si cmake o makensis no estan en el PATH,
REM el script los busca en las carpetas de instalacion por defecto de CLion y NSIS.
REM
REM Resultado: dist\CircuitoDeEscape-2.0.0-Windows.exe  (instalador)
REM            dist\CircuitoDeEscape-2.0.0-Windows.zip  (portable)
REM ====================================================================
setlocal EnableExtensions
cd /d "%~dp0.."

REM --- 1. Herramientas ---
where cmake >nul 2>nul
if errorlevel 1 call :find_clion
where makensis >nul 2>nul
if errorlevel 1 call :find_nsis

where cmake >nul 2>nul
if errorlevel 1 (
    echo ERROR: no se encontro CMake. Instale CLion ^(o CMake^) o agregue cmake al PATH.
    goto error
)
where makensis >nul 2>nul
if errorlevel 1 (
    echo ERROR: no se encontro NSIS. Instalelo desde https://nsis.sourceforge.io
    goto error
)

REM --- 2. Generador de CMake: Ninja si existe, si no MinGW Makefiles, si no el de por defecto.
REM     Si build-release ya existe, CMake reutiliza el generador con el que se creo.
set GENERATOR=
if not exist build-release\CMakeCache.txt (
    where ninja >nul 2>nul && set GENERATOR=-G Ninja
)
if not exist build-release\CMakeCache.txt if not defined GENERATOR (
    where mingw32-make >nul 2>nul && set GENERATOR=-G "MinGW Makefiles"
)

REM --- 3. Compilar en Release y empaquetar ---
cmake -S . -B build-release %GENERATOR% -DCMAKE_BUILD_TYPE=Release
if errorlevel 1 goto error

cmake --build build-release --config Release --target escape_circuit map_generator
if errorlevel 1 goto error

REM Las pruebas se ejecutan en Debug (en Release, NDEBUG desactiva los assert)

cpack --config build-release\CPackConfig.cmake -C Release -B dist
if errorlevel 1 goto error

echo.
echo Instalador generado en la carpeta dist\ (incluye escape_circuit y map_generator)
exit /b 0

:error
echo.
echo ERROR: no se pudo generar el instalador. Revisa los mensajes anteriores.
echo Si el error menciona el generador ^(Ninja / Makefiles^), borra la carpeta build-release y vuelve a intentar.
exit /b 1

REM Busca CLion en las carpetas por defecto y agrega su CMake, MinGW y Ninja al PATH de esta ejecucion
:find_clion
set CLION_BIN=
for /d %%D in ("%LOCALAPPDATA%\Programs\CLion*" "%ProgramFiles%\JetBrains\CLion*") do (
    if exist "%%~D\bin\cmake\win\x64\bin\cmake.exe" set "CLION_BIN=%%~D\bin"
)
if not defined CLION_BIN exit /b 0
echo Usando las herramientas de CLion: "%CLION_BIN%"
set "PATH=%CLION_BIN%\cmake\win\x64\bin;%CLION_BIN%\mingw\bin;%CLION_BIN%\ninja\win\x64;%PATH%"
exit /b 0

REM Busca NSIS en las carpetas por defecto y lo agrega al PATH de esta ejecucion
:find_nsis
if exist "%ProgramFiles(x86)%\NSIS\makensis.exe" set "PATH=%ProgramFiles(x86)%\NSIS;%PATH%"
if exist "%ProgramFiles%\NSIS\makensis.exe" set "PATH=%ProgramFiles%\NSIS;%PATH%"
exit /b 0
