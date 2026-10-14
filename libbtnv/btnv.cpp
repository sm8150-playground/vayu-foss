/*
 * Copyright (C) The LineageOS Project
 *
 * SPDX-License-Identifier: Apache-2.0
 */



#include <fcntl.h>
#include <grp.h>
#include <pwd.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include "btnv.h"

#define BT_NV_DIR "/mnt/vendor/persist/bluetooth"
#define BT_NV_FILE ".bt_nv.bin"
#define BT_NV_OWNER "bluetooth"

#define NV_RECORD_STRIDE 16
#define NV_PATH_SIZE 255
#define NV_ITEM_COUNT 3

struct nv_item {
    uint32_t type;
    uint8_t len;
    uint8_t reserved[3];
    uint32_t value;
};


static const struct nv_item nv_items[NV_ITEM_COUNT] = {
    { 1, 6, { 0, 0, 0 }, 1 },
    { 2, 1, { 0, 0, 0 }, 0 },
    { 3, 1, { 0, 0, 0 }, 0 },
};

static void bt_nv_path(char *out)
{
    snprintf(out, NV_PATH_SIZE, "%s/%s", BT_NV_DIR, BT_NV_FILE);
}

int bt_nv_init(void)
{
    return 0;
}

int bt_nv_deinit(void)
{
    return 0;
}

int bt_nv_write(struct nv_persist_params *params, unsigned char count)
{
    char path[NV_PATH_SIZE];
    unsigned char record[0x103];
    struct passwd *pw;
    struct group *gr;
    int fd;
    long written = -1;
    unsigned int i;

    bt_nv_path(path);

    umask(066);
    fd = open(path, O_RDWR | O_CREAT, 0666);
    if (fd < 0)
        return -1;

    pw = getpwnam(BT_NV_OWNER);
    if (pw == NULL)
        goto fail;
    gr = getgrnam(BT_NV_OWNER);
    if (gr == NULL)
        goto fail;
    if (fchown(fd, pw->pw_uid, gr->gr_gid) < 0)
        goto fail;

    if ((count & 0xff) == 0) {
        written = 0;
        goto out;
    }
    if (params == NULL)
        goto fail;

    for (i = 0; i < (count & 0xff); i++) {
        const struct nv_persist_params *p =
            (const struct nv_persist_params *)
                ((const char *)params + i * NV_RECORD_STRIDE);

        record[0] = (unsigned char)(p->type & 0xff);
        record[1] = (unsigned char)(p->field4 & 0xff);
        record[2] = p->len;
        memcpy(record + 3, p->data, p->len);

        written = write(fd, record, (long)p->len + 3);
        if (written < 1)
            goto fail;
    }

out:
    close(fd);
    return (int)written;

fail:
    written = -1;
    goto out;
}

int bt_nv_read(struct nv_persist_params *params)
{
    char path[NV_PATH_SIZE];
    unsigned char payload[0x100];
    unsigned char header[3];
    int fd;
    int got;
    int count = 0;

    bt_nv_path(path);

    fd = open(path, O_RDONLY);
    if (fd < 0)
        return -1;

    if (read(fd, header, 3) != 3) {
        count = 0;
        goto out;
    }

    for (;;) {
        params->type = header[0];
        params->field4 = header[1];
        params->len = header[2];

        got = read(fd, payload, header[2]);
        if (header[2] != 0 && got == header[2])
            memcpy(params->data, payload, (size_t)got);

        got = read(fd, header, 3);
        params = (struct nv_persist_params *)((char *)params +
                                              NV_RECORD_STRIDE);
        count++;
        if (got != 3)
            break;
    }

out:
    close(fd);
    return count;
}

extern "C" int bt_nv_is_random_addr(void)
{
    struct nv_persist_params params[3];
    int count;
    int i;

    memset(params, 0, sizeof(params));
    count = bt_nv_read(params);
    if (count < 1)
        return 0;

    for (i = 0; i < count; i++) {
        if (params[i].type == 1)
            return params[i].field4 == 0;
    }

    return 0;
}

extern "C" int bt_nv_cmd(int cmd, int type, void *out, int arg)
{
    struct nv_persist_params params[3];
    int count;
    int index = -1;
    int i;

    memset(params, 0, sizeof(params));
    count = bt_nv_read(params);

    if (count == -1) {
        if (cmd == 0)
            return 1;
        count = 0;
    }

    if ((unsigned int)(type - 1) > 2)
        return 1;

    for (i = 0; i < count; i++) {
        if (params[i].type == (uint32_t)type) {
            index = i;
            break;
        }
    }

    if (cmd == 1) {
        if (index >= 0 && params[index].field4 != 0) {

            if (count > 3)
                return 1;
            if (bt_nv_write(params, (unsigned char)count) == -1)
                return 1;
            return 3;
        }

        if (index < 0) {
            index = count;
            count = count + 1;
        }

        params[index].type = (uint32_t)type;
        params[index].field4 = nv_items[type - 1].value;
        params[index].len = nv_items[type - 1].len;

        if (type == 1) {
            if (arg != 0)
                params[index].field4 = 0;
            memcpy(params[index].data, out, 6);
        } else {
            params[index].data[0] = *(const unsigned char *)out;
        }

        if (count > 3)
            return 1;
        if (bt_nv_write(params, (unsigned char)count) == -1)
            return 1;
        return 0;
    }

    if (cmd != 0)
        return 2;
    if (index < 0)
        return 1;

    if (type == 1) {
        memcpy(out, params[index].data, 4);
        memcpy((char *)out + 4, params[index].data + 4, 2);
    } else {
        *(uint64_t *)out = params[index].data[0];
    }
    return 0;
}
