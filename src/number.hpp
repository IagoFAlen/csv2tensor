#pragma once

#include <charconv>
#include <cmath>
#include <string>

namespace CSV {
    /*
     * Lê a célula inteira como float. Usa std::from_chars, que não depende do locale:
     * o separador decimal é sempre o ponto ("1.5"; "1,5" não é número).
     * Aceita sinal (+/-), expoente ("1e3") e brancos em volta. nan e inf não contam como número.
     */
    inline bool parse_number(const std::string& cell, float* out){
        const char* begin = cell.data();
        const char* end   = begin + cell.size();

        while(begin < end && (*begin == ' ' || *begin == '\t')) begin++;
        while(end > begin && (end[-1] == ' ' || end[-1] == '\t')) end--;

        if(begin < end && *begin == '+') begin++;       // from_chars não aceita '+'
        if(begin == end) return false;

        float value = 0.0f;
        const std::from_chars_result r = std::from_chars(begin, end, value);
        if(r.ec != std::errc() || r.ptr != end) return false;
        if(!std::isfinite(value)) return false;

        *out = value;
        return true;
    }
}
