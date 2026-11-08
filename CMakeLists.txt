cmake_minimum_required(VERSION 3.24)

# De onde vem a lib tensor:
#   padrão                     find_package(tensor): a versão instalada (cmake --install no projeto tensor)
#   -DTENSOR_DIR=../tensor     compila a tensor junto (add_subdirectory), para mexer nas duas ao mesmo tempo
set(TENSOR_DIR "" CACHE PATH "Pasta da lib tensor (vazio: usar a instalada)")

# Mesma opção da lib tensor: TENSOR_EMULATE=ON compila sem CUDA, emulando a GPU na CPU.
option(TENSOR_EMULATE "Emular o CUDA na CPU (sem nvcc nem GPU)" OFF)
option(CSV_BUILD_TESTS "Compilar os testes" ON)

if(TENSOR_EMULATE AND NOT TENSOR_DIR)
    message(FATAL_ERROR "TENSOR_EMULATE=ON precisa de -DTENSOR_DIR=...: a tensor instalada é a do build com CUDA")
endif()

if(TENSOR_EMULATE OR NOT TENSOR_DIR)
    project(csv2tensor VERSION 0.1.0 LANGUAGES CXX)
else()
    # A tensor compilada junto tem fontes .cu: o CUDA precisa estar ligado aqui.
    # Padrão: a GPU desta máquina (ex.: RTX 4070 Ti -> 89). Para outra: -DCMAKE_CUDA_ARCHITECTURES=89
    if(NOT DEFINED CMAKE_CUDA_ARCHITECTURES)
        set(CMAKE_CUDA_ARCHITECTURES native)
    endif()
    project(csv2tensor VERSION 0.1.0 LANGUAGES CXX CUDA)
    set(CMAKE_CUDA_STANDARD 17)
    set(CMAKE_CUDA_STANDARD_REQUIRED ON)
endif()

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# MSVC: os fontes têm acentos em UTF-8.
if(MSVC)
    add_compile_options($<$<COMPILE_LANGUAGE:CXX>:/utf-8> $<$<COMPILE_LANGUAGE:CUDA>:-Xcompiler=/utf-8>)
endif()

if(TENSOR_DIR)
    set(TENSOR_BUILD_TESTS OFF)     # os testes da tensor rodam no projeto dela
    add_subdirectory(${TENSOR_DIR} ${CMAKE_CURRENT_BINARY_DIR}/tensor)
else()
    find_package(tensor 0.1 CONFIG REQUIRED)
endif()

include(GNUInstallDirs)

add_library(csv2tensor
    src/status.cpp
    src/table.cpp
    src/schema.cpp
    src/encode.cpp
    src/device.cpp
)
add_library(csv2tensor::csv2tensor ALIAS csv2tensor)
set_target_properties(csv2tensor PROPERTIES POSITION_INDEPENDENT_CODE ON)

# Os headers ficam em include/csv2tensor/: quem usa escreve #include "csv2tensor/table.hpp".
target_include_directories(csv2tensor
    PUBLIC  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
            $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)          # number.hpp
target_link_libraries(csv2tensor PUBLIC tensor::tensor)

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

# Instalação: cmake --install build (padrão /usr/local; outro lugar: --prefix /caminho)
#   <prefix>/include/csv2tensor/...      headers
#   <prefix>/lib/libcsv2tensor.a
#   <prefix>/lib/cmake/csv2tensor/       find_package(csv2tensor) -> csv2tensor::csv2tensor
#   <prefix>/bin/csv_inspect
# Só no build com CUDA: a tensor emulada não é instalada.
if(TENSOR_EMULATE)
    message(STATUS "csv2tensor: TENSOR_EMULATE=ON, sem regras de instalação")
else()
    include(CMakePackageConfigHelpers)

    install(TARGETS csv2tensor EXPORT csv2tensorTargets
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
    install(TARGETS csv_inspect RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
    install(DIRECTORY include/csv2tensor DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

    install(EXPORT csv2tensorTargets
        NAMESPACE csv2tensor::
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/csv2tensor)

    configure_package_config_file(cmake/csv2tensorConfig.cmake.in
        ${CMAKE_CURRENT_BINARY_DIR}/csv2tensorConfig.cmake
        INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/csv2tensor)
    write_basic_package_version_file(${CMAKE_CURRENT_BINARY_DIR}/csv2tensorConfigVersion.cmake
        COMPATIBILITY SameMinorVersion)
    install(FILES
        ${CMAKE_CURRENT_BINARY_DIR}/csv2tensorConfig.cmake
        ${CMAKE_CURRENT_BINARY_DIR}/csv2tensorConfigVersion.cmake
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/csv2tensor)
endif()
