/*
 * csv_inspect: lê CSVs, mostra o schema e manda o tensor para a GPU como TENSOR3.
 *
 *   csv_inspect [--no-header] [--delimiter ;] arquivo.csv [outro.csv ...]
 *
 * Vários arquivos viram o batch: todos precisam ter as mesmas colunas e o mesmo número de linhas.
 */
#include <cstdio>
#include <cstring>
#include <vector>
#include "csv/table.hpp"
#include "csv/schema.hpp"
#include "csv/encode.hpp"
#include "csv/device.hpp"

using namespace CSV;

// Os headers da tensor fazem `using namespace tensor_status;` no escopo global:
// fora do namespace CSV, `Status` sozinho é ambíguo. Use CSV::Status.

static int report(const char* step, CSV::Status st, const POSITION* p, const std::vector<const char*>& paths){
    std::fprintf(stderr, "%s: %s", step, status_name(st));
    if(p != nullptr && p->line > 0){
        std::fprintf(stderr, " (%s, linha %zu", paths[p->table], p->line);
        if(p->column > 0) std::fprintf(stderr, ", coluna %zu", p->column);
        std::fprintf(stderr, ")");
    }
    std::fprintf(stderr, "\n");
    return 1;
}

int main(int argc, char** argv){
    OPTIONS opt;
    std::vector<const char*> paths;

    for(int i = 1; i < argc; i++){
        if(std::strcmp(argv[i], "--no-header") == 0)              opt.header = false;
        else if(std::strcmp(argv[i], "--delimiter") == 0 && i + 1 < argc) opt.delimiter = argv[++i][0];
        else paths.push_back(argv[i]);
    }

    if(paths.empty()){
        std::fprintf(stderr, "uso: %s [--no-header] [--delimiter X] arquivo.csv [outro.csv ...]\n", argv[0]);
        return 2;
    }

    std::vector<TABLE> tables(paths.size());
    std::vector<const TABLE*> ptrs;
    POSITION p{};

    for(size_t k = 0; k < paths.size(); k++){
        CSV::Status st = parse_file(paths[k], &opt, &tables[k], &p);
        p.table = k;
        if(st != CSV_SUCCESS) return report("parse_file", st, &p, paths);
        ptrs.push_back(&tables[k]);
    }

    SCHEMA schema;
    CSV::Status st = fit(ptrs.data(), ptrs.size(), nullptr, &schema, &p);
    if(st != CSV_SUCCESS) return report("fit", st, &p, paths);

    std::printf("%-4s %-20s %-12s %-7s %-6s %s\n", "col", "nome", "tipo", "offset", "width", "categorias");
    for(size_t c = 0; c < schema.columns.size(); c++){
        const COLUMN& col = schema.columns[c];
        std::printf("%-4zu %-20s %-12s %-7zu %-6zu", c + 1, col.name.c_str(), kind_name(col.kind), col.offset, col.width);
        for(size_t j = 0; j < col.categories.size(); j++){
            std::printf("%s%s", j ? ", " : "", col.categories[j].c_str());
        }
        std::printf("\n");
    }

    size_t shape[3] = {};
    st = encoded_shape(ptrs.data(), ptrs.size(), &schema, shape, &p);
    if(st != CSV_SUCCESS) return report("encoded_shape", st, &p, paths);

    HOST::TENSOR h(shape, 3);
    if(h.alloc() != tensor_status::TENSOR_SUCCESS){
        std::fprintf(stderr, "HOST::TENSOR::alloc falhou\n");
        return 1;
    }

    st = encode(ptrs.data(), ptrs.size(), &schema, &h, &p);
    if(st != CSV_SUCCESS) return report("encode", st, &p, paths);

    DEVICE::TENSOR  d(h.shape, h.stride, h.rank);
    DEVICE::TENSOR3 t3 = {};
    tensor_status::Status inner = tensor_status::TENSOR_SUCCESS;

    st = upload(&h, &d, &t3, &inner);
    if(st != CSV_SUCCESS){
        std::fprintf(stderr, "upload: %s (tensor: %s)\n", status_name(st), tensor_status::status_name(inner));
        return 1;
    }

    std::printf("\nTENSOR3 shape [%zu, %zu, %zu], stride [%zu, %zu, %zu]\n",
                t3.shape[0], t3.shape[1], t3.shape[2], t3.stride[0], t3.stride[1], t3.stride[2]);

    // Primeiras linhas da matriz 0, lidas de volta da GPU.
    std::vector<float> back(h.n);
    d.device_to_host(back.data());

    const size_t show = shape[1] < 5 ? shape[1] : 5;
    std::printf("\nmatriz 0, primeiras %zu linhas:\n", show);
    for(size_t r = 0; r < show; r++){
        std::printf("  [");
        for(size_t c = 0; c < shape[2]; c++) std::printf("%s%g", c ? ", " : "", back[r * shape[2] + c]);
        std::printf("]\n");
    }

    return 0;
}
