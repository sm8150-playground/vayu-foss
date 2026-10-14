/*
 * Copyright (C) The LineageOS Project
 *
 * SPDX-License-Identifier: Apache-2.0
 */



#ifndef LIB_SCVE_COMMON_STUB_H
#define LIB_SCVE_COMMON_STUB_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif


typedef struct remote_arg {
    void *pv;
    size_t n;
} remote_arg;


__attribute__((visibility("default")))
int _dspCV_handle(void);


__attribute__((visibility("default")))
int _dspCV_atomic_CompareAndExchange(int *ptr, int newval, int expected);


__attribute__((visibility("default")))
int dspCV_skel_invoke(uint32_t sc, remote_arg *pra);


__attribute__((visibility("default")))
int dspCV_initQ6(void);


__attribute__((visibility("default")))
int dspCV_initQ6_with_attributes(void *attributes, int count);


__attribute__((visibility("default")))
int dspCV_getQ6_concurrency_attributes(void *attributes, int count);


__attribute__((visibility("default")))
int dspCV_deinitQ6(void);

#ifdef __cplusplus
}
#endif

#endif
