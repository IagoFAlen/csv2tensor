// encoded_shape e encode: tabelas + schema -> HOST::TENSOR [batch, linhas, colunas]. Também status_name.
#include <cstring>
#include <initializer_list>
#include "check.hpp"
#include "csv2tensor/table.hpp"
#include "csv2tensor/schema.hpp"
#include "csv2tensor/encode.hpp"

using namespace CSV;

static TABLE parse(const char* text){
    TABLE t;
    parse_string(text, std::strlen(text), nullptr, &t);
    return t;
}

// Compara a matriz k do tensor (pelo stride) com want, linha a linha.
static bool matrix_is(const HOST::TENSOR& h, size_t k, std::initializer_list<float> want){
    if(want.size() != h.shape[1] * h.shape[2]) return false;
    size_t i = 0;
    for(float w : want){
        const size_t r = i / h.shape[2], c = i % h.shape[2];
        if(h.data[k * h.stride[0] + r * h.stride[1] + c * h.stride[2]] != w) return false;
        i++;
    }
    return true;
}

static bool at(const POSITION& p, size_t table, size_t line, size_t column){
    return p.table == table && p.line == line && p.column == column;
}

static bool shape_is(const size_t* s, size_t a, size_t b, size_t c){ return s[0] == a && s[1] == b && s[2] == c; }

