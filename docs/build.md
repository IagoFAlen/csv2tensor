# Build e testes

## De onde vem a lib tensor

| Modo | Comando | Quando |
|---|---|---|
| instalada (padrão) | `cmake -S . -B build` | uso normal: `find_package(tensor)` acha a tensor em `/usr/local` |
| junto | `cmake -S . -B build -DTENSOR_DIR=../tensor` | mexendo nas duas libs ao mesmo tempo, sem reinstalar a tensor a cada mudança |

Se a tensor estiver instalada fora de `/usr/local`, passe `-DCMAKE_PREFIX_PATH=/caminho/do/prefixo`.

## Na GPU

```sh
cmake -S . -B build
cmake --build build
ctest --test-dir build --output-on-failure
```

Requisitos e problemas comuns (versão do CMake, `nvcc`, `gcc` incompatível) são os mesmos da tensor: ver [../tensor/docs/build.md](../../tensor/docs/build.md).

## Instalação no sistema

Primeiro a tensor, depois o csv2tensor:

```sh
# no projeto tensor
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
sudo cmake --install build

# aqui
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
sudo cmake --install build
```

Instala em `/usr/local` (para outro lugar: `cmake --install build --prefix /caminho`):

| Caminho | Conteúdo |
|---|---|
| `include/csv2tensor/` | headers: `#include "csv2tensor/table.hpp"`... |
| `lib/libcsv2tensor.a` | a biblioteca |
| `lib/cmake/csv2tensor/` | pacote do CMake; puxa a tensor sozinho |
| `bin/csv_inspect` | a ferramenta |

Num projeto CMake (não precisa ligar o CUDA como linguagem):

```cmake
find_package(csv2tensor 0.1 REQUIRED)
target_link_libraries(app PRIVATE csv2tensor::csv2tensor)
```

Sem CMake: `g++ -std=c++17 -I/usr/local/cuda/include app.cpp -lcsv2tensor -ltensor -lcudart` (nessa ordem: o csv2tensor usa a tensor).

## Sem GPU (emulação na CPU)

A tensor instalada é a do build com CUDA, então a emulação precisa da tensor compilada junto:

```sh
cmake -S . -B build-emu -DTENSOR_DIR=../tensor -DTENSOR_EMULATE=ON
cmake --build build-emu
ctest --test-dir build-emu --output-on-failure
```

Usa a emulação da tensor. Só o `test_device` toca a "GPU"; o resto é CPU de qualquer jeito. O build emulado não tem regras de instalação.

Com sanitizers (GCC ou Clang):

```sh
cmake -S . -B build-emu -DTENSOR_DIR=../tensor -DTENSOR_EMULATE=ON -DCMAKE_CXX_FLAGS="-fsanitize=address,undefined -g"
```

## Ferramenta

```sh
./build/csv_inspect tests/data/pessoas.csv tests/data/pessoas_b.csv
./build/csv_inspect --delimiter ';' --no-header dados.csv
```

Mostra o schema, o shape e o stride do TENSOR3 e as primeiras linhas lidas de volta da GPU.

## Os testes

| Arquivo | O que testa |
|---|---|
| [`test_table.cpp`](../tests/test_table.cpp) | leitura: aspas, quebra de linha em aspas, `\r\n`, brancos, delimitadores, BOM, erros com posição |
| [`test_schema.cpp`](../tests/test_schema.cpp) | `fit`: tipos inferidos, formatos de número, `forced`, união de categorias, erros |
| [`test_encode.cpp`](../tests/test_encode.cpp) | `encoded_shape` e `encode`: valores e one-hot, batch, schema de outro CSV, erros, `status_name` |
| [`test_device.cpp`](../tests/test_device.cpp) | `upload` de ponta a ponta (CSV → TENSOR3 → `device_to_host`) e erros com o `inner` da tensor |

Os dados ficam em [`tests/data/`](../tests/data/).
