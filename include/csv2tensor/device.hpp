#pragma once

#include "tensor/host/tensor.hpp"      // lib tensor
#include "tensor/device/tensor.hpp"    // lib tensor
#include "tensor/status.hpp"           // lib tensor: tensor_status::Status
#include "csv2tensor/status.hpp"

// Último passo: HOST::TENSOR -> GPU -> TENSOR3.
namespace CSV {
    /*
     * Monta d na GPU a partir de h, copia os dados (feed) e monta a view TENSOR3.
     * d só precisa do rank de h: o upload copia o shape e o stride de h para ele.
     *
     *     DEVICE::TENSOR  d(h.rank);
     *     DEVICE::TENSOR3 t3;
     *     upload(&h, &d, &t3);
     *
     * Se d já tiver shape ou stride, eles ficam, e o feed confere que batem com os de h.
     * d tem cópia própria de shape e stride: continua valendo depois que h é destruído.
     * O TENSOR3 não é dono de nada: vale enquanto d existir.
     * inner (opcional) recebe o status da lib tensor que causou a falha (TENSOR_SUCCESS se deu certo).
     */
    Status upload(const HOST::TENSOR* h, DEVICE::TENSOR* d, DEVICE::TENSOR3* out, tensor_status::Status* inner = nullptr);
}
