cmake_minimum_required(VERSION 3.24)

# A lib tensor fica ao lado deste projeto (~/projects/tensor). Para outro lugar: -DTENSOR_DIR=/caminho/da/tensor
set(TENSOR_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../tensor" CACHE PATH "Pasta da lib tensor")

# Mesma opção da lib tensor: TENSOR_EMULATE=ON compila sem CUDA, emulando a GPU na CPU.
option(TENSOR_EMULATE "Emular o CUDA na CPU (sem nvcc nem GPU)" OFF)
option(CSV_BUILD_TESTS "Compilar os testes" ON)

if(TENSOR_EMULATE)
    project(csv2tensor LANGUAGES CXX)
else()
    # Padrão: a GPU desta máquina (ex.: RTX 4070 Ti -> 89). Para outra: -DCMAKE_CUDA_ARCHITECTURES=89
    if(NOT DEFINED CMAKE_CUDA_ARCHITECTURES)
        set(CMAKE_CUDA_ARCHITECTURES native)
    endif()
    # A lib tensor tem objetos CUDA: o CUDA precisa estar ligado aqui também, para o link dos executáveis.
    project(csv2tensor LANGUAGES CXX CUDA)
    find_package(CUDAToolkit REQUIRED)
    set(CMAKE_CUDA_STANDARD 17)
    set(CMAKE_CUDA_STANDARD_REQUIRED ON)
endif()

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# MSVC: os fontes têm acentos em UTF-8.
if(MSVC)
    add_compile_options($<$<COMPILE_LANGUAGE:CXX>:/utf-8> $<$<COMPILE_LANGUAGE:CUDA>:-Xcompiler=/utf-8>)
endif()

# Só a biblioteca da tensor; os testes dela rodam no projeto dela.
set(TENSOR_BUILD_TESTS OFF)
add_subdirectory(${TENSOR_DIR} ${CMAKE_CURRENT_BINARY_DIR}/tensor)

add_library(csv2tensor
    src/status.cpp
    src/table.cpp
    src/schema.cpp
    src/encode.cpp
    src/device.cpp
)
target_include_directories(csv2tensor
    PUBLIC  ${CMAKE_CURRENT_SOURCE_DIR}/include
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)          # number.hpp
target_link_libraries(csv2tensor PUBLIC tensor)

if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(csv2tensor PRIVATE -Wall -Wextra)
endif()

# Ferramenta de linha de comando: lê um CSV e mostra o schema e o TENSOR3.
add_executable(csv_inspect tools/csv_inspect.cpp)
target_link_libraries(csv_inspect PRIVATE csv2tensor)

if(CSV_BUILD_TESTS)
    enable_testing()
    foreach(name table schema encode device)
        add_executable(test_${name} tests/test_${name}.cpp)
        target_link_libraries(test_${name} PRIVATE csv2tensor)
        target_compile_definitions(test_${name} PRIVATE CSV_TEST_DATA="${CMAKE_CURRENT_SOURCE_DIR}/tests/data")
        add_test(NAME ${name} COMMAND test_${name})
    endforeach()
endif()
