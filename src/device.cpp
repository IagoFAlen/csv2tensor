#include "csv/device.hpp"
#include "device/layout.hpp"    // lib tensor: DEVICE::to_tensor3

namespace CSV {

    static Status fail(Status s, tensor_status::Status* inner, tensor_status::Status cause){
        if(inner != nullptr) *inner = cause;
        return s;
    }

    Status upload(const HOST::TENSOR* h, DEVICE::TENSOR* d, DEVICE::TENSOR3* out, tensor_status::Status* inner){
        if(h == nullptr || d == nullptr || out == nullptr) return CSV_DEVICE_NULL_PARAM;

        tensor_status::Status st = d->alloc();
        if(st != tensor_status::TENSOR_SUCCESS) return fail(CSV_DEVICE_ALLOC_FAILED, inner, st);

        // O feed confere rank, shape e stride de h contra d antes de copiar.
        st = d->feed(h);
        if(st != tensor_status::TENSOR_SUCCESS) return fail(CSV_DEVICE_FEED_FAILED, inner, st);

        st = DEVICE::to_tensor3(d, out);
        if(st != tensor_status::TENSOR_SUCCESS) return fail(CSV_DEVICE_LAYOUT_FAILED, inner, st);

        return fail(CSV_SUCCESS, inner, tensor_status::TENSOR_SUCCESS);
    }
}
