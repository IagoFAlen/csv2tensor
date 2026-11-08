#pragma once

#include <cstddef>
#include <string>
#include <vector>
#include "csv/status.hpp"
#include "csv/table.hpp"

// Schema: decide o que cada coluna do CSV vira no tensor (número ou one-hot).
namespace CSV {
    typedef enum kind {
        KIND_AUTO,          // só no fit: número se toda célula for número finito, senão categoria
        KIND_NUMERIC,       // 1 coluna no tensor, com o valor
        KIND_CATEGORICAL,   // uma coluna por categoria, one-hot
        KIND_COUNT
    } KIND;

    const char* kind_name(KIND k);

    typedef struct column {
        std::string              name;          // do cabeçalho; vazio se o CSV não tem cabeçalho
        KIND                     kind;          // NUMERIC ou CATEGORICAL (nunca AUTO depois do fit)
        std::vector<std::string> categories;    // CATEGORICAL: em ordem alfabética (bytes); a posição é o índice do one-hot
        size_t                   offset;        // primeira coluna deste campo no tensor
        size_t                   width;         // 1 (NUMERIC) ou categories.size() (CATEGORICAL)
    } COLUMN;

    /*
     *   CSV:     idade, cidade
     *            31,    Recife
     *            25,    São Paulo
     *
     *   schema:  idade  NUMERIC      offset 0, width 1
     *            cidade CATEGORICAL  offset 1, width 2  [Recife, São Paulo]
     *
     *   tensor:  [31, 1, 0]
     *            [25, 0, 1]          width total = 3
     */
    typedef struct schema {
        std::vector<COLUMN> columns;
        size_t              width = 0;          // colunas no tensor: soma dos width
    } SCHEMA;

    /*
     * Lê todas as tabelas (por exemplo, todos os CSVs de um batch, ou o de treino)
     * e monta o schema. As categorias são a união do que aparece em todas elas.
     *
     * forced: nullptr (tudo KIND_AUTO) ou um KIND por coluna. Serve para, por exemplo,
     * tratar como categoria uma coluna de classes escrita com números (0, 1, 2).
     *
     * Em caso de falha, out não é alterado.
     */
    Status fit(const TABLE* const* tables, size_t count, const KIND* forced, SCHEMA* out, POSITION* where = nullptr);
    Status fit(const TABLE* table, const KIND* forced, SCHEMA* out, POSITION* where = nullptr);
}
