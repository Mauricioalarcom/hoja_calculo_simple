cmake_minimum_required(VERSION 3.31)
project(project_aed)

# Estándar de C++
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 1. RUTA ESPECÍFICA PARA SFML 2 EN ARCH/CACHYOS
# El paquete 'sfml2' instala su configuración en esta carpeta para no chocar con la v3
set(SFML_DIR "/usr/lib/cmake/SFML2")

# 2. BUSCAR SFML
find_package(SFML 2.5 COMPONENTS system window graphics audio REQUIRED)

if(SFML_FOUND)
    message(STATUS "SFML 2 detectada correctamente: ${SFML_VERSION}")
else()
    message(FATAL_ERROR "SFML 2 no encontrada. Asegúrate de haber ejecutado: sudo pacman -S sfml2")
endif()

# 3. DIRECTORIOS DE INCLUSIÓN
include_directories(${CMAKE_SOURCE_DIR}/include)

# 4. ARCHIVOS FUENTE (Usando tu lógica de GLOB pero limpia)
file(GLOB SOURCE_FILES
        "src/*.cpp"
        "include/*.h"
)

# 5. CREAR EJECUTABLE
add_executable(${PROJECT_NAME} ${SOURCE_FILES})

# 6. ENLAZAR LIBRERÍAS (En Linux es directo)
target_link_libraries(${PROJECT_NAME}
        sfml-graphics
        sfml-window
        sfml-system
        sfml-audio
)

# 7. COPIAR RECURSOS (arial.ttf)
if(EXISTS "${CMAKE_SOURCE_DIR}/arial.ttf")
    configure_file("${CMAKE_SOURCE_DIR}/arial.ttf"
            "${CMAKE_BINARY_DIR}/arial.ttf"
            COPYONLY)
    message(STATUS "Recurso arial.ttf copiado al directorio de salida.")
endif()