/*
 * Copyright (C) The LineageOS Project
 *
 * SPDX-License-Identifier: Apache-2.0
 */



#include <errno.h>
#include <stdint.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/un.h>
#include <time.h>
#include <unistd.h>

#include "time_genoff.h"

#define TIME_TAG "QC-time-services"
#define TIME_SOCKET_PATH "#time_genoff"
#define TIME_TIMEOUT_SEC 10

#define TIME_ERR EINVAL

#define LOG_ERROR 6
#define LOG_INFO 3

extern int __android_log_print(int prio, const char *tag, const char *fmt, ...);

#define TIME_LOG(prio, ...) \
    __android_log_print((prio), TIME_TAG, __VA_ARGS__)

#define TIME_FUNC_OP "time_genoff_operation"
#define TIME_FUNC_CTL "time_control_operations"


struct time_packet {
    int32_t base;
    int32_t unit;
    int32_t operation;
    int32_t reserved;
    uint64_t value;
    int32_t result;
    int32_t reserved2;
};

static int open_socket(const char *func)
{
    struct timeval timeout = { TIME_TIMEOUT_SEC, 0 };
    struct sockaddr_un addr;
    int fd;

    fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (fd < 0) {
        TIME_LOG(LOG_ERROR, "Lib:%s: Unable to create socket\n", func);
        return -1;
    }

    (void)setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));
    (void)setsockopt(fd, SOL_SOCKET, SO_SNDTIMEO, &timeout, sizeof(timeout));

    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    strlcpy(addr.sun_path, TIME_SOCKET_PATH, sizeof(addr.sun_path));

    if (connect(fd, (struct sockaddr *)&addr,
                (socklen_t)(sizeof(addr.sun_family) + strlen(addr.sun_path))) ==
        -1) {
        TIME_LOG(LOG_ERROR, "Lib:%s: Connection failed !!\n", func);
        close(fd);
        return -1;
    }

    return fd;
}


static const uint32_t time_unit_0[2] = { 1, 0 };
static const uint32_t time_unit_1[2] = { 1, 1 };
static const uint32_t time_unit_2[2] = { 1, 2 };
static const uint32_t time_unit_3[2] = { 1, 3 };
static const uint32_t time_unit_4[2] = { 1, 4 };

int time_genoff_operation(struct time_genoff_args *args)
{
    struct time_packet packet;
    const uint32_t *marker;
    uint64_t value;
    int fd;
    int rc;

    if (args == NULL) {
        TIME_LOG(LOG_ERROR, "Lib:%s: Invalid input to library\n", TIME_FUNC_OP);
        return -TIME_ERR;
    }

    if (args->unit != 1) {
        TIME_LOG(LOG_INFO, "Lib:%s: pargs->base = %d\n", TIME_FUNC_OP, args->cmd);
        TIME_LOG(LOG_INFO, "Lib:%s: pargs->operation = %d\n", TIME_FUNC_OP,
                 args->unit);
    }

    if (args->cmd > 0x12 || (unsigned int)(args->type - 1) > 2 ||
        (unsigned int)args->unit > 4 || args->base == NULL) {
        TIME_LOG(LOG_ERROR, "Lib:%s: Invalid input arguments\n", TIME_FUNC_OP);
        return -TIME_ERR;
    }

    if (args->unit != 1)
        TIME_LOG(LOG_INFO, "Lib:%s: pargs->ts_val = %llu\n", TIME_FUNC_OP,
                 (unsigned long long)*(uint64_t *)args->base);

    fd = open_socket(TIME_FUNC_OP);
    if (fd < 0)
        return -TIME_ERR;

    memset(&packet, 0, sizeof(packet));
    packet.base = args->cmd;
    packet.result = -1;

    switch (args->unit) {
    case 0:
        switch (args->type) {
        case 1:
            value = *(uint64_t *)args->base;
            break;
        case 2:
            value = *(uint64_t *)args->base * 1000u;
            break;
        default:
            value = (uint64_t)mktime((struct tm *)args->base) * 1000u;
            break;
        }
        packet.value = value;
        marker = time_unit_0;
        break;
    case 1:
        marker = time_unit_1;
        break;
    case 2:
        marker = time_unit_2;
        break;
    default:
        TIME_LOG(LOG_ERROR, "Lib:%s: Invalid operation specified\n",
                 TIME_FUNC_OP);
        close(fd);
        return -TIME_ERR;
    }

    memcpy(&packet.unit, marker, sizeof(uint32_t) * 2);

    if (sendto(fd, &packet, sizeof(packet), 0, NULL, 0) < 0) {
        TIME_LOG(LOG_ERROR, "Lib:%s: Send to server failed !!\n", TIME_FUNC_OP);
        close(fd);
        return -TIME_ERR;
    }

    if (args->unit != 1)
        TIME_LOG(LOG_INFO, "Lib:%s: Send to server  passed!!\n", TIME_FUNC_OP);

    if (recvfrom(fd, &packet, sizeof(packet), 0, NULL, NULL) < 0) {
        TIME_LOG(LOG_ERROR, "Lib:%s: Unable to recv data\n", TIME_FUNC_OP);
        close(fd);
        return -TIME_ERR;
    }

    switch (args->unit) {
    case 0:
        TIME_LOG(LOG_ERROR,
                 "Receive Passed == base = %d, unit = %d, operation = %d, "
                 "result = %d\n",
                 packet.base, packet.unit, packet.operation, packet.result);
        if (packet.result != 0) {
            TIME_LOG(LOG_ERROR, "Lib: Time Set for Base = %d failed\n",
                     packet.base);
            close(fd);
            return -TIME_ERR;
        }
        break;
    case 1:
        if (packet.result != 0) {
            TIME_LOG(LOG_ERROR, "Lib: Time Get for Base = %d failed\n",
                     packet.base);
            close(fd);
            return -TIME_ERR;
        }
        break;
    default:
        TIME_LOG(LOG_ERROR,
                 "Receive Passed == base = %d, unit = %d, operation = %d, "
                 "result = %d\n",
                 packet.base, packet.unit, packet.operation, packet.result);
        break;
    }

    close(fd);


    if (args->unit == 1) {
        value = packet.value;
        switch (args->type) {
        case 1:
            *(uint64_t *)args->base = value;
            break;
        case 2:
            *(uint64_t *)args->base = value / 1000u;
            break;
        default: {
            time_t seconds = (time_t)(value / 1000u);
            struct tm result;


            memset(&result, 0, sizeof(result));
            if (gmtime_r(&seconds, &result) == NULL) {
                TIME_LOG(LOG_ERROR, "Lib:%s: Invalid time %llu\n", TIME_FUNC_OP,
                         (unsigned long long)seconds);
                return -TIME_ERR;
            }
            memcpy(args->base, &result, sizeof(result));
            break;
        }
        }
    } else if (args->unit == 2) {
        *(uint64_t *)args->base = packet.value;
    }

    return 0;
}

