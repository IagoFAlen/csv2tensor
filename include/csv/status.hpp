#pragma once

#include <cstddef>

namespace CSV {
    /*
     * Um status por verificação: o nome do status já diz onde a execução falhou.
     * Onde faz sentido, a função também preenche um POSITION (table.hpp) com o lugar do erro.
     */
    typedef enum Status {
        CSV_SUCCESS,

        // Leitura (table.cpp)
        CSV_READ_NULL_PARAM,            // parse_string, parse_file: text, path ou out == nullptr
        CSV_READ_BAD_DELIMITER,         // parse_*: delimitador é aspas, '\n' ou '\r'
        CSV_READ_OPEN_FAILED,           // parse_file: o arquivo não abriu
        CSV_READ_EMPTY,                 // parse_*: nenhuma linha de dados (só o cabeçalho também conta)
        CSV_READ_UNCLOSED_QUOTE,        // parse_*: o texto acabou dentro de um campo entre aspas
        CSV_READ_QUOTE_IN_FIELD,        // parse_*: aspas no meio de um campo sem aspas, ou texto depois da aspa que fecha
        CSV_READ_RAGGED_ROW,            // parse_*: linha com número de colunas diferente da primeira

        // Schema (schema.cpp)
        CSV_FIT_NULL_PARAM,             // fit: tables, out ou alguma tabela == nullptr, ou count == 0
        CSV_FIT_BAD_KIND,               // fit: forced[c] fora de KIND
        CSV_FIT_COLUMN_MISMATCH,        // fit: tabelas com número de colunas diferente
        CSV_FIT_HEADER_MISMATCH,        // fit: tabelas com cabeçalhos diferentes
        CSV_FIT_EMPTY_CELL,             // fit: célula vazia (valor faltando não é suportado)
        CSV_FIT_NOT_NUMBER,             // fit: coluna forçada como KIND_NUMERIC com célula que não é número finito

        // Codificação (encode.cpp)
        CSV_ENCODE_NULL_PARAM,          // encoded_shape, encode: tables, schema, shape, out ou alguma tabela == nullptr, ou count == 0
        CSV_ENCODE_COLUMN_MISMATCH,     // encoded_shape, encode: tabela com número de colunas diferente do schema
        CSV_ENCODE_HEADER_MISMATCH,     // encoded_shape, encode: cabeçalho diferente do usado no fit
        CSV_ENCODE_ROWS_MISMATCH,       // encoded_shape, encode: tabelas do batch com número de linhas diferente
        CSV_ENCODE_NOT_ALLOCATED,       // encode: out->data ou out->stride == nullptr (alloc não foi chamado ou falhou)
        CSV_ENCODE_SHAPE_MISMATCH,      // encode: out não tem rank 3 e shape [count, rows, schema->width]
        CSV_ENCODE_EMPTY_CELL,          // encode: célula vazia
        CSV_ENCODE_NOT_NUMBER,          // encode: célula de coluna KIND_NUMERIC que não é número finito
        CSV_ENCODE_UNKNOWN_CATEGORY,    // encode: categoria que não apareceu no fit

        // GPU (device.cpp). O status da lib tensor que causou a falha vai em *inner.
        CSV_DEVICE_NULL_PARAM,          // upload: h, d ou out == nullptr
        CSV_DEVICE_ALLOC_FAILED,        // upload: DEVICE::TENSOR::alloc falhou
        CSV_DEVICE_FEED_FAILED,         // upload: DEVICE::TENSOR::feed falhou (shape/stride de d diferente de h, h sem alloc, cópia)
        CSV_DEVICE_LAYOUT_FAILED,       // upload: DEVICE::to_tensor3 falhou

        CSV_STATUS_COUNT
    } Status;

    const char* status_name(Status s);
}
