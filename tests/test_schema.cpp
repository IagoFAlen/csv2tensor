// fit: tabelas -> SCHEMA (tipo de cada coluna, categorias, offset e width no tensor).
#include <cstring>
#include <initializer_list>
#include "check.hpp"
#include "csv/table.hpp"
#include "csv/schema.hpp"

using namespace CSV;

static TABLE parse(const char* text){
    TABLE t;
    parse_string(text, std::strlen(text), nullptr, &t);
    return t;
}

static bool same(const std::vector<std::string>& got, std::initializer_list<const char*> want){
    if(got.size() != want.size()) return false;
    size_t i = 0;
    for(const char* w : want){
        if(got[i++] != w) return false;
    }
    return true;
}

static bool col_is(const COLUMN& c, KIND kind, size_t offset, size_t width){
    return c.kind == kind && c.offset == offset && c.width == width;
}

static bool at(const POSITION& p, size_t table, size_t line, size_t column){
    return p.table == table && p.line == line && p.column == column;
}

int main(){
    SECTION("fit: tipos inferidos");
    {
        TABLE t;
        parse_file(CSV_TEST_DATA "/pessoas.csv", nullptr, &t);
        SCHEMA s;
        CHECK_STATUS("pessoas.csv", fit(&t, nullptr, &s), CSV_SUCCESS);
        CHECK("  4 colunas, width 6", s.columns.size() == 4 && s.width == 6);
        CHECK("  idade  NUMERIC     offset 0, width 1", col_is(s.columns[0], KIND_NUMERIC, 0, 1));
        CHECK("  altura NUMERIC     offset 1, width 1", col_is(s.columns[1], KIND_NUMERIC, 1, 1));
        CHECK("  cidade CATEGORICAL offset 2, width 2", col_is(s.columns[2], KIND_CATEGORICAL, 2, 2));
        CHECK("  ativo  CATEGORICAL offset 4, width 2", col_is(s.columns[3], KIND_CATEGORICAL, 4, 2));
        CHECK("  cidade: [Recife, São Paulo]", same(s.columns[2].categories, {"Recife", "São Paulo"}));
        CHECK("  ativo: [nao, sim] (ordem alfabetica)", same(s.columns[3].categories, {"nao", "sim"}));
        CHECK("  nomes vem do cabecalho", s.columns[0].name == "idade" && s.columns[3].name == "ativo");
        CHECK("  NUMERIC nao tem categorias", s.columns[0].categories.empty());
    }
    {
        TABLE t = parse("a,b,c,d,e,f\n1e3,-2.5,+4,.5,nan,1.5\n7, 0 ,\" 8 \",9,inf,x\n");
        SCHEMA s;
        CHECK_STATUS("formatos de numero", fit(&t, nullptr, &s), CSV_SUCCESS);
        CHECK("  1e3, -2.5, +4, .5 e \" 8 \" sao numero",
              s.columns[0].kind == KIND_NUMERIC && s.columns[1].kind == KIND_NUMERIC &&
              s.columns[2].kind == KIND_NUMERIC && s.columns[3].kind == KIND_NUMERIC);
        CHECK("  nan/inf nao sao numero: coluna vira categoria", s.columns[4].kind == KIND_CATEGORICAL);
        CHECK("  um texto basta para virar categoria", s.columns[5].kind == KIND_CATEGORICAL && same(s.columns[5].categories, {"1.5", "x"}));
    }
    {
        TABLE t;
        OPTIONS o; o.delimiter = ';';
        parse_string("v\n1,5\n", 6, &o, &t);
        SCHEMA s;
        fit(&t, nullptr, &s);
        CHECK("\"1,5\" nao e numero (decimal e ponto, sem locale)", s.columns[0].kind == KIND_CATEGORICAL);
    }
    {
        TABLE t = parse("x\n1\n2\n");
        t.header.clear();
        SCHEMA s;
        fit(&t, nullptr, &s);
        CHECK("sem cabecalho: nome vazio", s.columns[0].name.empty());
    }

    SECTION("fit: forced");
    {
        TABLE t;
        parse_file(CSV_TEST_DATA "/pessoas.csv", nullptr, &t);
        const KIND forced[] = {KIND_CATEGORICAL, KIND_AUTO, KIND_AUTO, KIND_AUTO};
        SCHEMA s;
        CHECK_STATUS("idade forcada como CATEGORICAL", fit(&t, forced, &s), CSV_SUCCESS);
        CHECK("  idade: [25, 31, 40], width 3", col_is(s.columns[0], KIND_CATEGORICAL, 0, 3) && same(s.columns[0].categories, {"25", "31", "40"}));
        CHECK("  os offsets seguintes andam: altura 3, cidade 4, ativo 6", s.columns[1].offset == 3 && s.columns[2].offset == 4 && s.columns[3].offset == 6);
        CHECK("  width 8", s.width == 8);
    }
    {
        TABLE t;
        parse_file(CSV_TEST_DATA "/pessoas.csv", nullptr, &t);
        const KIND forced[] = {KIND_AUTO, KIND_AUTO, KIND_NUMERIC, KIND_AUTO};
        SCHEMA s; POSITION p{};
        CHECK_STATUS("cidade forcada como NUMERIC", fit(&t, forced, &s, &p), CSV_FIT_NOT_NUMBER);
        CHECK("  primeira celula que nao e numero: linha 2, coluna 3", at(p, 0, 2, 3));
    }
    {
        TABLE t = parse("a\n1\n");
        const KIND forced[] = {KIND_COUNT};
        SCHEMA s;
        CHECK_STATUS("KIND fora do enum", fit(&t, forced, &s), CSV_FIT_BAD_KIND);
    }

    SECTION("fit: varias tabelas");
    {
        TABLE a, b;
        parse_file(CSV_TEST_DATA "/pessoas.csv", nullptr, &a);
        parse_file(CSV_TEST_DATA "/pessoas_b.csv", nullptr, &b);
        const TABLE* ts[] = {&a, &b};
        SCHEMA s;
        CHECK_STATUS("pessoas.csv + pessoas_b.csv", fit(ts, 2, nullptr, &s), CSV_SUCCESS);
        CHECK("  cidade: uniao [Curitiba, Recife, São Paulo]", same(s.columns[2].categories, {"Curitiba", "Recife", "São Paulo"}));
        CHECK("  width 7", s.width == 7);
    }
    {
        TABLE a = parse("a\n1\n"), b = parse("a\nx\n");
        const TABLE* ts[] = {&a, &b};
        SCHEMA s;
        fit(ts, 2, nullptr, &s);
        CHECK("numero numa tabela e texto na outra: CATEGORICAL", s.columns[0].kind == KIND_CATEGORICAL && same(s.columns[0].categories, {"1", "x"}));
    }
    {
        TABLE a = parse("a,b\n1,2\n"), b = parse("a\n1\n");
        const TABLE* ts[] = {&a, &b};
        SCHEMA s; POSITION p{};
        CHECK_STATUS("numero de colunas diferente", fit(ts, 2, nullptr, &s, &p), CSV_FIT_COLUMN_MISMATCH);
        CHECK("  na tabela 1", p.table == 1);
    }
    {
        TABLE a = parse("a,b\n1,2\n"), b = parse("a,c\n1,2\n");
        const TABLE* ts[] = {&a, &b};
        SCHEMA s; POSITION p{};
        CHECK_STATUS("cabecalho diferente", fit(ts, 2, nullptr, &s, &p), CSV_FIT_HEADER_MISMATCH);
        CHECK("  tabela 1, linha 1, coluna 2", at(p, 1, 1, 2));
    }

    SECTION("fit: erros");
    {
        TABLE a = parse("a,b\n1,2\n"), b = parse("a,b\n1,\n2,3\n");
        const TABLE* ts[] = {&a, &b};
        SCHEMA s; POSITION p{};
        CHECK_STATUS("celula vazia", fit(ts, 2, nullptr, &s, &p), CSV_FIT_EMPTY_CELL);
        CHECK("  tabela 1, linha 2, coluna 2", at(p, 1, 2, 2));
    }
    {
        TABLE t = parse("a\n1\n");
        SCHEMA s;
        const TABLE* nulls[] = {&t, nullptr};
        CHECK_STATUS("tables nulo", fit(static_cast<const TABLE* const*>(nullptr), 1, nullptr, &s), CSV_FIT_NULL_PARAM);
        CHECK_STATUS("count 0", fit(nulls, 0, nullptr, &s), CSV_FIT_NULL_PARAM);
        CHECK_STATUS("uma das tabelas nula", fit(nulls, 2, nullptr, &s), CSV_FIT_NULL_PARAM);
        CHECK_STATUS("out nulo", fit(&t, nullptr, nullptr), CSV_FIT_NULL_PARAM);
    }
    {
        TABLE ok = parse("x\n1\n"), bad = parse("a\n\"\"\n");
        SCHEMA s;
        fit(&ok, nullptr, &s);
        CHECK_STATUS("out intacto depois da falha", fit(&bad, nullptr, &s), CSV_FIT_EMPTY_CELL);
        CHECK("  ainda e o schema anterior", s.columns.size() == 1 && s.columns[0].name == "x");
    }

    SECTION("kind_name");
    {
        bool all = true;
        for(int k = 0; k < KIND_COUNT; k++){
            if(std::strcmp(kind_name(static_cast<KIND>(k)), "UNRECOGNIZED KIND") == 0) all = false;
        }
        CHECK("todo KIND tem nome", all);
        CHECK("KIND_COUNT nao tem nome", std::strcmp(kind_name(KIND_COUNT), "UNRECOGNIZED KIND") == 0);
    }

    return finish();
}