int time_control_operations(struct time_genoff_args *args)
{
    struct time_packet packet;
    const uint32_t *marker;
    const char *message;
    int fd;

    fd = open_socket(TIME_FUNC_CTL);
    if (fd < 0)
        return -TIME_ERR;

    memset(&packet, 0, sizeof(packet));
    packet.base = args->cmd;
    packet.result = -1;

    switch (args->unit) {
    case 3:
        marker = time_unit_3;
        message =
            "Disable Passed == base = %d, unit = %d, operation = %d, "
            "result = %d\n";
        break;
    case 4:
        marker = time_unit_4;
        message =
            "Enable Passed == base = %d, unit = %d, operation = %d, "
            "result = %d\n";
        break;
    default:
        TIME_LOG(LOG_ERROR, "Lib:%s: Invalid operation specified\n",
                 TIME_FUNC_CTL);
        close(fd);
        return -TIME_ERR;
    }

    memcpy(&packet.unit, marker, sizeof(uint32_t) * 2);

    if (sendto(fd, &packet, sizeof(packet), 0, NULL, 0) < 0) {
        TIME_LOG(LOG_ERROR, "Lib:%s: send to server failed !!\n", TIME_FUNC_CTL);
        close(fd);
        return -TIME_ERR;
    }
    TIME_LOG(LOG_INFO, "Lib:%s: Send to server  passed!!\n", TIME_FUNC_CTL);

    if (recvfrom(fd, &packet, sizeof(packet), 0, NULL, NULL) < 0) {
        TIME_LOG(LOG_ERROR, "Lib:%s: Unable to recv data\n", TIME_FUNC_CTL);
        close(fd);
        return -TIME_ERR;
    }

    TIME_LOG(LOG_ERROR, message, packet.base, packet.unit, packet.operation,
             packet.result);

    *(uint64_t *)args->base = packet.value;
    close(fd);
    return 0;
}


unsigned int time_get_from_timetick(void)
{
    uint64_t counter;
    uint64_t scaled;
    uint64_t a;
    uint64_t b;
    uint32_t coarse;
    uint32_t fraction;

#if defined(__aarch64__)
    __asm__ volatile("mrs %0, cntvct_el0" : "=r"(counter));
#else

    counter = (uint64_t)time(NULL);
#endif

    scaled = counter << 8;
    a = (uint64_t)(((unsigned __int128)scaled * 0x0624dd2f1a9fbe77ULL) >> 64);
    b = (uint64_t)(((unsigned __int128)scaled * 0xaec33e1f671529a5ULL) >> 64);

    fraction = (uint32_t)scaled - (uint32_t)a;
    fraction = (uint32_t)a + (fraction >> 1);
    coarse = (uint32_t)(b >> 22);
    fraction = coarse * (uint32_t)(-49152) + (fraction >> 6);

    return (fraction & 0xffffu) | (coarse << 16);
}
