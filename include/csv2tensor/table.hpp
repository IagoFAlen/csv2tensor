#pragma once

#include <cstddef>
#include <string>
#include <vector>
#include "csv2tensor/status.hpp"

// Leitura do CSV: texto -> células (ainda como texto). Nenhuma conversão para número aqui.
namespace CSV {
    typedef struct options {
        char delimiter = ',';
        bool header    = true;      // a primeira linha é o nome das colunas
    } OPTIONS;

    /*
     * Onde um erro aconteceu, para mostrar a quem escreveu o CSV.
     *   table:  índice da tabela no array passado (0 nas funções de uma tabela só)
     *   line:   linha do arquivo, começando em 1 (o cabeçalho é a linha 1)
     *   column: coluna, começando em 1; 0 quando o erro é da linha inteira
     */
    typedef struct position {
        size_t table;
        size_t line;
        size_t column;
    } POSITION;

    /*
     * O CSV lido, célula por célula, em row-major: cells[r * columns + c].
     * É dono das strings; copiar é seguro (só custa memória).
     */
    typedef struct table {
        std::vector<std::string> header;    // vazio se OPTIONS::header == false
        std::vector<std::string> cells;
        std::vector<size_t>      lines;     // lines[r]: linha do arquivo onde a linha de dados r começa
        size_t rows    = 0;
        size_t columns = 0;

        const std::string& at(size_t r, size_t c) const { return cells[r * columns + c]; }
    } TABLE;

    /*
     * Regras (RFC 4180, com algumas folgas):
     *   - campo entre aspas pode ter delimitador, quebra de linha e aspas dobradas ("" -> ")
     *   - fim de linha \n ou \r\n; linhas totalmente vazias são puladas
     *   - espaços em volta de um campo são removidos (dentro das aspas, ficam)
     *   - BOM UTF-8 no começo é ignorado
     *   - toda linha precisa ter o mesmo número de colunas
     *
     * opt == nullptr usa OPTIONS padrão. Em caso de falha, out não é alterado.
     */
    Status parse_string(const char* text, size_t length, const OPTIONS* opt, TABLE* out, POSITION* where = nullptr);
    Status parse_file(const char* path, const OPTIONS* opt, TABLE* out, POSITION* where = nullptr);
}
