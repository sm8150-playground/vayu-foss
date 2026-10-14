/*
 * Copyright (C) The LineageOS Project
 *
 * SPDX-License-Identifier: Apache-2.0
 */



#define LOG_TAG "ultrasound"

#include <errno.h>
#include <poll.h>
#include <stdint.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

#include <cutils/properties.h>
#include <log/log.h>

#include "ultrasound.h"


#define ULTRASOUND_SOCKET_NAME "/dev/socket/audio_hw_socket"


#define ULTRASOUND_HWVERSION_PROP "ro.boot.hwversion"
#define ULTRASOUND_HWVERSION_DEFAULT "0.0.0"


#define ULTRASOUND_MSG_TYPE 17u
#define ULTRASOUND_MSG_PARAM 1u

struct ultrasound_request {
    uint32_t msg_type;
    uint32_t param;
    uint8_t enable;
    uint8_t reserved[255];
};

#define ULTRASOUND_RESPONSE_SIZE 2
#define ULTRASOUND_POLL_TIMEOUT_MS 10000
#define ULTRASOUND_RETRY_LIMIT 3


static int g_sockfd_client = -1;


static void clientConnect(void)
{
    struct sockaddr_un sa;
    int fd;

    memset(&sa, 0, sizeof(sa));

    fd = socket(AF_UNIX, SOCK_SEQPACKET, 0);
    if (fd == -1) {
        ALOGE("%s: Error in socket() %s", __func__, strerror(errno));
    } else {
        sa.sun_family = AF_UNIX;
        strlcpy(sa.sun_path, ULTRASOUND_SOCKET_NAME, sizeof(sa.sun_path));

        if (connect(fd, (struct sockaddr *)&sa, sizeof(sa)) == -1) {
            ALOGE("%s: Error connect server socket %s %s", __func__,
                  ULTRASOUND_SOCKET_NAME, strerror(errno));
            close(fd);
            fd = -1;
        }
    }

    g_sockfd_client = fd;
}


static void clientDisconnect(void)
{
    if (g_sockfd_client < 0) {
        ALOGE("%s: g_sockfd_client is already closed %d", __func__,
              g_sockfd_client);
    }

    close(g_sockfd_client);
    g_sockfd_client = -1;
}


static int clientRequest(int enable)
{
    struct ultrasound_request req;
    struct pollfd pfd;
    uint8_t status[ULTRASOUND_RESPONSE_SIZE] = { 0 };
    ssize_t ret;
    int fd = g_sockfd_client;

    if (fd < 0) {
        ALOGE("%s: sockfd not connect %d", __func__, fd);
        return -1;
    }

    memset(&req, 0, sizeof(req));
    req.msg_type = ULTRASOUND_MSG_TYPE;
    req.param = ULTRASOUND_MSG_PARAM;
    req.enable = enable ? 1 : 0;

    ret = write(fd, &req, sizeof(req));
    if (ret < 0) {
        ALOGE("%s: Error write to server %s", __func__, strerror(errno));
        clientDisconnect();
        return -1;
    }

    ALOGD("%s: write to server %d", __func__, (int)ret);

    pfd.fd = fd;
    pfd.events = POLLIN;
    pfd.revents = 0;
    if (poll(&pfd, 1, ULTRASOUND_POLL_TIMEOUT_MS) != 1 ||
        !(pfd.revents & POLLIN)) {
        ALOGE("%s read timeout error", __func__);
        return -1;
    }

    if (read(fd, status, sizeof(status)) < 0) {
        ALOGE("%s read IO error %s", __func__, strerror(errno));
        clientDisconnect();
        return -1;
    }

    if (status[0] != 1) {
        ALOGE("%s read status error %d", __func__, status[0]);
        return -1;
    }

    return 0;
}

__attribute__((visibility("default")))
int ultrasound_enable(int enable)
{
    int retry_cnt = 0;
    int retry = 1;

    ALOGI("%s: enable ultrasound %d", __func__, enable);

    for (;;) {
        if (g_sockfd_client == -1) {
            clientConnect();
        }

        if (clientRequest(enable) == 0) {
            return 0;
        }

        ALOGE("%s: retry time %d", __func__, retry);
        if (++retry_cnt == ULTRASOUND_RETRY_LIMIT) {
            return 0;
        }
        retry++;
    }
}

__attribute__((visibility("default")))
int ultrasound_is_supported(void)
{
    char hwversion[PROP_VALUE_MAX] = { 0 };

    property_get(ULTRASOUND_HWVERSION_PROP, hwversion,
                 ULTRASOUND_HWVERSION_DEFAULT);
    ALOGI("%s: hwversion = %s", __func__, hwversion);

    return 1;
}
