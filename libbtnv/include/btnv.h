/*
 * Copyright (C) The LineageOS Project
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef LIB_BTNV_H
#define LIB_BTNV_H

#include <stdint.h>


struct nv_persist_params {
    uint32_t type;
    uint32_t field4;
    uint8_t len;
    uint8_t data[7];
};


__attribute__((visibility("default")))
int bt_nv_init(void);

__attribute__((visibility("default")))
int bt_nv_deinit(void);

__attribute__((visibility("default")))
int bt_nv_write(struct nv_persist_params *params, unsigned char count);

__attribute__((visibility("default")))
int bt_nv_read(struct nv_persist_params *params);

#ifdef __cplusplus
extern "C" {
#endif


__attribute__((visibility("default")))
int bt_nv_cmd(int cmd, int type, void *out, int arg);


__attribute__((visibility("default")))
int bt_nv_is_random_addr(void);

#ifdef __cplusplus
}
#endif

#endif
