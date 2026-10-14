/*
 * Copyright (C) The LineageOS Project
 *
 * SPDX-License-Identifier: Apache-2.0
 */



#include "libqsocket.h"

#include <errno.h>
#include <stdint.h>
#include <stdlib.h>
#include <sys/ioctl.h>


struct ipcr_name_req {
    uint32_t word0;
    uint32_t word1;
    uint32_t num;
    int32_t found;
    uint32_t flag;
};


struct ipcr_name_entry {
    uint64_t value;
    uint32_t unused;
    uint32_t id;
};

#define IPCR_REQ_HDR_SIZE 0x14
#define IPCR_ENTRY_STRIDE 16
#define IPCR_RECORD_STRIDE 0x14


#define IPCR_IOCTL_LOOKUP _IOWR(0xC3, 2, struct ipcr_name_req)


typedef char ipcr_name_req_size_check[(sizeof(struct ipcr_name_req) ==
                                       IPCR_REQ_HDR_SIZE) ? 1 : -1];

int ipcr_find_name(int fd, const struct ipcr_name *name, void *entries,
                   uint32_t *ids, int32_t *num, int flag)
{
    struct ipcr_name_req *req;
    uint8_t *in;
    uint64_t i;
    uint32_t count;
    int64_t found;
    int32_t ret;

    if (fd < 0)
        return -9;

    if (num != NULL) {
        count = (uint32_t)*num;
        *num = 0;
    } else {
        count = 0;
    }

    req = malloc(IPCR_REQ_HDR_SIZE + ((size_t)count * IPCR_ENTRY_STRIDE));
    if (req == NULL)
        return -12;


    const volatile uint32_t *words = (const volatile uint32_t *)name;
    uint32_t word0 = words[0];
    uint32_t word1 = words[1];

    req->word0 = word0;
    req->word1 = (flag == 1) ? 0 : word1;
    req->num = count;
    req->found = 0;
    req->flag = (flag == 1) ? 0 : 0xffffffffu;

    if (ioctl(fd, IPCR_IOCTL_LOOKUP, req) < 0) {
        free(req);
        return -errno;
    }


    i = 0;
    if ((int32_t)count >= 1) {
        found = (int32_t)req->found;
        in = (uint8_t *)req + IPCR_REQ_HDR_SIZE;

        for (; (int64_t)i < found && i != (uint64_t)count; i++) {
            const struct ipcr_name_entry *entry =
                (const struct ipcr_name_entry *)(in + (i * IPCR_ENTRY_STRIDE));
            uint8_t *out = (uint8_t *)entries + (i * IPCR_RECORD_STRIDE);

            *(uint16_t *)(out + 0x00) = 0x001b;
            *(uint8_t *)(out + 0x04) = 0x02;
            *(uint64_t *)(out + 0x08) = entry->value;

            if (ids != NULL)
                ids[i] = entry->id;
        }
    }

    if (num != NULL)
        *num = (int32_t)i;

    ret = req->found;
    free(req);
    return ret;
}
