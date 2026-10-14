/*
 * Copyright (C) The LineageOS Project
 *
 * SPDX-License-Identifier: Apache-2.0
 */



#include <errno.h>
#include <sched.h>
#include <stdint.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <time.h>
#include <unistd.h>

#include "libGPreqcancel_svc.h"

extern int __android_log_print(int prio, const char *tag, const char *fmt, ...);

#define GP_TAG "LibGPCancelSvc"


#define GP_LOG_INFO 4
#define GP_LOG_WARN 5
#define GP_LOG_ERROR 6


#define GP_SOCK_NAME_MAX 108


#define GP_BIND_ATTEMPTS 0x400


#define GP_REQCANCEL_RECEIVED (-65534)
#define GP_REQCANCEL_FAILED (-65536)


static int g_sock_fd = -1;
static char g_sock_name[GP_SOCK_NAME_MAX];


static uint32_t gp_ms_from_clocks(clock_t t0, clock_t t1)
{
    double ms = (double)(t1 - t0) * 1000.0;

    if (!(ms > 0.0))
        return 0;
    if (ms >= 4294967296.0)
        return 0xFFFFFFFFu;
    return (uint32_t)ms;
}

int gp_reqcancel_svc_sock_init(const char *name, size_t len, int do_bind)
{
    struct sockaddr_un addr;
    socklen_t addr_len;
    unsigned long long attempt;
    int rv = 0;

    if (g_sock_fd != -1) {
        __android_log_print(GP_LOG_WARN, GP_TAG, "%s(%s): already initialized",
                            __func__, g_sock_name);
        return 0;
    }

    if (name == NULL) {
        __android_log_print(GP_LOG_ERROR, GP_TAG, "%s(%s): NULL socket name",
                            __func__, g_sock_name);
        rv = -EINVAL;
        goto out;
    }


    if ((size_t)(len - 2) >= (GP_SOCK_NAME_MAX - 1)) {
        __android_log_print(GP_LOG_ERROR, GP_TAG,
                            "%s(%s): socket name length is invalid: %zu vs %u",
                            __func__, g_sock_name, len, GP_SOCK_NAME_MAX);
        rv = -EINVAL;
        goto out;
    }


    if (name[0] != '\0') {
        __android_log_print(GP_LOG_ERROR, GP_TAG,
                            "%s(%s): bad socket name(s): %s", __func__,
                            g_sock_name, name);
        rv = -EINVAL;
        goto out;
    }

    strlcpy(g_sock_name, name + 1, len - 1);

    g_sock_fd = socket(AF_UNIX, SOCK_DGRAM, 0);
    if (g_sock_fd == -1) {
        __android_log_print(GP_LOG_ERROR, GP_TAG, "%s(%s): Socket error: %s",
                            __func__, g_sock_name, strerror(errno));
        rv = -errno;
        goto out;
    }

    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    addr.sun_path[0] = '\0';
    strlcpy(&addr.sun_path[1], name + 1,
            len - 1 < sizeof(addr.sun_path) - 1 ? len - 1
                                                : sizeof(addr.sun_path) - 1);

    if (do_bind == 0)
        return 0;

    addr_len = (socklen_t)((uint32_t)len + 2);
    for (attempt = 1;; attempt++) {
        if (bind(g_sock_fd, (struct sockaddr *)&addr, addr_len) == 0)
            return 0;
        if (errno == EADDRINUSE) {
            __android_log_print(GP_LOG_ERROR, GP_TAG,
                                "%s(%s): Address in use, yield, #%zu", __func__,
                                g_sock_name, (size_t)attempt);
            sched_yield();
        }
        if (attempt == GP_BIND_ATTEMPTS)
            break;
    }

    __android_log_print(GP_LOG_ERROR, GP_TAG, "%s(%s): Error binding socket: %s",
                        __func__, g_sock_name, strerror(errno));
    rv = -errno;

out:
    if (rv != 0 && g_sock_fd != -1) {
        close(g_sock_fd);
        g_sock_fd = -1;
    }
    return rv;
}

void gp_reqcancel_svc_sock_terminate(void)
{
    if (g_sock_fd != -1) {
        shutdown(g_sock_fd, SHUT_RDWR);
        close(g_sock_fd);
        g_sock_fd = -1;
    }
    memset(g_sock_name, 0, sizeof(g_sock_name));
}

