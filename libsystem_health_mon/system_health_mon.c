/*
 * Copyright (C) The LineageOS Project
 *
 * SPDX-License-Identifier: Apache-2.0
 */



#include <errno.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <unistd.h>

#include "system_health_mon.h"

#define SHM_DEVICE "/dev/system_health_monitor"
#define SHM_TAG "SHM"


#define SHM_START_IOCTL 0x8004c300


#define SHM_LOG_ERROR 6
#define SHM_LOG_INFO 3
#define SHM_LOG_ID_SYSTEM 3

extern int __android_log_print(int prio, const char *tag, const char *fmt, ...);
extern int __android_log_buf_print(int bufId, int prio, const char *tag,
                                   const char *fmt, ...);

int check_system_health(void)
{
    int fd = open(SHM_DEVICE, O_RDONLY | O_NONBLOCK | O_CLOEXEC);

    if (fd <= 0) {
        __android_log_print(SHM_LOG_ERROR, SHM_TAG,
                            "%s: Failed to open %s - errno %d\n", __func__,
                            SHM_DEVICE, errno);
        return -1;
    }

    if (ioctl(fd, SHM_START_IOCTL, NULL) < 0) {
        __android_log_print(SHM_LOG_ERROR, SHM_TAG,
                            "%s: Failed to initiate health check - errno %d\n",
                            __func__, errno);
        close(fd);
        return 0;
    }

    __android_log_buf_print(SHM_LOG_ID_SYSTEM, SHM_LOG_INFO, SHM_TAG,
                            "%s: Successfully initiated health check\n",
                            __func__);
    close(fd);
    return 0;
}
