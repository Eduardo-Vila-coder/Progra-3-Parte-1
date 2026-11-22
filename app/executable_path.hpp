#pragma once
// Ruta del ejecutable en curso. La usan escape_circuit y map_generator para ubicar la carpeta
// assets/maps instalada junto al programa (proyecto 2, sección 8.1).
#include <filesystem>
#include <system_error>

#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#endif

// En Windows se usa GetModuleFileNameW (funciona aunque el programa se abra desde el menú Inicio
// o desde el PATH); en otros sistemas, argv[0] convertido en ruta absoluta.
inline std::filesystem::path executablePath(const char* argv0)
{
#ifdef _WIN32
    wchar_t buffer[MAX_PATH];
    const DWORD length = GetModuleFileNameW(nullptr, buffer, MAX_PATH);
    if (length > 0 && length < MAX_PATH) {
        return std::filesystem::path(buffer);
    }
#endif
    std::error_code error;
    const std::filesystem::path path = std::filesystem::weakly_canonical(argv0, error);
    return error ? std::filesystem::path(argv0) : path;
}
