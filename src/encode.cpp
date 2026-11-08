#include <algorithm>
#include "csv2tensor/encode.hpp"
#include "number.hpp"

namespace CSV {

    static Status fail(Status s, POSITION* where, size_t table, size_t line, size_t column){
        if(where != nullptr) *where = POSITION{table, line, column};
        return s;
    }

    // Confere que as tabelas batem com o schema e entre si; devolve o número de linhas.
    static Status check_tables(const TABLE* const* tables, size_t count, const SCHEMA* schema, size_t* rows, POSITION* where){
        if(tables == nullptr || schema == nullptr || count == 0) return CSV_ENCODE_NULL_PARAM;
        for(size_t k = 0; k < count; k++){
            if(tables[k] == nullptr) return CSV_ENCODE_NULL_PARAM;
        }

        const size_t columns = schema->columns.size();

        for(size_t k = 0; k < count; k++){
            const TABLE* t = tables[k];
            if(t->columns != columns) return fail(CSV_ENCODE_COLUMN_MISMATCH, where, k, 0, 0);

            if(!t->header.empty()){
                for(size_t c = 0; c < columns; c++){
                    const std::string& name = schema->columns[c].name;
                    if(!name.empty() && t->header[c] != name) return fail(CSV_ENCODE_HEADER_MISMATCH, where, k, 1, c + 1);
                }
            }

            if(t->rows != tables[0]->rows) return fail(CSV_ENCODE_ROWS_MISMATCH, where, k, 0, 0);
        }

        *rows = tables[0]->rows;
        return CSV_SUCCESS;
    }

    Status encoded_shape(const TABLE* const* tables, size_t count, const SCHEMA* schema, size_t shape[3], POSITION* where){
        if(shape == nullptr) return CSV_ENCODE_NULL_PARAM;

        size_t rows = 0;
        Status st = check_tables(tables, count, schema, &rows, where);
        if(st != CSV_SUCCESS) return st;

        shape[0] = count;
        shape[1] = rows;
        shape[2] = schema->width;
        return CSV_SUCCESS;
    }

    Status encode(const TABLE* const* tables, size_t count, const SCHEMA* schema, HOST::TENSOR* out, POSITION* where){
        if(out == nullptr) return CSV_ENCODE_NULL_PARAM;

        size_t rows = 0;
        Status st = check_tables(tables, count, schema, &rows, where);
        if(st != CSV_SUCCESS) return st;

        if(out->data == nullptr || out->stride == nullptr) return CSV_ENCODE_NOT_ALLOCATED;

        if(out->rank != 3 || out->shape == nullptr ||
           out->shape[0] != count || out->shape[1] != rows || out->shape[2] != schema->width){
            return CSV_ENCODE_SHAPE_MISMATCH;
        }

        const size_t st_batch = out->stride[0];
        const size_t st_row   = out->stride[1];
        const size_t st_col   = out->stride[2];

        for(size_t k = 0; k < count; k++){
            const TABLE* t = tables[k];

            for(size_t r = 0; r < rows; r++){
                float* row = out->data + k * st_batch + r * st_row;

                for(size_t c = 0; c < schema->columns.size(); c++){
                    const COLUMN&      col  = schema->columns[c];
                    const std::string& cell = t->at(r, c);

                    if(cell.empty()) return fail(CSV_ENCODE_EMPTY_CELL, where, k, t->lines[r], c + 1);

                    if(col.kind == KIND_NUMERIC){
                        float value = 0.0f;
                        if(!parse_number(cell, &value)) return fail(CSV_ENCODE_NOT_NUMBER, where, k, t->lines[r], c + 1);
                        row[col.offset * st_col] = value;
                        continue;
                    }

                    // CATEGORICAL: categories está ordenado, então a busca é binária.
                    auto it = std::lower_bound(col.categories.begin(), col.categories.end(), cell);
                    if(it == col.categories.end() || *it != cell){
                        return fail(CSV_ENCODE_UNKNOWN_CATEGORY, where, k, t->lines[r], c + 1);
                    }

                    const size_t hot = static_cast<size_t>(it - col.categories.begin());
                    for(size_t j = 0; j < col.width; j++){
                        row[(col.offset + j) * st_col] = (j == hot) ? 1.0f : 0.0f;
                    }
                }
            }
        }

        return CSV_SUCCESS;
    }

    Status encoded_shape(const TABLE* table, const SCHEMA* schema, size_t shape[3], POSITION* where){
        return encoded_shape(&table, 1, schema, shape, where);
    }

    Status encode(const TABLE* table, const SCHEMA* schema, HOST::TENSOR* out, POSITION* where){
        return encode(&table, 1, schema, out, where);
    }
}
