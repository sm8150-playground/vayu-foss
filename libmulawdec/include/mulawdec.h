/*
 * Copyright (C) The LineageOS Project
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef LIB_MULAW_DEC_H
#define LIB_MULAW_DEC_H

#include <stdint.h>


#define MULAW_SEGMENTS 8

#ifdef __cplusplus
extern "C" {
#endif


__attribute__((visibility("default")))
int mulaw_dec_process(short *out_buf, char *in_buf, unsigned int in_bytes);


extern __attribute__((visibility("default")))
const uint16_t INDEX_OFFSET[MULAW_SEGMENTS];
extern __attribute__((visibility("default")))
const uint16_t INTERVAL_SIZE[MULAW_SEGMENTS];
extern __attribute__((visibility("default")))
const uint16_t MID_OFFSET[MULAW_SEGMENTS];
extern __attribute__((visibility("default")))
const uint16_t SAMP_OFFSET[MULAW_SEGMENTS];

#ifdef __cplusplus
}
#endif

#endif
