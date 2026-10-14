/*
 * Copyright (C) The LineageOS Project
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef LIB_TIME_GENOFF_H
#define LIB_TIME_GENOFF_H

#include <stdint.h>


struct time_genoff_args {
    int32_t cmd;
    int32_t reserved;
    void *base;
    int32_t type;
    int32_t unit;
};

#ifdef __cplusplus
extern "C" {
#endif


__attribute__((visibility("default")))
int time_genoff_operation(struct time_genoff_args *args);


__attribute__((visibility("default")))
int time_control_operations(struct time_genoff_args *args);


__attribute__((visibility("default")))
unsigned int time_get_from_timetick(void);

#ifdef __cplusplus
}
#endif

#endif
