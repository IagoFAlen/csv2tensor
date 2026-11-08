// parse_string e parse_file: texto -> TABLE.
#include <cstring>
#include <initializer_list>
#include "check.hpp"
#include "csv/table.hpp"

using namespace CSV;

static Status parse(const char* text, TABLE* t, const OPTIONS* opt = nullptr, POSITION* where = nullptr){
    return parse_string(text, std::strlen(text), opt, t, where);
}

static bool same(const std::vector<std::string>& got, std::initializer_list<const char*> want){
    if(got.size() != want.size()) return false;
    size_t i = 0;
    for(const char* w : want){
        if(got[i++] != w) return false;
    }
    return true;
}

static bool same_lines(const std::vector<size_t>& got, std::initializer_list<size_t> want){
    return got == std::vector<size_t>(want);
}

static bool at(const POSITION& p, size_t table, size_t line, size_t column){
    return p.table == table && p.line == line && p.column == column;
}

int main(){
    SECTION("parse_string");
    {
        TABLE t;
        CHECK_STATUS("cabecalho + 2 linhas", parse("a,b\n1,2\n3,4\n", &t), CSV_SUCCESS);
        CHECK("  header [a, b]", same(t.header, {"a", "b"}));
        CHECK("  2 linhas, 2 colunas", t.rows == 2 && t.columns == 2);
        CHECK("  celulas em row-major", same(t.cells, {"1", "2", "3", "4"}));
        CHECK("  at(1, 0) == \"3\"", t.at(1, 0) == "3");
        CHECK("  linha do arquivo de cada linha de dados: [2, 3]", same_lines(t.lines, {2, 3}));
    }
    {
        TABLE t;
        OPTIONS o; o.header = false;
        CHECK_STATUS("sem cabecalho, sem \\n no fim", parse("1,2\n3,4", &t, &o), CSV_SUCCESS);
        CHECK("  header vazio, 2 linhas", t.header.empty() && t.rows == 2 && same(t.cells, {"1", "2", "3", "4"}));
        CHECK("  linhas [1, 2]", same_lines(t.lines, {1, 2}));
    }
    {
        TABLE t;
        CHECK_STATUS("aspas com delimitador e aspas dobradas", parse("nome,obs\n\"Silva, Ana\",\"disse \"\"oi\"\"\"\n", &t), CSV_SUCCESS);
        CHECK("  [Silva, Ana] [disse \"oi\"]", same(t.cells, {"Silva, Ana", "disse \"oi\""}));
    }
    {
        TABLE t;
        CHECK_STATUS("quebra de linha dentro das aspas", parse("a,b\n\"x\ny\",1\n2,3\n", &t), CSV_SUCCESS);
        CHECK("  celula \"x\\ny\"", t.at(0, 0) == "x\ny");
        CHECK("  linhas [2, 4] (a primeira ocupa 2 linhas do arquivo)", same_lines(t.lines, {2, 4}));
    }
    {
        TABLE t;
        CHECK_STATUS("\\r\\n", parse("a,b\r\n1,2\r\n3,4", &t), CSV_SUCCESS);
        CHECK("  sem \\r nas celulas", same(t.header, {"a", "b"}) && same(t.cells, {"1", "2", "3", "4"}));
    }
    {
        TABLE t;
        CHECK_STATUS("linhas vazias sao puladas", parse("a\n\n1\n\n\n2\n\n", &t), CSV_SUCCESS);
        CHECK("  2 linhas, nas linhas [3, 6] do arquivo", t.rows == 2 && same_lines(t.lines, {3, 6}));
    }
    {
        TABLE t;
        CHECK_STATUS("brancos em volta", parse(" a , b \n 1 , \" x \" \n", &t), CSV_SUCCESS);
        CHECK("  removidos fora das aspas, mantidos dentro", same(t.header, {"a", "b"}) && same(t.cells, {"1", " x "}));
    }
    {
        TABLE t;
        OPTIONS o; o.delimiter = ';';
        CHECK_STATUS("delimitador ';'", parse("a;b\n1,5;2\n", &t, &o), CSV_SUCCESS);
        CHECK("  virgula vira texto: [1,5] [2]", same(t.cells, {"1,5", "2"}));
    }
    {
        TABLE t;
        OPTIONS o; o.delimiter = '\t'; o.header = false;
        CHECK_STATUS("delimitador tab", parse("x\t y\n", &t, &o), CSV_SUCCESS);
        CHECK("  tab separa, espaco e removido", same(t.cells, {"x", "y"}));
    }
    {
        TABLE t;
        CHECK_STATUS("BOM UTF-8", parse("\xEF\xBB\xBF" "a\n1\n", &t), CSV_SUCCESS);
        CHECK("  header [a]", same(t.header, {"a"}));
    }
    {
        TABLE t;
        CHECK_STATUS("celula vazia (o parse aceita; fit e encode recusam)", parse("a,b,c\n1,,\"\"\n", &t), CSV_SUCCESS);
        CHECK("  [1] [] []", same(t.cells, {"1", "", ""}));
    }

    SECTION("parse_string: erros");
    {
        TABLE t; POSITION p{};
        CHECK_STATUS("linha com coluna a mais", parse("a,b\n1,2\n1,2,3\n", &t, nullptr, &p), CSV_READ_RAGGED_ROW);
        CHECK("  em linha 3, coluna 0 (linha inteira)", at(p, 0, 3, 0));
    }
    {
        TABLE t; POSITION p{};
        OPTIONS o; o.header = false;
        CHECK_STATUS("sem cabecalho: linha com coluna a menos", parse("1,2\n3\n", &t, &o, &p), CSV_READ_RAGGED_ROW);
        CHECK("  em linha 2", at(p, 0, 2, 0));
    }
    {
        TABLE t; POSITION p{};
        CHECK_STATUS("aspas sem fechar", parse("a,b\n1,\"abc\nd\n", &t, nullptr, &p), CSV_READ_UNCLOSED_QUOTE);
        CHECK("  aberta na linha 2, coluna 2", at(p, 0, 2, 2));
    }
    {
        TABLE t; POSITION p{};
        CHECK_STATUS("aspas no meio do campo", parse("a\nab\"c\n", &t, nullptr, &p), CSV_READ_QUOTE_IN_FIELD);
        CHECK("  em linha 2, coluna 1", at(p, 0, 2, 1));
    }
    {
        TABLE t; POSITION p{};
        CHECK_STATUS("texto depois da aspa que fecha", parse("a,b\n1,\"x\"y\n", &t, nullptr, &p), CSV_READ_QUOTE_IN_FIELD);
        CHECK("  em linha 2, coluna 2", at(p, 0, 2, 2));
    }
    {
        TABLE t;
        CHECK_STATUS("so o cabecalho", parse("a,b\n", &t), CSV_READ_EMPTY);
        CHECK_STATUS("texto vazio", parse("", &t), CSV_READ_EMPTY);
        CHECK_STATUS("so linhas vazias", parse("\n\r\n\n", &t), CSV_READ_EMPTY);
    }
    {
        TABLE t;
        OPTIONS o; o.delimiter = '"';
        CHECK_STATUS("delimitador aspas", parse("a\n1\n", &t, &o), CSV_READ_BAD_DELIMITER);
        o.delimiter = '\n';
        CHECK_STATUS("delimitador \\n", parse("a\n1\n", &t, &o), CSV_READ_BAD_DELIMITER);
    }
    {
        TABLE t;
        CHECK_STATUS("text nulo", parse_string(nullptr, 0, nullptr, &t), CSV_READ_NULL_PARAM);
        CHECK_STATUS("out nulo", parse_string("a\n1\n", 4, nullptr, nullptr), CSV_READ_NULL_PARAM);
    }
    {
        TABLE t;
        parse("x\n7\n", &t);
        CHECK_STATUS("out intacto depois da falha", parse("a,b\n1\n", &t), CSV_READ_RAGGED_ROW);
        CHECK("  ainda e a tabela anterior", same(t.header, {"x"}) && same(t.cells, {"7"}));
    }

    SECTION("parse_file");
    {
        TABLE t;
        CHECK_STATUS("tests/data/pessoas.csv", parse_file(CSV_TEST_DATA "/pessoas.csv", nullptr, &t), CSV_SUCCESS);
        CHECK("  header [idade, altura, cidade, ativo]", same(t.header, {"idade", "altura", "cidade", "ativo"}));
        CHECK("  3 linhas x 4 colunas", t.rows == 3 && t.columns == 4);
        CHECK("  aspas removidas: \"São Paulo\"", t.at(1, 2) == "São Paulo");
    }
    {
        TABLE t;
        CHECK_STATUS("arquivo que nao existe", parse_file(CSV_TEST_DATA "/nao_existe.csv", nullptr, &t), CSV_READ_OPEN_FAILED);
        CHECK_STATUS("path nulo", parse_file(nullptr, nullptr, &t), CSV_READ_NULL_PARAM);
    }

    return finish();
}
