@echo off
REM ====================================================================
REM Genera el instalador de Windows de Circuito de Escape (proyecto 2, 8.1)
REM Requisitos en la PC donde se GENERA (no en la de destino):
REM   - CMake y un compilador C++20 en el PATH (por ejemplo, el MinGW de CLion)
REM   - NSIS 3 (https://nsis.sourceforge.io) con makensis en el PATH
REM Resultado: dist\CircuitoDeEscape-2.0.0-Windows.exe  (instalador)
REM            dist\CircuitoDeEscape-2.0.0-Windows.zip  (portable)
REM ====================================================================
setlocal
cd /d "%~dp0.."

REM Si Ninja esta disponible (viene con CLion) se usa; si no, el generador por defecto
set GENERATOR=
where ninja >nul 2>nul && set GENERATOR=-G Ninja

cmake -S . -B build-release %GENERATOR% -DCMAKE_BUILD_TYPE=Release
if errorlevel 1 goto error

cmake --build build-release --config Release
if errorlevel 1 goto error

REM Las pruebas se ejecutan en Debug (en Release, NDEBUG desactiva los assert)

cpack --config build-release\CPackConfig.cmake -C Release -B dist
if errorlevel 1 goto error

echo.
echo Instalador generado en la carpeta dist\
exit /b 0

:error
echo.
echo ERROR: no se pudo generar el instalador. Revisa los mensajes anteriores.
exit /b 1
