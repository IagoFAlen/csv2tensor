# Build e testes

Precisa da lib tensor em `../tensor`. Para outro lugar, passe `-DTENSOR_DIR=/caminho/da/tensor` no primeiro comando. O CMake compila a tensor junto (`add_subdirectory`), sem os testes dela.

## Na GPU

```sh
cmake -S . -B build
cmake --build build
ctest --test-dir build --output-on-failure
```

Requisitos e problemas comuns (versão do CMake, `nvcc`, `gcc` incompatível) são os mesmos da tensor: ver [../tensor/docs/build.md](../../tensor/docs/build.md).

## Sem GPU (emulação na CPU)

```sh
cmake -S . -B build-emu -DTENSOR_EMULATE=ON
cmake --build build-emu
ctest --test-dir build-emu --output-on-failure
```

Usa a emulação da tensor. Só o `test_device` toca a "GPU"; o resto é CPU de qualquer jeito.

Com sanitizers (GCC ou Clang):

```sh
cmake -S . -B build-emu -DTENSOR_EMULATE=ON -DCMAKE_CXX_FLAGS="-fsanitize=address,undefined -g"
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
