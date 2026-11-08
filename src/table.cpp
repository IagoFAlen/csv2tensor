#include <fstream>
#include <sstream>
#include <utility>
#include "csv/table.hpp"

namespace CSV {

    // Espaço e tab são brancos, a não ser que sejam o próprio delimitador.
    static bool is_blank(char c, char delimiter){
        return (c == ' ' || c == '\t') && c != delimiter;
    }

    // Fim de linha em i: \n, \r\n ou um \r sozinho no fim do texto.
    static bool is_eol(const char* text, size_t length, size_t i){
        if(text[i] == '\n') return true;
        return text[i] == '\r' && (i + 1 == length || text[i + 1] == '\n');
    }

    static size_t skip_eol(const char* text, size_t length, size_t i){
        if(text[i] == '\r') i++;
        if(i < length && text[i] == '\n') i++;
        return i;
    }

    static Status fail(Status s, POSITION* where, size_t line, size_t column){
        if(where != nullptr) *where = POSITION{0, line, column};
        return s;
    }

    Status parse_string(const char* text, size_t length, const OPTIONS* opt, TABLE* out, POSITION* where){
        if(text == nullptr || out == nullptr) return CSV_READ_NULL_PARAM;

        const OPTIONS defaults;
        if(opt == nullptr) opt = &defaults;

        const char d = opt->delimiter;
        if(d == '"' || d == '\n' || d == '\r') return CSV_READ_BAD_DELIMITER;

        // Monta numa tabela local: out só muda se der tudo certo.
        TABLE t;
        bool need_header  = opt->header;
        bool have_columns = false;

        std::vector<std::string> fields;
        std::string field;
        size_t line = 1;
        size_t i    = 0;

        // BOM UTF-8 (Excel costuma gravar)
        if(length >= 3 && (unsigned char)text[0] == 0xEF && (unsigned char)text[1] == 0xBB && (unsigned char)text[2] == 0xBF){
            i = 3;
        }

        while(i < length){
            // Linha totalmente vazia: pula.
            if(is_eol(text, length, i)){
                i = skip_eol(text, length, i);
                line++;
                continue;
            }

            const size_t record_line = line;
            fields.clear();

            // Um campo por volta, até o fim da linha.
            for(;;){
                const size_t column = fields.size() + 1;
                field.clear();

                while(i < length && is_blank(text[i], d)) i++;

                if(i < length && text[i] == '"'){
                    const size_t open_line = line;
                    i++;
                    for(;;){
                        if(i >= length) return fail(CSV_READ_UNCLOSED_QUOTE, where, open_line, column);

                        const char c = text[i++];
                        if(c == '"'){
                            if(i < length && text[i] == '"'){ field += '"'; i++; continue; }   // "" -> "
                            break;                                                              // aspa que fecha
                        }
                        if(c == '\n') line++;
                        field += c;
                    }

                    // Depois da aspa que fecha, só brancos até o delimitador ou o fim da linha.
                    while(i < length && is_blank(text[i], d)) i++;
                    if(i < length && text[i] != d && !is_eol(text, length, i)){
                        return fail(CSV_READ_QUOTE_IN_FIELD, where, line, column);
                    }
                } else {
                    while(i < length && text[i] != d && !is_eol(text, length, i)){
                        if(text[i] == '"') return fail(CSV_READ_QUOTE_IN_FIELD, where, line, column);
                        field += text[i++];
                    }
                    while(!field.empty() && is_blank(field.back(), d)) field.pop_back();
                }

                fields.push_back(std::move(field));

                if(i < length && text[i] == d){ i++; continue; }    // tem mais campo nesta linha

                if(i < length){                                     // fim da linha (ou do texto)
                    i = skip_eol(text, length, i);
                    line++;
                }
                break;
            }

            if(need_header){
                t.header     = fields;
                t.columns    = fields.size();
                have_columns = true;
                need_header  = false;
                continue;
            }

            if(!have_columns){
                t.columns    = fields.size();
                have_columns = true;
            }

            if(fields.size() != t.columns) return fail(CSV_READ_RAGGED_ROW, where, record_line, 0);

            for(std::string& f : fields) t.cells.push_back(std::move(f));
            t.lines.push_back(record_line);
            t.rows++;
        }

        if(t.rows == 0) return fail(CSV_READ_EMPTY, where, line, 0);

        *out = std::move(t);
        return CSV_SUCCESS;
    }

    Status parse_file(const char* path, const OPTIONS* opt, TABLE* out, POSITION* where){
        if(path == nullptr || out == nullptr) return CSV_READ_NULL_PARAM;

        std::ifstream file(path, std::ios::binary);
        if(!file) return CSV_READ_OPEN_FAILED;

        std::ostringstream buffer;
        buffer << file.rdbuf();
        const std::string text = buffer.str();

        return parse_string(text.data(), text.size(), opt, out, where);
    }
}
