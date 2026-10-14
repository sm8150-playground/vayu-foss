/*
 * Copyright (C) The LineageOS Project
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef LIB_RILUTILS_H
#define LIB_RILUTILS_H

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif


__attribute__((visibility("default")))
void *record_stream_new(int fd, size_t size);


__attribute__((visibility("default")))
int record_stream_get_next(void *stream, void **out_data, unsigned int *out_len);


__attribute__((visibility("default")))
void record_stream_free(void *stream);


__attribute__((visibility("default")))
unsigned long long ril_nano_time(void);

#ifdef __cplusplus
}
#endif

#endif
