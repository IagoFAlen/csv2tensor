// Mini framework de teste: imprime cada caso e conta as falhas.
#pragma once

#include <cstdio>
#include "csv/status.hpp"

static int g_fails = 0;

#define SECTION(name) std::printf("\n  %s\n", name)

#define CHECK(name, cond) do {                                                        \
    bool ok_ = (cond);                                                                \
    std::printf("    %-62s %s\n", name, ok_ ? "ok" : "FALHOU");                       \
    if (!ok_) g_fails++;                                                              \
} while (0)

#define CHECK_STATUS(name, got, want) do {                                            \
    CSV::Status got_ = (got), want_ = (want);                                         \
    bool ok_ = got_ == want_;                                                         \
    std::printf("    %-62s %-40s %s\n", name, CSV::status_name(got_), ok_ ? "ok" : "FALHOU"); \
    if (!ok_) { g_fails++; std::printf("      esperado: %s\n", CSV::status_name(want_)); } \
} while (0)

static int finish(){
    std::printf("\n  %s\n", g_fails ? "HA FALHAS" : "todos os casos passaram");
    return g_fails != 0;
}
