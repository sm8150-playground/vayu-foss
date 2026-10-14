/*
 * Copyright (C) The LineageOS Project
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef LIB_CNEQMIUTILS_H
#define LIB_CNEQMIUTILS_H

#ifdef __cplusplus


class CneQmiUtils {
public:

    __attribute__((visibility("default")))
    static bool isQmiPort(char const *ifname);
};


__attribute__((visibility("default")))
extern char const *qmiPorts[47];

#endif

#endif
