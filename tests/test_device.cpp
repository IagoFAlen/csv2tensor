// upload: HOST::TENSOR -> GPU -> TENSOR3, de ponta a ponta a partir do CSV.
#include "check.hpp"
#include "csv/table.hpp"
#include "csv/schema.hpp"
#include "csv/encode.hpp"
#include "csv/device.hpp"

using namespace CSV;

static bool eq3(const size_t* s, size_t a, size_t b, size_t c){ return s[0] == a && s[1] == b && s[2] == c; }

int main(){
    SECTION("CSV -> TENSOR3");
    {
        TABLE a, b;
        parse_file(CSV_TEST_DATA "/pessoas.csv", nullptr, &a);
        parse_file(CSV_TEST_DATA "/pessoas_b.csv", nullptr, &b);
        const TABLE* ts[] = {&a, &b};

        SCHEMA s;
        size_t shape[3] = {};
        fit(ts, 2, nullptr, &s);
        encoded_shape(ts, 2, &s, shape);

        HOST::TENSOR h(shape, 3);
        h.alloc();
        encode(ts, 2, &s, &h);

        DEVICE::TENSOR  d(h.shape, h.stride, h.rank);
        DEVICE::TENSOR3 t3 = {};
        tensor_status::Status inner = tensor_status::TENSOR_BAD_PARAM;

        CHECK_STATUS("upload", upload(&h, &d, &t3, &inner), CSV_SUCCESS);
        CHECK("  inner == TENSOR_SUCCESS", inner == tensor_status::TENSOR_SUCCESS);
        CHECK("  TENSOR3 shape [2, 3, 7]", eq3(t3.shape, 2, 3, 7));
        CHECK("  TENSOR3 stride [21, 7, 1]", eq3(t3.stride, 21, 7, 1));
        CHECK("  TENSOR3 aponta para a memoria de d", t3.data == d.data && t3.data != nullptr);

        float back[2 * 3 * 7] = {};
        d.device_to_host(back);
        bool same = true;
        for(size_t i = 0; i < h.n; i++) if(back[i] != h.data[i]) same = false;
        CHECK("  device_to_host devolve o que o encode escreveu", same);
    }

    SECTION("upload: erros");
    {
        size_t shape[3] = {1, 2, 2};
        HOST::TENSOR h(shape, 3);
        h.alloc();

        size_t outro[3] = {1, 2, 3}, outro_st[3] = {6, 3, 1};
        DEVICE::TENSOR d(outro, outro_st, 3);
        DEVICE::TENSOR3 t3 = {};
        tensor_status::Status inner = tensor_status::TENSOR_SUCCESS;

        CHECK_STATUS("d com shape diferente de h", upload(&h, &d, &t3, &inner), CSV_DEVICE_FEED_FAILED);
        CHECK("  inner == TENSOR_FEED_SHAPE_MISMATCH", inner == tensor_status::TENSOR_FEED_SHAPE_MISMATCH);
    }
    {
        size_t shape[3] = {1, 2, 2}, st[3] = {4, 2, 1};
        HOST::TENSOR h(shape, 3);                // sem alloc
        DEVICE::TENSOR d(shape, st, 3);
        DEVICE::TENSOR3 t3 = {};
        tensor_status::Status inner = tensor_status::TENSOR_SUCCESS;

        CHECK_STATUS("h sem alloc", upload(&h, &d, &t3, &inner), CSV_DEVICE_FEED_FAILED);
        CHECK("  inner == TENSOR_FEED_NULL_DATA", inner == tensor_status::TENSOR_FEED_NULL_DATA);
    }
    {
        size_t shape[3] = {1, 2, 2}, st[3] = {4, 2, 1};
        HOST::TENSOR h(shape, 3);
        DEVICE::TENSOR d(shape, st, 3);
        DEVICE::TENSOR3 t3 = {};
        CHECK_STATUS("h nulo", upload(nullptr, &d, &t3), CSV_DEVICE_NULL_PARAM);
        CHECK_STATUS("d nulo", upload(&h, nullptr, &t3), CSV_DEVICE_NULL_PARAM);
        CHECK_STATUS("out nulo", upload(&h, &d, nullptr), CSV_DEVICE_NULL_PARAM);
    }

    return finish();
}
