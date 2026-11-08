# csv2tensor

Converte CSV em tensor da lib [tensor](../tensor): CSV → `TABLE` → `SCHEMA` → `HOST::TENSOR` → GPU → `DEVICE::TENSOR3`.

## Escopo

- Só a preparação dos dados. Rede neural fica em outro projeto; operações de tensor ficam na lib tensor.
- Um CSV é uma matriz `[1, linhas, colunas]`. Vários CSVs com as mesmas colunas e o mesmo número de linhas viram o batch `[n, linhas, colunas]`.
- Coluna numérica vira 1 coluna no tensor; coluna de texto vira one-hot.
- Pipeline e regras em [docs/csv.md](docs/csv.md); build em [docs/build.md](docs/build.md).

## Padrões

- Um `Status` por verificação (`include/csv2tensor/status.hpp`), com prefixo `CSV_`. Nome de status e de enum via tabela indexada pelo enum, nunca `switch`.
- Funções devolvem `Status`; o resultado sai por ponteiro. Se a falha tem lugar no CSV, preenche `POSITION`.
- Em caso de falha, `out` fica intacto (exceto `encode`, que escreve direto no tensor).
- Os headers da tensor fazem `using namespace tensor_status;` no escopo global: fora do `namespace CSV`, escreva `CSV::Status`.

## Ambiente

- Headers em `include/csv2tensor/`; os da tensor como `"tensor/host/tensor.hpp"`. As duas libs se instalam em `/usr/local` com `cmake --install` ([docs/build.md](docs/build.md)).
- A tensor vem do `find_package(tensor)` (instalada). Para desenvolver as duas juntas: `-DTENSOR_DIR=../tensor`.
- O `git` está instalado só no WSL. A partir do Windows, use `wsl git ...`.
