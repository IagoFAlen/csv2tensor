#include "csv2tensor/status.hpp"

namespace CSV {

    /*
     * Tabela indexada pelo Status. Cada entrada guarda o próprio status, e o
     * static_assert abaixo confere que a entrada i é o status i: um status novo
     * fora de ordem ou sem nome vira erro de compilação.
     */
    typedef struct entry {
        Status      status;
        const char* name;
    } ENTRY;

    static constexpr ENTRY NAMES[] = {
        {CSV_SUCCESS,                 "SUCCESS"},

        {CSV_READ_NULL_PARAM,         "READ: NULL PARAM"},
        {CSV_READ_BAD_DELIMITER,      "READ: INVALID DELIMITER"},
        {CSV_READ_OPEN_FAILED,        "READ: FILE OPEN FAILED"},
        {CSV_READ_EMPTY,              "READ: NO DATA ROWS"},
        {CSV_READ_UNCLOSED_QUOTE,     "READ: UNCLOSED QUOTE"},
        {CSV_READ_QUOTE_IN_FIELD,     "READ: MISPLACED QUOTE"},
        {CSV_READ_RAGGED_ROW,         "READ: ROW WITH WRONG COLUMN COUNT"},

        {CSV_FIT_NULL_PARAM,          "FIT: NULL PARAM"},
        {CSV_FIT_BAD_KIND,            "FIT: INVALID KIND"},
        {CSV_FIT_COLUMN_MISMATCH,     "FIT: TABLES WITH DIFFERENT COLUMN COUNT"},
        {CSV_FIT_HEADER_MISMATCH,     "FIT: TABLES WITH DIFFERENT HEADERS"},
        {CSV_FIT_EMPTY_CELL,          "FIT: EMPTY CELL"},
        {CSV_FIT_NOT_NUMBER,          "FIT: NUMERIC COLUMN WITH NON-NUMBER"},

        {CSV_ENCODE_NULL_PARAM,       "ENCODE: NULL PARAM"},
        {CSV_ENCODE_COLUMN_MISMATCH,  "ENCODE: COLUMN COUNT != SCHEMA"},
        {CSV_ENCODE_HEADER_MISMATCH,  "ENCODE: HEADER != SCHEMA"},
        {CSV_ENCODE_ROWS_MISMATCH,    "ENCODE: TABLES WITH DIFFERENT ROW COUNT"},
        {CSV_ENCODE_NOT_ALLOCATED,    "ENCODE: HOST TENSOR NOT ALLOCATED"},
        {CSV_ENCODE_SHAPE_MISMATCH,   "ENCODE: TENSOR SHAPE != [BATCH, ROWS, WIDTH]"},
        {CSV_ENCODE_EMPTY_CELL,       "ENCODE: EMPTY CELL"},
        {CSV_ENCODE_NOT_NUMBER,       "ENCODE: NUMERIC COLUMN WITH NON-NUMBER"},
        {CSV_ENCODE_UNKNOWN_CATEGORY, "ENCODE: CATEGORY NOT SEEN IN FIT"},

        {CSV_DEVICE_NULL_PARAM,       "DEVICE: NULL PARAM"},
        {CSV_DEVICE_HOST_NOT_ALLOCATED, "DEVICE: HOST TENSOR NOT ALLOCATED"},
        {CSV_DEVICE_RANK_MISMATCH,    "DEVICE: RANK OF D != RANK OF H"},
        {CSV_DEVICE_ALLOC_FAILED,     "DEVICE: ALLOC FAILED"},
        {CSV_DEVICE_FEED_FAILED,      "DEVICE: FEED FAILED"},
        {CSV_DEVICE_LAYOUT_FAILED,    "DEVICE: TO_TENSOR3 FAILED"},
    };

    static constexpr size_t NAMES_COUNT = sizeof(NAMES) / sizeof(NAMES[0]);

    static constexpr bool names_in_order(){
        for(size_t i = 0; i < NAMES_COUNT; i++){
            if(static_cast<size_t>(NAMES[i].status) != i) return false;
        }
        return true;
    }

    static_assert(NAMES_COUNT == CSV_STATUS_COUNT, "todo Status precisa de um nome em NAMES");
    static_assert(names_in_order(), "NAMES precisa estar na mesma ordem do enum Status");

    const char* status_name(Status s){
        if(static_cast<size_t>(s) >= NAMES_COUNT) return "UNRECOGNIZED STATUS";
        return NAMES[s].name;
    }
}
