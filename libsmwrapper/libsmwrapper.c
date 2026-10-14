/*
 * Copyright (C) The LineageOS Project
 *
 * SPDX-License-Identifier: Apache-2.0
 */



#define LOG_TAG "smwrapper"

#include <log/log.h>
#include <stdlib.h>
#include <string.h>

#include "smwrapper.h"


#define SM_ERR_NOMEM (-12)
#define SM_ERR_NODEV (-19)
#define SM_ERR_INVAL (-22)


_Static_assert(sizeof(struct sm_phrase) == 0x60, "phrase stride");
_Static_assert(offsetof(struct sm_phrase, conf) == 0x08, "phrase conf");
_Static_assert(offsetof(struct sm_phrase, num_users) == 0x0c, "phrase count");
_Static_assert(offsetof(struct sm_phrase, users) == 0x10, "phrase users");
_Static_assert(sizeof(struct sm_user) == 8, "user pair");
_Static_assert(offsetof(struct sm_config, num_phrases) == 0x0c, "num_phrases");
_Static_assert(offsetof(struct sm_config, phrases) == 0x10, "phrases");
_Static_assert(offsetof(struct sm_handle, max_phrases) == 0x2c, "max_phrases");
_Static_assert(offsetof(struct sm_handle, field34) == 0x34, "field34");
_Static_assert(offsetof(struct sm_event_header, data_size) == 0x78, "data_size");
_Static_assert(offsetof(struct sm_event_header, data_offset) == 0x7c, "data_offset");
_Static_assert(offsetof(struct sm_event_header, num_phrases) == 0x80, "num_phrases");
_Static_assert(offsetof(struct sm_event_header, phrases) == 0x84, "event phrases");


static int fill_sound_trigger_recognition_config_payload(const struct sm_handle *h,
                                                         const struct sm_config *cfg,
                                                         uint8_t *out, uint32_t size,
                                                         uint32_t level_count)
{
    uint8_t *seen;
    uint32_t num_phrases, i, j;

    if (h == NULL || cfg == NULL || out == NULL || size == 0) {
        ALOGE("%s: ERROR. Invalid inputs", __func__);
        return SM_ERR_INVAL;
    }

    seen = calloc(1, size);
    if (seen == NULL) {
        ALOGE("%s: failed to allocate user_id_tracker", __func__);
        return SM_ERR_NOMEM;
    }

    num_phrases = cfg->num_phrases;
    for (i = 0; i < num_phrases; i++) {
        const struct sm_phrase *ph = &cfg->phrases[i];
        uint32_t count = ph->num_users;

        out[i] = (uint8_t)ph->conf;

        for (j = 0; j < count; j++) {
            uint32_t id = ph->users[j].id;
            uint32_t level = ph->users[j].level;

            if (id < num_phrases || id >= size) {
                ALOGE("%s: ERROR. Invalid params user id %d>%d", __func__, id, level_count);
                free(seen);
                return SM_ERR_INVAL;
            }
            if (seen[id] == 1) {
                ALOGE("%s: ERROR. Duplicate user id %d", __func__, id);
                free(seen);
                return SM_ERR_INVAL;
            }

            if (level >= 100) {
                level = 100;
            }
            out[id] = (uint8_t)level;
            seen[id] = 1;
        }
    }

    free(seen);
    return 0;
}

int generate_sound_trigger_phrase_recognition_event(void *handle, void *config,
                                                    const uint8_t *data,
                                                    uint32_t data_size, void **event)
{
    const struct sm_config *cfg = config;
    const struct sm_handle *h = handle;
    struct sm_event_header *ev;
    uint32_t num_phrases, i, j;

    ALOGD("%s: Enter payload_size %d", __func__, data_size);

    if (handle == NULL || data == NULL || config == NULL || event == NULL) {
        ALOGE("%s: Null params", __func__);
        return SM_ERR_INVAL;
    }
    *event = NULL;

    ev = calloc(1, (size_t)(uint32_t)(data_size + SM_EVENT_PAYLOAD_OFFSET));
    if (ev == NULL) {
        ALOGE("%s: event allocation failed size %d", __func__, data_size);
        return SM_ERR_NODEV;
    }

    num_phrases = cfg->num_phrases;
    ev->data_size = data_size;
    ev->data_offset = SM_EVENT_PAYLOAD_OFFSET;
    ev->num_phrases = num_phrases;
    memcpy((uint8_t *)ev + SM_EVENT_PAYLOAD_OFFSET, data, data_size);


    for (i = 0; i < num_phrases; i++) {
        const struct sm_phrase *ph = &cfg->phrases[i];
        struct sm_phrase *eph = &ev->phrases[i];
        uint32_t count = ph->num_users;

        eph->id = ph->id;
        eph->field4 = h->field34;
        eph->conf = data[i];
        eph->num_users = count;

        for (j = 0; j < count; j++) {
            uint32_t id = ph->users[j].id;

            eph->users[j].id = id;
            eph->users[j].level = data[id];
        }
    }

    *event = ev;
    return 0;
}

