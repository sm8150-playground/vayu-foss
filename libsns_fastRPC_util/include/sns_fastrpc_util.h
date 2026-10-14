/*
 * Copyright (C) The LineageOS Project
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef LIB_SNS_FASTRPC_UTIL_H
#define LIB_SNS_FASTRPC_UTIL_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif


typedef uint32_t sns_remote_handle;


typedef struct {
    int (*remote_handle_open)(const char *name, sns_remote_handle *handle);
    int (*remote_handle_close)(sns_remote_handle handle);
    int (*remote_handle_invoke)(sns_remote_handle handle, uint32_t scalars,
                               void *args, int num_args);
    int (*remote_register_buf)(int fd, void *buf, int size);
    int (*remote_register_buf_attr)(int fd, void *buf, int size, int attr);
    void *(*rpcmem_alloc)(int heapid, uint32_t flags, int size);
    void (*rpcmem_free)(void *po);
    int (*rpcmem_to_fd)(void *po);
} sns_fastrpc_util_t;


__attribute__((visibility("default")))
const char *get_lib(void);


__attribute__((visibility("default")))
int sns_rpc_get_handle(sns_fastrpc_util_t *util);


__attribute__((visibility("default")))
void sns_rpc_reset(void);

#ifdef __cplusplus
}
#endif

#endif
