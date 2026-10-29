// Private call tracing. LAN implementations live in hle_lan_room/socket.c.
#pragma once
#include <stdatomic.h>
#include <stdio.h>
#define ACNH_NET_TRACE() do { \
    static _Atomic unsigned long long acnh_hits; \
    unsigned long long acnh_n = atomic_fetch_add_explicit(&acnh_hits, 1, memory_order_relaxed) + 1; \
    if (acnh_n <= 4 || (acnh_n & (acnh_n - 1)) == 0) \
        fprintf(stderr, "[ACNH NET TRACE] %s call=%llu caller=%p; experimental LAN dispatch\n", __func__, acnh_n, __builtin_return_address(0)); \
} while (0)
