cmake_minimum_required(VERSION 3.26)
project(lab4)

set(CMAKE_CXX_STANDARD 20)


set(SDL2_INCLUDE_DIR C:/Users/caste/OneDrive/Documentos/SDL2-2.28.1/include)
set(SDL2_LIB_DIR C:/Users/caste/OneDrive/Documentos/SDL2-2.28.1/lib/x64)

include_directories(${SDL2_INCLUDE_DIR})
link_directories(${SDL2_LIB_DIR})

# Agrega los archivos fuente al ejecutable
add_executable(lab4 main.cpp ObjLoader.cpp)

target_link_libraries(${PROJECT_NAME} SDL2main SDL2)

