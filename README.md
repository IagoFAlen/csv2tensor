# csv2tensor

Converte CSV em tensor na GPU, usando a lib [tensor](https://github.com/IagoFAlen/tensor). Colunas numéricas viram números e colunas de texto viram one-hot. O resultado é um `DEVICE::TENSOR3` `[batch, linhas, colunas]`, pronto para as operações da tensor.

```
idade, altura, cidade,       ativo
31,    1.72,   Recife,       sim      ->  [31, 1.72,  1, 0,  0, 1]
25,    1.60,   "São Paulo",  nao      ->  [25, 1.60,  0, 1,  1, 0]
                                              idade altura  cidade  ativo
```

## Como funciona

```
arquivo.csv ─parse_file─► TABLE ─fit─► SCHEMA ─encode─► HOST::TENSOR ─upload─► DEVICE::TENSOR3
```

1. **`parse_file` / `parse_string`**: lê o CSV (RFC 4180: aspas, `""`, quebra de linha dentro de aspas, `\r\n`, BOM) e devolve as células como texto.
2. **`fit`**: decide o tipo de cada coluna. É número se toda célula é um número finito (com ponto decimal, sem depender do locale); senão é categoria, com as categorias em ordem alfabética. O tipo de uma coluna pode ser forçado, por exemplo para classes escritas como `0, 1, 2` virarem one-hot.
3. **`encode`**: escreve um `HOST::TENSOR`. Um CSV vira `[1, linhas, colunas]`; vários CSVs com as mesmas colunas e o mesmo número de linhas viram o batch `[n, linhas, colunas]`.
4. **`upload`**: monta o `DEVICE::TENSOR` com o shape e o stride de `h`, aloca na GPU, copia e monta o `TENSOR3`.

Para treino e teste, faça o `fit` uma vez e use o mesmo `SCHEMA` nos outros CSVs. Assim cada coluna do tensor quer dizer a mesma coisa em todos.

Os erros saem como `CSV::Status`, um por verificação. Quando o erro tem lugar no arquivo, um `POSITION` diz a tabela, a linha e a coluna. Regras completas em [docs/csv.md](docs/csv.md).

## Exemplo

```cpp
#include <cstdio>
#include "csv2tensor/table.hpp"
#include "csv2tensor/schema.hpp"
#include "csv2tensor/encode.hpp"
#include "csv2tensor/device.hpp"

int main(){
    CSV::TABLE    table;
    CSV::SCHEMA   schema;
    CSV::POSITION where{};
    size_t shape[3];

    CSV::Status st = CSV::parse_file("dados.csv", nullptr, &table, &where);
    if(st == CSV::CSV_SUCCESS) st = CSV::fit(&table, nullptr, &schema, &where);
    if(st == CSV::CSV_SUCCESS) st = CSV::encoded_shape(&table, &schema, shape, &where);
    if(st != CSV::CSV_SUCCESS){
        std::printf("%s (linha %zu, coluna %zu)\n", CSV::status_name(st), where.line, where.column);
        return 1;
    }

    HOST::TENSOR h(3);
    h.alloc_shape(shape);                  // copia o shape
    h.alloc_data();
    CSV::encode(&table, &schema, &h);

    DEVICE::TENSOR  d(h.rank);             // o upload copia shape e stride de h
    DEVICE::TENSOR3 t3;                    // vale enquanto d existir
    CSV::upload(&h, &d, &t3);

    std::printf("[%zu, %zu, %zu]\n", t3.shape[0], t3.shape[1], t3.shape[2]);
}
```

Fora do `namespace CSV`, escreva `CSV::Status`: os headers da tensor deixam o `Status` dela visível no escopo global.

### csv_inspect

```sh
csv_inspect tests/data/pessoas.csv tests/data/pessoas_b.csv
csv_inspect --delimiter ';' --no-header dados.csv
```

Mostra o schema, o shape do `TENSOR3` e as primeiras linhas lidas de volta da GPU:

```
col  nome                 tipo         offset  width  categorias
1    idade                NUMERIC      0       1
2    altura               NUMERIC      1       1
3    cidade               CATEGORICAL  2       3     Curitiba, Recife, São Paulo
4    ativo                CATEGORICAL  5       2     nao, sim

TENSOR3 shape [2, 3, 7], stride [21, 7, 1]
```

## Build e instalação

Precisa de CMake 3.24+, um compilador C++17, o CUDA Toolkit e a tensor instalada:

```sh
# na tensor
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release && cmake --build build && sudo cmake --install build

# aqui
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
ctest --test-dir build --output-on-failure
sudo cmake --install build          # /usr/local; outro lugar: --prefix /caminho
```

Os headers vão para `/usr/local/include/csv2tensor/`, a biblioteca para `/usr/local/lib/libcsv2tensor.a` e o `csv_inspect` para `/usr/local/bin`.

```cmake
find_package(csv2tensor 0.1 REQUIRED)       # puxa a tensor sozinho
target_link_libraries(app PRIVATE csv2tensor::csv2tensor)
```

Sem CMake: `g++ -std=c++17 -I/usr/local/cuda/include app.cpp -lcsv2tensor -ltensor -lcudart`.

Para mexer nas duas libs ao mesmo tempo, compile a tensor junto em vez de usar a instalada: `cmake -S . -B build -DTENSOR_DIR=../tensor`. Assim também dá para rodar sem GPU, com a emulação da tensor (`-DTENSOR_EMULATE=ON`). Detalhes em [docs/build.md](docs/build.md).

## Estrutura

```
include/csv2tensor/    API pública
  status.hpp           CSV::Status e status_name
  table.hpp            TABLE, OPTIONS, POSITION, parse_string, parse_file
  schema.hpp           KIND, COLUMN, SCHEMA, fit
  encode.hpp           encoded_shape, encode
  device.hpp           upload
src/                   implementação
tools/csv_inspect.cpp  ferramenta de linha de comando
tests/                 testes e CSVs de exemplo
docs/                  regras do CSV, build e pendências
```

## Limitações

Célula vazia ainda é erro, e não há normalização das colunas numéricas. O que falta está em [docs/todo.md](docs/todo.md).
