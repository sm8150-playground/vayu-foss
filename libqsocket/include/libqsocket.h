/*
 * Copyright (C) The LineageOS Project
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef LIB_QSOCKET_H
#define LIB_QSOCKET_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif


struct ipcr_name {
    uint32_t word0;
    uint32_t word1;
};


__attribute__((visibility("default")))
int ipcr_find_name(int fd, const struct ipcr_name *name, void *entries,
                   uint32_t *ids, int32_t *num, int flag);

#ifdef __cplusplus
}
#endif

#endif
