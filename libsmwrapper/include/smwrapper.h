/*
 * Copyright (C) The LineageOS Project
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef LIB_SMWRAPPER_H
#define LIB_SMWRAPPER_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif




#define SM_MAX_USERS 10


#define SM_MAX_PHRASES 10


#define SM_EVENT_PAYLOAD_OFFSET 0x448


struct sm_user {
    uint32_t id;
    uint32_t level;
};


struct sm_phrase {
    uint32_t id;
    uint32_t field4;
    uint32_t conf;
    uint32_t num_users;
    struct sm_user users[SM_MAX_USERS];
};


struct sm_config {
    uint32_t reserved0[3];
    uint32_t num_phrases;
    struct sm_phrase phrases[];
};


struct sm_handle {
    uint32_t reserved0[11];
    uint32_t max_phrases;
    uint32_t reserved1[1];
    uint32_t field34;
};


struct sm_event_header {
    uint8_t reserved0[0x78];
    uint32_t data_size;
    uint32_t data_offset;
    uint32_t num_phrases;
    struct sm_phrase phrases[];
};


__attribute__((visibility("default")))
int generate_sound_trigger_phrase_recognition_event(void *handle, void *config,
                                                    const uint8_t *data,
                                                    uint32_t data_size,
                                                    void **event);


__attribute__((visibility("default")))
int generate_sound_trigger_phrase_recognition_event_v2(void *handle, void *config,
                                                       const uint8_t *payload,
                                                       uint32_t payload_size,
                                                       void **event);


__attribute__((visibility("default")))
int generate_sound_trigger_recognition_config_payload(void *handle, void *config,
                                                      void **tracker,
                                                      uint32_t *tracker_size);


__attribute__((visibility("default")))
int generate_sound_trigger_recognition_config_payload_v2(void *handle, void *config,
                                                         void **payload,
                                                         uint32_t *payload_size);


__attribute__((visibility("default")))
void __aeabi_memcpy(void *dest, const void *src, size_t n);
__attribute__((visibility("default")))
void __aeabi_memcpy4(void *dest, const void *src, size_t n);
__attribute__((visibility("default")))
void __aeabi_memcpy8(void *dest, const void *src, size_t n);
__attribute__((visibility("default")))
void __aeabi_memset(void *dest, size_t n, int c);
__attribute__((visibility("default")))
void __aeabi_memset4(void *dest, size_t n, int c);
__attribute__((visibility("default")))
void __aeabi_memset8(void *dest, size_t n, int c);
__attribute__((visibility("default")))
void __aeabi_memclr(void *dest, size_t n);
__attribute__((visibility("default")))
void __aeabi_memclr4(void *dest, size_t n);
__attribute__((visibility("default")))
void __aeabi_memclr8(void *dest, size_t n);

#ifdef __cplusplus
}
#endif

#endif
