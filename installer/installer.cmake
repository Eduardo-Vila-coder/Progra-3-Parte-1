# =====================================================================
# Instalador de Circuito de Escape (proyecto 2, sección 8.1)
# Instala el juego (escape_circuit), el generador de mapas (map_generator,
# cuando exista) y los escenarios de assets/maps. En Windows genera un
# instalador NSIS (.exe) con desinstalador y un .zip portable.
# Uso: installer/build_installer.bat (ver installer/README.md)
# =====================================================================

# Programas que distribuye el instalador
set(CIRCUIT_ESCAPE_PROGRAMS escape_circuit)
if(TARGET map_generator)
    list(APPEND CIRCUIT_ESCAPE_PROGRAMS map_generator)
endif()

# Con MinGW (el compilador de CLion) se enlaza todo de forma estática: el .exe no necesita
# libstdc++-6.dll, libgcc_s_seh-1.dll ni libwinpthread-1.dll en la PC de destino.
# FTXUI ya se compila como biblioteca estática.
if(MINGW)
    foreach(program IN LISTS CIRCUIT_ESCAPE_PROGRAMS)
        # El enlazado totalmente estático es necesario para el instalador (Release),
        # pero este MinGW no lo resuelve en Debug. Las pruebas y el desarrollo usan
        # enlazado dinámico con el directorio del compilador en PATH.
        foreach(option IN ITEMS -static -static-libgcc -static-libstdc++)
            target_link_options(${program} PRIVATE "$<$<CONFIG:Release>:${option}>")
        endforeach()
    endforeach()
endif()

# Con Visual Studio (MSVC) se copian las DLL del runtime de C++ junto a los programas
set(CMAKE_INSTALL_SYSTEM_RUNTIME_DESTINATION bin)
include(InstallRequiredSystemLibraries)

# Estructura instalada:
#   <carpeta de instalación>/bin/escape_circuit.exe
#   <carpeta de instalación>/bin/map_generator.exe
#   <carpeta de instalación>/assets/maps/*.txt   (el juego los busca en bin/../assets/maps)
#   <carpeta de instalación>/README.md
install(TARGETS ${CIRCUIT_ESCAPE_PROGRAMS} RUNTIME DESTINATION bin)
install(DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/assets/maps/" DESTINATION assets/maps)
install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/README.md" DESTINATION .)

# ----- CPack: datos del paquete -----
set(CPACK_PACKAGE_NAME "CircuitoDeEscape")
set(CPACK_PACKAGE_VENDOR "CS2013 Programacion III - UTEC")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Circuito de Escape: juego de consola y generador de mapas")
set(CPACK_PACKAGE_VERSION "2.0.0")
set(CPACK_PACKAGE_VERSION_MAJOR "2")
set(CPACK_PACKAGE_VERSION_MINOR "0")
set(CPACK_PACKAGE_VERSION_PATCH "0")
set(CPACK_PACKAGE_INSTALL_DIRECTORY "Circuito de Escape")
set(CPACK_PACKAGE_FILE_NAME "CircuitoDeEscape-${CPACK_PACKAGE_VERSION}-${CMAKE_SYSTEM_NAME}")
set(CPACK_RESOURCE_FILE_README "${CMAKE_CURRENT_SOURCE_DIR}/README.md")

# Accesos directos del menú Inicio: ejecutable y nombre visible
set(CPACK_PACKAGE_EXECUTABLES "escape_circuit" "Circuito de Escape")
if(TARGET map_generator)
    list(APPEND CPACK_PACKAGE_EXECUTABLES "map_generator" "Generador de mapas")
endif()

if(WIN32)
    # NSIS: instalador .exe con asistente, accesos directos y desinstalador
    # (aparece en "Agregar o quitar programas"). ZIP: versión portable.
    set(CPACK_GENERATOR "NSIS;ZIP")
    set(CPACK_NSIS_PACKAGE_NAME "Circuito de Escape")
    set(CPACK_NSIS_DISPLAY_NAME "Circuito de Escape")
    set(CPACK_NSIS_ENABLE_UNINSTALL_BEFORE_INSTALL ON) # desinstala una versión previa antes de instalar
    set(CPACK_NSIS_MODIFY_PATH ON)                     # ofrece agregar bin al PATH para usarlo desde la terminal
    set(CPACK_NSIS_INSTALLED_ICON_NAME "bin\\\\escape_circuit.exe")
    # El generador instalado reescribe los niveles en assets\maps (los que carga el juego).
    # C:\Program Files solo lo puede modificar un administrador, así que el instalador da permiso
    # de modificación sobre esa carpeta al grupo Usuarios (SID S-1-5-32-545) con icacls.
    set(CPACK_NSIS_EXTRA_INSTALL_COMMANDS
        "nsExec::Exec 'icacls \\\"$INSTDIR\\\\assets\\\\maps\\\" /grant *S-1-5-32-545:(OI)(CI)M /T'")
else()
    set(CPACK_GENERATOR "TGZ")
endif()

include(CPack)
