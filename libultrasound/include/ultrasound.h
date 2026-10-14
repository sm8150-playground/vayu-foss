/*
 * Copyright (C) The LineageOS Project
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _ULTRASOUND_H_
#define _ULTRASOUND_H_

#ifdef __cplusplus
extern "C" {
#endif


__attribute__((visibility("default")))
int ultrasound_enable(int enable);


__attribute__((visibility("default")))
int ultrasound_is_supported(void);

#ifdef __cplusplus
}
#endif

#endif