int gp_reqcancel_svc_sock_rcv_msg(unsigned int timeout_ms,
                                  unsigned int session_id)
{
    struct sockaddr_un from;
    struct timeval tv;
    socklen_t from_len;
    int msg = 0;
    ssize_t n;

    __android_log_print(GP_LOG_INFO, GP_TAG, "%s(%s):timeout %u ms t",
                        __func__, g_sock_name, timeout_ms);

    if (timeout_ms == 0)
        return 0;

    for (;;) {
        clock_t t0, t1;
        uint32_t elapsed;

        tv.tv_sec = timeout_ms / 1000;
        tv.tv_usec = (timeout_ms % 1000) * 1000;
        setsockopt(g_sock_fd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));

        t0 = clock();
        from_len = sizeof(from);
        n = recvfrom(g_sock_fd, &msg, sizeof(msg), 0, (struct sockaddr *)&from,
                     &from_len);
        t1 = clock();

        elapsed = gp_ms_from_clocks(t0, t1);
        __android_log_print(GP_LOG_ERROR, GP_TAG,
                            "%s(%s): totalTime %u ms from socket", __func__,
                            g_sock_name, elapsed);

        if (elapsed >= timeout_ms)
            timeout_ms = 0;
        else
            timeout_ms -= elapsed;

        if (n == (ssize_t)sizeof(msg)) {
            __android_log_print(GP_LOG_INFO, GP_TAG,
                                "%s(%s): received message with size %d",
                                __func__, g_sock_name, (int)sizeof(msg));
            if (msg == (int)session_id) {
                __android_log_print(GP_LOG_INFO, GP_TAG,
                                    "Receive TEEC_ReqCancel for session id %u",
                                    session_id);
                return GP_REQCANCEL_RECEIVED;
            }
            __android_log_print(GP_LOG_ERROR, GP_TAG,
                                "TA session id %u doesn't match session id %u "
                                "from GPlib",
                                session_id, (unsigned int)msg);
            if (timeout_ms != 0)
                continue;
            return 0;
        }

        if (n == 0) {
            __android_log_print(GP_LOG_ERROR, GP_TAG,
                                "%s(%s): Socket connection closed", __func__,
                                g_sock_name);
        } else if (n == -1) {
            int err = errno;

            if (err == ETIMEDOUT || err == EAGAIN) {
                __android_log_print(GP_LOG_ERROR, GP_TAG,
                                    "%s(%s): No msg received within timeout %lu "
                                    "seconds and %ld microseconds from socket",
                                    __func__, g_sock_name,
                                    (unsigned long)tv.tv_sec, (long)tv.tv_usec);
                return 0;
            }
            __android_log_print(GP_LOG_ERROR, GP_TAG,
                                "%s(%s): error reading from socket: %s",
                                __func__, g_sock_name, strerror(err));
        } else {
            __android_log_print(GP_LOG_ERROR, GP_TAG,
                                "%s(%s): Unknown sized %d message received, "
                                "discarding",
                                __func__, g_sock_name, (int)n);
        }

        if (timeout_ms != 0)
            usleep((useconds_t)((unsigned long long)timeout_ms * 1000));
        return GP_REQCANCEL_FAILED;
    }
}

void gp_reqcancel_svc_sock_send_msg(int msg, const char *name, size_t len)
{
    struct sockaddr_un addr;
    size_t name_len = len - 1;

    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    addr.sun_path[0] = '\0';
    strlcpy(&addr.sun_path[1], name + 1,
            name_len < sizeof(addr.sun_path) - 1 ? name_len
                                                 : sizeof(addr.sun_path) - 1);

    if (g_sock_fd == -1) {
        __android_log_print(GP_LOG_ERROR, GP_TAG, "%s(%s): No active socket",
                            __func__, g_sock_name);
        return;
    }

    if (sendto(g_sock_fd, &msg, sizeof(msg), 0, (struct sockaddr *)&addr,
               (socklen_t)((unsigned long long)len + 2)) !=
        (ssize_t)sizeof(msg)) {
        __android_log_print(GP_LOG_ERROR, GP_TAG,
                            "%s(%s): Error sending data with errno %d", __func__,
                            g_sock_name, errno);
    }
}
