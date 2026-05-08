cmake_minimum_required(VERSION 3.31)
project(project_aed)

set(CMAKE_CXX_STANDARD 17)
set(SFML_DIR "/opt/homebrew/opt/sfml@2/lib/cmake/SFML")

find_package(SFML 2.5.1 COMPONENTS system window graphics audio REQUIRED)

if(SFML_FOUND)
    message("SFML found: ${SFML_VERSION}")
else()
    message(FATAL_ERROR "SFML not found")

endif ()

include_directories(./include/)
file(GLOB SOURCE_FILES
        src/SparseMatrix.cpp
        src/*.cpp
        include/*.h
)

add_executable(${PROJECT_NAME} ${SOURCE_FILES}
        include/SparseMatrix.h
        src/main.cpp
        include/GUI.h
        src/GUI.cpp
)

if (APPLE OR UNIX)
    message("Acces Granted for Apple or Unix")

    target_link_libraries(${PROJECT_NAME}  sfml-system sfml-window sfml-graphics sfml-audio)

elseif(WIN32)
    message("Acces Granted for Windows")
    set(SFML_ROOT "C:/SFML/SFML-2.5.1/64bit")
    set(SFML_DIR "${SFML_ROOT}/lib/cmake/SFML")
    set(SFML_STATIC_LIBRARIES TRUE)


    include_directories(C:/SFML/include)
    target_link_libraries(${PROJECT_NAME} sfml-system sfml-window sfml-graphics -static sfml-audio)

else ()
    message("Acces denegated")
endif()

configure_file(${CMAKE_SOURCE_DIR}/arial.ttf
        ${CMAKE_BINARY_DIR}/arial.ttf COPYONLY)
