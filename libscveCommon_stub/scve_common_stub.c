/*
 * Copyright (C) The LineageOS Project
 *
 * SPDX-License-Identifier: Apache-2.0
 */



#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "scve_common_stub.h"


extern int remote_handle_open(const char *name, int *ph);
extern int remote_handle_close(int h);
extern int remote_handle_invoke(int h, uint32_t sc, remote_arg *pra);


static const char dspcv_handle_name[] = "dspCV";


static int g_dspcv_handle = -1;

int _dspCV_handle(void)
{
    if (g_dspcv_handle == -1) {
        int handle = -1;

        if (remote_handle_open(dspcv_handle_name, &handle) == 0) {
            int expected = -1;

            if (!__atomic_compare_exchange_n(&g_dspcv_handle, &expected, handle,
                                             false, __ATOMIC_SEQ_CST,
                                             __ATOMIC_SEQ_CST)) {

                if (handle != -1)
                    remote_handle_close(handle);
            }
        }
    }

    return g_dspcv_handle;
}

int _dspCV_atomic_CompareAndExchange(int *ptr, int newval, int expected)
{
    __atomic_compare_exchange_n(ptr, &expected, newval, false, __ATOMIC_SEQ_CST,
                                __ATOMIC_SEQ_CST);
    return expected;
}

int dspCV_skel_invoke(uint32_t sc, remote_arg *pra)
{
    return remote_handle_invoke(_dspCV_handle(), sc, pra);
}

int dspCV_initQ6(void)
{
    return remote_handle_invoke(_dspCV_handle(), 0x00000000u, NULL);
}

int dspCV_initQ6_with_attributes(void *attributes, int count)
{
    remote_arg args[2];

    args[0].pv = &count;
    args[0].n = sizeof(count);
    args[1].pv = attributes;
    args[1].n = (size_t)count * 8;

    return remote_handle_invoke(_dspCV_handle(), 0x02020000u, args);
}

int dspCV_getQ6_concurrency_attributes(void *attributes, int count)
{
    int session_count[2] = { count, count };
    remote_arg args[3];

    args[0].pv = session_count;
    args[0].n = sizeof(session_count);
    args[1].pv = attributes;
    args[1].n = (size_t)count * 8;
    args[2].pv = attributes;
    args[2].n = (size_t)count * 8;

    return remote_handle_invoke(_dspCV_handle(), 0x03020100u, args);
}

int dspCV_deinitQ6(void)
{
    return remote_handle_invoke(_dspCV_handle(), 0x01000000u, NULL);
}
