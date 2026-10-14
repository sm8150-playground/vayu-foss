/*
 * Copyright (C) The LineageOS Project
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef LIB_GPREQCANCEL_SVC_H
#define LIB_GPREQCANCEL_SVC_H

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif




__attribute__((visibility("default")))
int gp_reqcancel_svc_sock_init(const char *name, size_t len, int do_bind);


__attribute__((visibility("default")))
void gp_reqcancel_svc_sock_terminate(void);


__attribute__((visibility("default")))
int gp_reqcancel_svc_sock_rcv_msg(unsigned int timeout_ms,
                                  unsigned int session_id);


__attribute__((visibility("default")))
void gp_reqcancel_svc_sock_send_msg(int session_id, const char *name,
                                    size_t len);

#ifdef __cplusplus
}
#endif

#endif
