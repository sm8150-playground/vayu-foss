/*
 * Copyright (C) 2026 The LineageOS Project
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ANDROID_MDMIMGLOAD_H
#define ANDROID_MDMIMGLOAD_H

#include <sys/cdefs.h>

__BEGIN_DECLS


__attribute__((visibility("default"))) int wait_for_com_port(const char *port);


__attribute__((visibility("default"))) int mdm_img_transfer(const char *mdm_name,
                                                            const char *link_type,
                                                            const char *dev_name,
                                                            const char *w_arg,
                                                            const char *g_arg, int mode);

__END_DECLS

#endif
