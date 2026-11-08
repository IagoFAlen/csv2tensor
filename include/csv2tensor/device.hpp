#pragma once

#include "tensor/host/tensor.hpp"      // lib tensor
#include "tensor/device/tensor.hpp"    // lib tensor
#include "tensor/status.hpp"           // lib tensor: tensor_status::Status
#include "csv2tensor/status.hpp"

// Último passo: HOST::TENSOR -> GPU -> TENSOR3.
namespace CSV {
    /*
     * Aloca d na GPU, copia h para lá (feed) e monta a view TENSOR3.
     * d precisa ter sido construído com o shape e o stride de h:
     *
     *     DEVICE::TENSOR  d(h.shape, h.stride, h.rank);
     *     DEVICE::TENSOR3 t3;
     *     upload(&h, &d, &t3);
     *
     * O TENSOR3 não é dono de nada: vale enquanto d existir.
     * inner (opcional) recebe o status da lib tensor que causou a falha (TENSOR_SUCCESS se deu certo).
     */
    Status upload(const HOST::TENSOR* h, DEVICE::TENSOR* d, DEVICE::TENSOR3* out, tensor_status::Status* inner = nullptr);
}