int main(){
    TABLE a, b;
    parse_file(CSV_TEST_DATA "/pessoas.csv", nullptr, &a);
    parse_file(CSV_TEST_DATA "/pessoas_b.csv", nullptr, &b);

    SECTION("um CSV -> [1, linhas, colunas]");
    {
        SCHEMA s;
        fit(&a, nullptr, &s);

        size_t shape[3] = {};
        CHECK_STATUS("encoded_shape", encoded_shape(&a, &s, shape), CSV_SUCCESS);
        CHECK("  [1, 3, 6]", shape_is(shape, 1, 3, 6));

        HOST::TENSOR h(3);
        h.alloc_shape(shape);
        h.alloc_data();
        CHECK_STATUS("encode", encode(&a, &s, &h), CSV_SUCCESS);
        //                 idade  altura  Recife SP   nao sim
        CHECK("  numeros e one-hot (cidade, ativo)", matrix_is(h, 0, {
                           31,    1.72f,  1,     0,   0,  1,
                           25,    1.60f,  0,     1,   1,  0,
                           40,    1.85f,  1,     0,   0,  1,
        }));
    }

    SECTION("dois CSVs -> batch [2, linhas, colunas]");
    {
        const TABLE* ts[] = {&a, &b};
        SCHEMA s;
        fit(ts, 2, nullptr, &s);

        size_t shape[3] = {};
        CHECK_STATUS("encoded_shape", encoded_shape(ts, 2, &s, shape), CSV_SUCCESS);
        CHECK("  [2, 3, 7]", shape_is(shape, 2, 3, 7));

        HOST::TENSOR h(3);
        h.alloc_shape(shape);
        h.alloc_data();
        CHECK_STATUS("encode", encode(ts, 2, &s, &h), CSV_SUCCESS);
        //                 idade  altura  Curitiba Recife SP   nao sim
        CHECK("  matriz 0 (pessoas.csv)", matrix_is(h, 0, {
                           31,    1.72f,  0,       1,     0,   0,  1,
                           25,    1.60f,  0,       0,     1,   1,  0,
                           40,    1.85f,  0,       1,     0,   0,  1,
        }));
        CHECK("  matriz 1 (pessoas_b.csv)", matrix_is(h, 1, {
                           19,    1.55f,  1,       0,     0,   1,  0,
                           52,    1.70f,  0,       1,     0,   0,  1,
                           33,    1.68f,  0,       0,     1,   0,  1,
        }));
    }

    SECTION("schema do fit de um CSV, encode de outro");
    {
        SCHEMA s;
        fit(&a, nullptr, &s);                    // sem Curitiba

        size_t shape[3] = {1, 3, s.width};
        HOST::TENSOR h(3);
        h.alloc_shape(shape);
        h.alloc_data();
        POSITION p{};
        CHECK_STATUS("categoria que nao apareceu no fit", encode(&b, &s, &h, &p), CSV_ENCODE_UNKNOWN_CATEGORY);
        CHECK("  Curitiba: linha 2, coluna 3", at(p, 0, 2, 3));
    }
    {
        TABLE train = parse("x\n1\n2\n"), test = parse("x\nabc\n3\n");
        SCHEMA s;
        fit(&train, nullptr, &s);
        size_t shape[3] = {1, 2, 1};
        HOST::TENSOR h(3);
        h.alloc_shape(shape);
        h.alloc_data();
        POSITION p{};
        CHECK_STATUS("texto em coluna NUMERIC", encode(&test, &s, &h, &p), CSV_ENCODE_NOT_NUMBER);
        CHECK("  linha 2, coluna 1", at(p, 0, 2, 1));
    }
    {
        TABLE train = parse("x,y\n1,a\n"), test = parse("x,y\n2,\n");
        SCHEMA s;
        fit(&train, nullptr, &s);
        size_t shape[3] = {1, 1, 2};
        HOST::TENSOR h(3);
        h.alloc_shape(shape);
        h.alloc_data();
        POSITION p{};
        CHECK_STATUS("celula vazia", encode(&test, &s, &h, &p), CSV_ENCODE_EMPTY_CELL);
        CHECK("  linha 2, coluna 2", at(p, 0, 2, 2));
    }

    SECTION("encoded_shape e encode: erros");
    {
        SCHEMA s;
        fit(&a, nullptr, &s);
        TABLE menos = parse("idade,altura,cidade,ativo\n1,2,Recife,sim\n");
        TABLE outro = parse("idade,peso,cidade,ativo\n1,2,Recife,sim\n1,2,Recife,sim\n1,2,Recife,sim\n");
        TABLE estreita = parse("idade\n1\n");
        size_t shape[3] = {};
        POSITION p{};

        const TABLE* ts[] = {&a, &menos};
        CHECK_STATUS("linhas diferentes no batch", encoded_shape(ts, 2, &s, shape, &p), CSV_ENCODE_ROWS_MISMATCH);
        CHECK("  na tabela 1", p.table == 1);
        CHECK_STATUS("cabecalho diferente do schema", encoded_shape(&outro, &s, shape, &p), CSV_ENCODE_HEADER_MISMATCH);
        CHECK("  linha 1, coluna 2", at(p, 0, 1, 2));
        CHECK_STATUS("colunas diferentes do schema", encoded_shape(&estreita, &s, shape), CSV_ENCODE_COLUMN_MISMATCH);
        CHECK_STATUS("shape nulo", encoded_shape(&a, &s, nullptr), CSV_ENCODE_NULL_PARAM);
        CHECK_STATUS("schema nulo", encoded_shape(&a, nullptr, shape), CSV_ENCODE_NULL_PARAM);
        CHECK_STATUS("count 0", encoded_shape(ts, 0, &s, shape), CSV_ENCODE_NULL_PARAM);
    }
    {
        SCHEMA s;
        fit(&a, nullptr, &s);

        size_t certo[3] = {1, 3, 6};
        HOST::TENSOR sem_alloc(3);
        sem_alloc.alloc_shape(certo);            // sem alloc_data
        CHECK_STATUS("tensor sem alloc_data", encode(&a, &s, &sem_alloc), CSV_ENCODE_NOT_ALLOCATED);

        size_t errado[3] = {1, 3, 5};
        HOST::TENSOR estreito(3);
        estreito.alloc_shape(errado);
        estreito.alloc_data();
        CHECK_STATUS("shape [1, 3, 5] em vez de [1, 3, 6]", encode(&a, &s, &estreito), CSV_ENCODE_SHAPE_MISMATCH);

        size_t r2[2] = {3, 6};
        HOST::TENSOR rank2(2);
        rank2.alloc_shape(r2);
        rank2.alloc_data();
        CHECK_STATUS("rank 2", encode(&a, &s, &rank2), CSV_ENCODE_SHAPE_MISMATCH);

        CHECK_STATUS("out nulo", encode(&a, &s, nullptr), CSV_ENCODE_NULL_PARAM);
    }

    SECTION("status_name");
    {
        bool all = true;
        for(int i = 0; i < CSV_STATUS_COUNT; i++){
            if(std::strcmp(status_name(static_cast<CSV::Status>(i)), "UNRECOGNIZED STATUS") == 0) all = false;
        }
        CHECK("todo status tem nome", all);
        CHECK("CSV_STATUS_COUNT nao tem nome", std::strcmp(status_name(CSV_STATUS_COUNT), "UNRECOGNIZED STATUS") == 0);
    }

    return finish();
}
