#pragma once

#include <cstddef>
#include "host/tensor.hpp"      // lib tensor
#include "csv/status.hpp"
#include "csv/table.hpp"
#include "csv/schema.hpp"

// Codificação: tabelas + schema -> HOST::TENSOR [batch, linhas, colunas].
namespace CSV {
    /*
     * Cada tabela é uma matriz do batch: count tabelas com rows linhas viram
     *
     *     shape = [count, rows, schema->width]
     *
     * Todas precisam ter o mesmo número de linhas. Um CSV só: shape = [1, rows, width].
     */
    Status encoded_shape(const TABLE* const* tables, size_t count, const SCHEMA* schema, size_t shape[3], POSITION* where = nullptr);
    Status encoded_shape(const TABLE* table, const SCHEMA* schema, size_t shape[3], POSITION* where = nullptr);

    /*
     * Preenche out, que precisa ter o shape do encoded_shape e já ter passado pelo alloc():
     *
     *     size_t shape[3];
     *     encoded_shape(&table, &schema, shape);
     *     HOST::TENSOR h(shape, 3);       // shape precisa viver tanto quanto h
     *     h.alloc();
     *     encode(&table, &schema, &h);
     *
     * Escreve pelo stride de out. Em caso de falha, out pode ter sido escrito pela metade.
     */
    Status encode(const TABLE* const* tables, size_t count, const SCHEMA* schema, HOST::TENSOR* out, POSITION* where = nullptr);
    Status encode(const TABLE* table, const SCHEMA* schema, HOST::TENSOR* out, POSITION* where = nullptr);
}
