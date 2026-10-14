/*
 * Copyright (C) The LineageOS Project
 *
 * SPDX-License-Identifier: Apache-2.0
 */



#ifndef ANDROID_LIBAUDIOPARSERS_H
#define ANDROID_LIBAUDIOPARSERS_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif


#define AUDIO_PARSER_CODEC_AC3 1
#define AUDIO_PARSER_CODEC_DTS 2


#define AUDIO_PARSER_AC3_MAX_FRAME 0x0f00
#define AUDIO_PARSER_DTS_MAX_FRAME 0x4000


typedef struct {
    uint32_t codec_type;
    uint32_t frame_size;
    uint32_t u08;
    uint32_t u0c;
    uint32_t u10;
} frame_info_t;


typedef struct {
    uint16_t sync_word;
    uint16_t crc1;
    uint16_t fscod;
    uint16_t frmsizecod;
    uint16_t bsid;
    uint16_t bsmod;
    uint32_t sample_rate;
    uint32_t frame_words;
    uint32_t swap_flag;
} ac3_sync_info_t;


typedef struct {
    uint32_t sync_word;
    uint16_t word4;
    uint16_t pad;
    uint32_t sample_rate;
    uint32_t nsamples;
    uint8_t rate_index;
    uint8_t pad2[3];
    uint32_t swap_flag;
} dts_sync_info_t;


extern uint16_t ac3_fr_sz_code_tbl[114];


int init_audio_parser(void *data, int size, int codec_type);
int get_first_frame_info(frame_info_t *info);
int get_audio_frame(void *dst, int dst_size, frame_info_t *info);
int get_channel_status(void *dst, int codec_type, void *unused);


void *init_audio_parser_ac3(void *data, int size);
int get_ac3_first_frame_info(frame_info_t *info);
int get_ac3_frame(void *dst, int dst_size, frame_info_t *info);
int get_ac3_frame_sync_info(const uint8_t *in, int len, ac3_sync_info_t *out);
int get_ac3_frame_info(ac3_sync_info_t *si);
int get_ac3_frame_channel_info(uint8_t *out);
int get_channel_status_ac3(void *dst);


void *init_audio_parser_dts(void *data, int size);
int get_dts_first_frame_info(frame_info_t *info);
int get_dts_frame(void *dst, int dst_size, frame_info_t *info);
int get_dts_frame_sync_info(const uint8_t *in, int len, dts_sync_info_t *out);
int get_dts_frame_info(dts_sync_info_t *info);
int get_dts_frame_channel_info(uint8_t *out);
int get_channel_status_dts(void *dst);
void *get_silent_frame(void *out);


int init_chan_status(void *dst, const uint8_t *in);
void set_bits(void *dst, int nbits, unsigned value, uint8_t *bitpos);
void *extract_bits(const uint8_t *buf, int nbits, uint16_t *out, int bitpos);


int get_iec61937_info(const int *codec_type);


void __aeabi_memcpy(void *dst, const void *src, unsigned int n);
void __aeabi_memcpy4(void *dst, const void *src, unsigned int n);
void __aeabi_memcpy8(void *dst, const void *src, unsigned int n);
void __aeabi_memset(void *dst, unsigned int n, int c);
void __aeabi_memset4(void *dst, unsigned int n, int c);
void __aeabi_memset8(void *dst, unsigned int n, int c);
void __aeabi_memclr(void *dst, unsigned int n);
void __aeabi_memclr4(void *dst, unsigned int n);
void __aeabi_memclr8(void *dst, unsigned int n);

#ifdef __cplusplus
}
#endif

#endif
