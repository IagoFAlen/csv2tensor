#include "csv2tensor/device.hpp"
#include "tensor/device/layout.hpp"    // lib tensor: DEVICE::to_tensor3

namespace CSV {

    static Status fail(Status s, tensor_status::Status* inner, tensor_status::Status cause){
        if(inner != nullptr) *inner = cause;
        return s;
    }

    Status upload(const HOST::TENSOR* h, DEVICE::TENSOR* d, DEVICE::TENSOR3* out, tensor_status::Status* inner){
        if(h == nullptr || d == nullptr || out == nullptr) return CSV_DEVICE_NULL_PARAM;

        // h precisa estar completo (alloc_shape + alloc_data) antes de qualquer coisa em d.
        if(h->shape == nullptr || h->stride == nullptr || h->data == nullptr) return CSV_DEVICE_HOST_NOT_ALLOCATED;

        // alloc_shape e alloc_stride leem d->rank posições dos arrays de h.
        if(d->rank != h->rank) return CSV_DEVICE_RANK_MISMATCH;

        tensor_status::Status st;

        // d só com o rank: recebe uma cópia do shape e do stride de h.
        // Se d já tem shape ou stride, ficam os dele, e o feed confere contra h.
        if(d->shape == nullptr){
            st = d->alloc_shape(h->shape);
            if(st != tensor_status::TENSOR_SUCCESS) return fail(CSV_DEVICE_ALLOC_FAILED, inner, st);
        }

        if(d->stride == nullptr){
            st = d->alloc_stride(h->stride);
            if(st != tensor_status::TENSOR_SUCCESS) return fail(CSV_DEVICE_ALLOC_FAILED, inner, st);
        }

        st = d->alloc_data();
        if(st != tensor_status::TENSOR_SUCCESS) return fail(CSV_DEVICE_ALLOC_FAILED, inner, st);

        // O feed confere rank, shape e stride de h contra d antes de copiar.
        st = d->feed(h);
        if(st != tensor_status::TENSOR_SUCCESS) return fail(CSV_DEVICE_FEED_FAILED, inner, st);

        st = DEVICE::to_tensor3(d, out);
        if(st != tensor_status::TENSOR_SUCCESS) return fail(CSV_DEVICE_LAYOUT_FAILED, inner, st);

        return fail(CSV_SUCCESS, inner, tensor_status::TENSOR_SUCCESS);
    }
}