int generate_sound_trigger_phrase_recognition_event_v2(void *handle, void *config,
                                                       const uint8_t *payload,
                                                       uint32_t payload_size, void **event)
{
    uint8_t minor_version;
    uint8_t num_active_models;

    if (payload == NULL) {
        ALOGE("%s: Null params", __func__);
        return SM_ERR_INVAL;
    }

    minor_version = payload[0];
    num_active_models = payload[1];

    ALOGD("%s: payload_size %d, minor_version = %d, num_active_models = %d", __func__,
          payload_size, minor_version, num_active_models);

    return generate_sound_trigger_phrase_recognition_event(handle, config, payload + 2,
                                                           num_active_models, event);
}

int generate_sound_trigger_recognition_config_payload(void *handle, void *config,
                                                      void **tracker, uint32_t *tracker_size)
{
    const struct sm_config *cfg = config;
    const struct sm_handle *h = handle;
    uint32_t num_phrases, max_phrases, total, i, level_count = 0;
    uint8_t *buf;
    int ret;

    if (handle == NULL || config == NULL || tracker == NULL || tracker_size == NULL) {
        ALOGE("%s: ERROR. Invalid inputs", __func__);
        return SM_ERR_INVAL;
    }
    *tracker = NULL;
    *tracker_size = 0;

    num_phrases = cfg->num_phrases;
    max_phrases = h->max_phrases;
    if (num_phrases == 0 || num_phrases > max_phrases) {
        ALOGE("%s: ERROR. Invalid phrases %d!=%d", __func__, num_phrases, max_phrases);
        return SM_ERR_INVAL;
    }

    for (i = 0; i < num_phrases; i++) {
        level_count += cfg->phrases[i].num_users;
    }
    total = level_count + num_phrases;

    buf = calloc(1, total);
    if (buf == NULL) {
        ALOGE("%s: ERROR. conf levels alloc failed", __func__);
        return SM_ERR_NOMEM;
    }

    ret = fill_sound_trigger_recognition_config_payload(h, cfg, buf, total, level_count);
    if (ret != 0) {
        ALOGE("%s: fill config payload failed, error %d", __func__, ret);
        free(buf);
        return ret;
    }

    *tracker = buf;
    *tracker_size = total;
    return 0;
}

int generate_sound_trigger_recognition_config_payload_v2(void *handle, void *config,
                                                         void **payload, uint32_t *payload_size)
{
    const struct sm_config *cfg = config;
    const struct sm_handle *h = handle;
    uint32_t num_phrases, max_phrases, total, size, i, level_count = 0;
    uint8_t *buf;
    int ret;

    if (handle == NULL || config == NULL || payload == NULL || payload_size == NULL) {
        ALOGE("%s: ERROR. Invalid inputs", __func__);
        return SM_ERR_INVAL;
    }
    *payload = NULL;
    *payload_size = 0;

    num_phrases = cfg->num_phrases;
    max_phrases = h->max_phrases;
    if (num_phrases == 0 || num_phrases > max_phrases) {
        ALOGE("%s: ERROR. Invalid phrases %d!=%d", __func__, num_phrases, max_phrases);
        return SM_ERR_INVAL;
    }

    for (i = 0; i < num_phrases; i++) {
        level_count += cfg->phrases[i].num_users;
    }
    total = level_count + num_phrases;
    size = 2u + 2u * total;

    buf = calloc(1, size);
    if (buf == NULL) {
        ALOGE("%s: ERROR. conf levels alloc failed", __func__);
        return SM_ERR_NOMEM;
    }

    buf[0] = 1;
    buf[1] = (uint8_t)total;

    ret = fill_sound_trigger_recognition_config_payload(h, cfg, buf + 2, total, level_count);
    if (ret != 0) {
        ALOGE("%s: fill config payload failed, error %d", __func__, ret);
        free(buf);
        return ret;
    }


    memset(buf + 2 + total, 1, total);

    *payload = buf;
    *payload_size = size;
    return 0;
}


#define SM_EABI_HELPERS __attribute__((target("arm")))

extern void *sm_libc_memcpy(void *dest, const void *src, size_t n) __asm__("memcpy");
extern void *sm_libc_memset(void *dest, int c, size_t n) __asm__("memset");

SM_EABI_HELPERS __attribute__((no_builtin("memcpy")))
void __aeabi_memcpy(void *dest, const void *src, size_t n)
{
    sm_libc_memcpy(dest, src, n);
}

extern __typeof(__aeabi_memcpy) __aeabi_memcpy4
        __attribute__((alias("__aeabi_memcpy"), target("arm")));
extern __typeof(__aeabi_memcpy) __aeabi_memcpy8
        __attribute__((alias("__aeabi_memcpy"), target("arm")));

SM_EABI_HELPERS __attribute__((no_builtin("memset")))
void __aeabi_memset(void *dest, size_t n, int c)
{
    sm_libc_memset(dest, c, n);
}

extern __typeof(__aeabi_memset) __aeabi_memset4
        __attribute__((alias("__aeabi_memset"), target("arm")));
extern __typeof(__aeabi_memset) __aeabi_memset8
        __attribute__((alias("__aeabi_memset"), target("arm")));

SM_EABI_HELPERS __attribute__((no_builtin("memset")))
void __aeabi_memclr(void *dest, size_t n)
{
    sm_libc_memset(dest, 0, n);
}

extern __typeof(__aeabi_memclr) __aeabi_memclr4
        __attribute__((alias("__aeabi_memclr"), target("arm")));
extern __typeof(__aeabi_memclr) __aeabi_memclr8
        __attribute__((alias("__aeabi_memclr"), target("arm")));
