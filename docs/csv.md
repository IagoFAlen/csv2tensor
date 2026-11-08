# Do CSV ao TENSOR3

```
arquivo.csv ──parse_file──► TABLE ──fit──► SCHEMA
                              │               │
                              └──────┬────────┘
                                     ▼
                     encoded_shape + encode ──► HOST::TENSOR [batch, linhas, colunas]
                                                         │
                                                       upload
                                                         ▼
                                     DEVICE::TENSOR (dono) + DEVICE::TENSOR3 (view)
```

```cpp
CSV::TABLE  t;
CSV::SCHEMA s;
size_t shape[3];

CSV::parse_file("dados.csv", nullptr, &t);
CSV::fit(&t, nullptr, &s);
CSV::encoded_shape(&t, &s, shape);

HOST::TENSOR h(3);
h.alloc_shape(shape);                  // copia o shape
h.alloc_data();
CSV::encode(&t, &s, &h);

DEVICE::TENSOR  d(h.rank);             // o upload copia shape e stride de h
DEVICE::TENSOR3 t3;
CSV::upload(&h, &d, &t3);              // t3 vale enquanto d existir
```

Cada chamada devolve `CSV::Status`; o exemplo omite as checagens. [`tools/csv_inspect.cpp`](../tools/csv_inspect.cpp) é o mesmo caminho, com as checagens.

## 1. Leitura (`parse_string`, `parse_file`)

Texto → `TABLE`, com as células ainda como texto, em row-major (`cells[r * columns + c]`).

- Campo entre aspas pode ter delimitador, quebra de linha e aspas dobradas (`""` → `"`).
- Fim de linha `\n` ou `\r\n`. Linhas totalmente vazias são puladas.
- Espaços e tabs em volta do campo são removidos; dentro das aspas, ficam.
- BOM UTF-8 no começo é ignorado.
- Toda linha precisa ter o mesmo número de colunas (`CSV_READ_RAGGED_ROW`).
- `OPTIONS`: `delimiter` (padrão `,`) e `header` (padrão `true`).

`TABLE::lines[r]` guarda a linha do arquivo onde a linha de dados `r` começa, para os erros das etapas seguintes apontarem o lugar certo.

## 2. Schema (`fit`)

Decide, coluna por coluna, o que vira no tensor:

| Tipo | Quando (`KIND_AUTO`) | Colunas no tensor |
|---|---|---|
| `KIND_NUMERIC` | toda célula é número finito | 1, com o valor |
| `KIND_CATEGORICAL` | alguma célula não é número | uma por categoria (one-hot) |

- Número: `std::from_chars`, sem locale. Decimal é sempre ponto: `1.5` é número, `1,5` não é. Aceita sinal e expoente; `nan` e `inf` não contam.
- Categorias em ordem alfabética (por bytes). O mesmo conjunto de categorias dá sempre o mesmo one-hot, não importa a ordem das linhas.
- `forced` muda o tipo de colunas específicas. Uso típico: classes escritas como `0, 1, 2` que precisam virar one-hot (`KIND_CATEGORICAL`).
- Com várias tabelas, as categorias são a união de todas.
- Célula vazia é erro (`CSV_FIT_EMPTY_CELL`): valor faltando ainda não é suportado.

`COLUMN::offset` e `COLUMN::width` dizem onde cada coluna do CSV cai no tensor:

```
idade, altura, cidade,       ativo
31,    1.72,   Recife,       sim      ->  [31, 1.72,  1, 0,  0, 1]
25,    1.60,   "São Paulo",  nao      ->  [25, 1.60,  0, 1,  1, 0]
                                            │   │     └─┬┘   └─┬┘
                                  offset    0   1      2      4
                                  width     1   1      2      2
```

### Treino e teste

Faça o `fit` uma vez (no CSV de treino, ou em todos juntos) e use o mesmo `SCHEMA` no `encode` dos outros. Assim a coluna `k` do tensor quer dizer a mesma coisa em todos. Uma categoria que não apareceu no `fit` dá `CSV_ENCODE_UNKNOWN_CATEGORY`.

## 3. Codificação (`encoded_shape`, `encode`)

- `encoded_shape` devolve `[count, rows, schema->width]`.
- `encode` escreve no `HOST::TENSOR` pelo stride dele. Precisa de rank 3 e do shape exato, e o `alloc_shape` e o `alloc_data` já precisam ter sido chamados.
- Todas as tabelas do batch precisam do mesmo número de linhas e, se tiverem cabeçalho, do mesmo cabeçalho do `fit`.

## 4. GPU (`upload`)

`alloc_shape` + `alloc_stride` + `alloc_data` + `feed` + `DEVICE::to_tensor3` da lib tensor. `d` só precisa do rank: o `upload` copia o shape e o stride de `h` para ele, e `d` continua valendo depois que `h` é destruído. Se `d` já tiver shape ou stride, eles ficam, e o `feed` confere que batem com os de `h`. Se algo falha, o `CSV_DEVICE_*` diz qual passo foi, e `inner` traz o `tensor_status::Status` da lib tensor.

## Erros e `POSITION`

As funções que olham células recebem um `POSITION*` opcional:

| Campo | Significado |
|---|---|
| `table` | índice da tabela no array (0 nas funções de uma tabela) |
| `line` | linha do arquivo, a partir de 1 (o cabeçalho é a linha 1) |
| `column` | coluna, a partir de 1; 0 quando o erro é da linha inteira |
