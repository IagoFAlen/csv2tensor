#include <algorithm>
#include <utility>
#include "csv2tensor/schema.hpp"
#include "number.hpp"

namespace CSV {

    static const char* const KIND_NAMES[] = {"AUTO", "NUMERIC", "CATEGORICAL"};
    static_assert(sizeof(KIND_NAMES) / sizeof(KIND_NAMES[0]) == KIND_COUNT, "todo KIND precisa de um nome");

    const char* kind_name(KIND k){
        if(static_cast<size_t>(k) >= KIND_COUNT) return "UNRECOGNIZED KIND";
        return KIND_NAMES[k];
    }

    static Status fail(Status s, POSITION* where, size_t table, size_t line, size_t column){
        if(where != nullptr) *where = POSITION{table, line, column};
        return s;
    }

    Status fit(const TABLE* const* tables, size_t count, const KIND* forced, SCHEMA* out, POSITION* where){
        if(tables == nullptr || out == nullptr || count == 0) return CSV_FIT_NULL_PARAM;
        for(size_t k = 0; k < count; k++){
            if(tables[k] == nullptr) return CSV_FIT_NULL_PARAM;
        }

        const TABLE*  first   = tables[0];
        const size_t  columns = first->columns;

        if(forced != nullptr){
            for(size_t c = 0; c < columns; c++){
                if(static_cast<size_t>(forced[c]) >= KIND_COUNT) return fail(CSV_FIT_BAD_KIND, where, 0, 0, c + 1);
            }
        }

        // Todas as tabelas precisam descrever as mesmas colunas.
        for(size_t k = 1; k < count; k++){
            const TABLE* t = tables[k];
            if(t->columns != columns) return fail(CSV_FIT_COLUMN_MISMATCH, where, k, 0, 0);

            if(!first->header.empty() && !t->header.empty()){
                for(size_t c = 0; c < columns; c++){
                    if(t->header[c] != first->header[c]) return fail(CSV_FIT_HEADER_MISMATCH, where, k, 1, c + 1);
                }
            }
        }

        SCHEMA s;
        s.columns.resize(columns);

        for(size_t c = 0; c < columns; c++){
            COLUMN& col = s.columns[c];
            col.name = first->header.empty() ? std::string() : first->header[c];

            // Passa por todas as células da coluna: vazia é erro; guarda a primeira que não é número.
            bool     numeric = true;
            POSITION not_number{};
            float    ignored;

            for(size_t k = 0; k < count; k++){
                const TABLE* t = tables[k];
                for(size_t r = 0; r < t->rows; r++){
                    const std::string& cell = t->at(r, c);
                    if(cell.empty()) return fail(CSV_FIT_EMPTY_CELL, where, k, t->lines[r], c + 1);

                    if(numeric && !parse_number(cell, &ignored)){
                        numeric    = false;
                        not_number = POSITION{k, t->lines[r], c + 1};
                    }
                }
            }

            const KIND want = (forced != nullptr) ? forced[c] : KIND_AUTO;
            if(want == KIND_NUMERIC && !numeric){
                if(where != nullptr) *where = not_number;
                return CSV_FIT_NOT_NUMBER;
            }

            col.kind = (want == KIND_AUTO) ? (numeric ? KIND_NUMERIC : KIND_CATEGORICAL) : want;

            if(col.kind == KIND_CATEGORICAL){
                for(size_t k = 0; k < count; k++){
                    const TABLE* t = tables[k];
                    for(size_t r = 0; r < t->rows; r++) col.categories.push_back(t->at(r, c));
                }
                // Ordem alfabética: o mesmo conjunto de categorias dá o mesmo one-hot,
                // não importa a ordem das linhas.
                std::sort(col.categories.begin(), col.categories.end());
                col.categories.erase(std::unique(col.categories.begin(), col.categories.end()), col.categories.end());
            }

            col.offset = s.width;
            col.width  = (col.kind == KIND_NUMERIC) ? 1 : col.categories.size();
            s.width   += col.width;
        }

        *out = std::move(s);
        return CSV_SUCCESS;
    }

    Status fit(const TABLE* table, const KIND* forced, SCHEMA* out, POSITION* where){
        return fit(&table, 1, forced, out, where);
    }
}
