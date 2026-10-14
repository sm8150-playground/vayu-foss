/*
 * Copyright (C) The LineageOS Project
 *
 * SPDX-License-Identifier: Apache-2.0
 */



#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "libaudioparsers.h"


#pragma GCC visibility push(default)


#define INTERPOSABLE __attribute__((noinline))


static uint8_t *ac3_base;
static uint32_t ac3_off;
static uint32_t ac3_len;
static uint8_t *dts_hdr;
static uint8_t *dts_base;
static uint32_t dts_off;
static uint32_t dts_len;
static uint32_t dts_frame_len;


uint16_t ac3_fr_sz_code_tbl[114] = {

    64, 64, 80, 80, 96, 96, 112, 112, 128, 128,
    160, 160, 192, 192, 224, 224, 256, 256, 320, 320,
    384, 384, 448, 448, 512, 512, 640, 640, 768, 768,
    896, 896, 1024, 1024, 1152, 1152, 1280, 1280,

    69, 70, 87, 88, 104, 105, 121, 122, 139, 140,
    174, 175, 208, 209, 243, 244, 278, 279, 348, 349,
    417, 418, 487, 488, 557, 558, 696, 697, 835, 836,
    975, 976, 1114, 1115, 1253, 1254, 1393, 1394,

    96, 96, 120, 120, 144, 144, 168, 168, 192, 192,
    240, 240, 288, 288, 336, 336, 384, 384, 480, 480,
    576, 576, 672, 672, 768, 768, 960, 960, 1152, 1152,
    1344, 1344, 1536, 1536, 1728, 1728, 1920, 1920,
};


static inline uint16_t swap16(uint16_t v)
{
    return (uint16_t)((v >> 8) | (v << 8));
}


INTERPOSABLE void set_bits(void *dst, int nbits, unsigned value, uint8_t *bitpos)
{
    while (nbits != 0) {
        unsigned pos = *bitpos;
        unsigned avail = 8 - (pos & 7);
        unsigned remaining = (uint8_t)nbits;
        unsigned fill = avail < remaining;
        unsigned chunk;
        uint8_t mask;

        if (fill)
            nbits = (int)avail;
        chunk = (uint8_t)nbits;
        mask = fill ? (uint8_t)(0xFFu << chunk)
                    : (uint8_t)(0xFFu >> chunk);
        {
            unsigned left = remaining - chunk;
            unsigned align = avail - chunk;
            unsigned bits = (value >> left) << align;
            unsigned idx = pos >> 3;
            uint8_t byte = ((uint8_t *)dst)[idx];

            byte = (uint8_t)((byte & mask) | bits);
            ((uint8_t *)dst)[idx] = byte;
            *bitpos = (uint8_t)(pos + chunk);
            nbits = (int)left;
        }
    }
}


void *extract_bits(const uint8_t *buf, int nbits, uint16_t *out, int bitpos)
{
    uint32_t acc = 0;

    while (nbits != 0) {
        unsigned pos = (unsigned)bitpos;
        unsigned avail = 8 - (pos & 7);
        unsigned remaining = (uint8_t)nbits;
        unsigned chunk;
        uint8_t byte;

        if (avail < remaining)
            remaining = avail;
        chunk = remaining;
        byte = buf[pos >> 3];
        byte &= (uint8_t)~(0xFFu << avail);
        acc = (acc << chunk) | (uint32_t)(byte >> (avail - chunk));
        nbits -= (int)chunk;
        bitpos += (int)chunk;
    }
    *out = (uint16_t)acc;
    return (void *)(uintptr_t)buf;
}


static uint32_t bits_extract(const uint8_t *buf, int bitpos, int nbits)
{
    uint32_t acc = 0;

    while (nbits != 0) {
        unsigned pos = (unsigned)bitpos;
        unsigned avail = 8 - (pos & 7);
        unsigned remaining = (uint8_t)nbits;
        unsigned chunk = (avail < remaining) ? avail : remaining;
        uint8_t byte = buf[pos >> 3];

        byte &= (uint8_t)~(0xFFu << avail);
        acc = (acc << chunk) | (uint32_t)(byte >> (avail - chunk));
        nbits -= (int)chunk;
        bitpos += (int)chunk;
    }
    return acc;
}


int get_iec61937_info(const int *codec_type)
{
    int type = *codec_type;

    if (type == 3) {

        fprintf(stderr, "error %d for codec type %u\n", -38, 3);
        return -22;
    }
    fprintf(stderr, "%s un supported code type %u\n", "get_iec61937_info", type);
    return -22;
}


int get_audio_frame(void *dst, int dst_size, frame_info_t *info)
{
    switch (info->codec_type) {
    case 1:
        return get_ac3_frame(dst, dst_size, info);
    case 2:
        return get_dts_frame(dst, dst_size, info);
    default:
        fprintf(stderr, "%s un supported code type %u\n", "get_audio_frame",
                info->codec_type);
        return -1;
    }
}


int get_first_frame_info(frame_info_t *info)
{
    switch (info->codec_type) {
    case 1:
        get_ac3_first_frame_info(info);
        return 0;
    case 2:
        get_dts_first_frame_info(info);
        return 0;
    default:

        fprintf(stderr, "Unsupported codec type %u\n", info->codec_type);
        return -22;
    }
}


int get_channel_status(void *dst, int codec_type, void *unused)
{
    (void)unused;

    switch (codec_type) {
    case 2:
        get_channel_status_dts(dst);
        return 0;
    case 1:
        get_channel_status_ac3(dst);
        return 0;
    default:
        fprintf(stderr, "%s un supported code type %u\n", "get_channel_status",
                codec_type);
        return -1;
    }
}


int init_audio_parser(void *data, int size, int codec_type)
{
    switch (codec_type) {
    case 3:
        return 0x9000;
    case 2:
        init_audio_parser_dts(data, size);
        return 0x4000;
    case 1:
        init_audio_parser_ac3(data, size);
        return 0xf00;
    default:
        fprintf(stderr, "%s un supported code type %u\n", "init_audio_parser",
                codec_type);
        return -1;
    }
}


INTERPOSABLE int get_ac3_frame_sync_info(const uint8_t *in, int len, ac3_sync_info_t *out)
{
    uint32_t reverse;
    uint16_t sync;
    uint16_t word;
    uint8_t b2, b3, b4, b5;

    if (len != 6) {
        fwrite("invalid syncinfo size\n", 22, 1, stderr);
        return -1;
    }

    sync = swap16((uint16_t)(in[0] | (in[1] << 8)));
    out->sync_word = sync;

    if (sync == 0x0B77) {
        reverse = 0;
        out->swap_flag = 1;
    } else if (sync == 0x770B) {
        reverse = 1;
        out->swap_flag = 0;
    } else {
        fprintf(stderr, "invalid syncinfo word 0x%04x\n", sync);
        return -1;
    }

    b2 = in[2];
    b3 = in[3];
    word = (uint16_t)(b3 | (b2 << 8));
    if (reverse)
        word = (uint16_t)(b2 | (b3 << 8));
    out->crc1 = word;

    b4 = in[4];
    b5 = in[5];
    word = (uint16_t)(b5 | (b4 << 8));
    if (reverse)
        word = (uint16_t)(b4 | (b5 << 8));

    out->bsmod = word & 0x7;
    out->bsid = (word >> 3) & 0x1F;
    out->fscod = word >> 14;
    out->frmsizecod = (word >> 8) & 0x3F;


    fprintf(stderr,
            "sync word 0x%04x fscod 0x%02x frmsizcod 0x%02x bsid 0x%02x bsmod = 0x%02x\n",
            sync, out->fscod, out->frmsizecod, out->bsid, out->bsmod);

    if (out->fscod >= 3) {
        fprintf(stderr, "invalid fscod 0x%02x\n", out->fscod);
        return -1;
    }
    if (out->frmsizecod >= 0x26) {
        fprintf(stderr, "invalid frmsizcod 0x%02x\n", out->frmsizecod);
        return -1;
    }
    if (out->bsid >= 9) {
        fprintf(stderr, "invalid bsid 0x%02x\n", out->bsid);
        return -1;
    }
    if (out->bsmod < 8)
        return 0;
    fprintf(stderr, "invalid bsmod 0x%02x\n", out->bsmod);
    return -1;
}


int get_ac3_frame_info(ac3_sync_info_t *si)
{
    uint32_t rate;

    switch (si->fscod) {
    case 0:
        rate = 48000;
        break;
    case 1:
        rate = 44100;
        break;
    case 2:
        rate = 32000;
        break;
    default:
        si->sample_rate = 0xFFFFFFFFu;
        fprintf(stderr, "invalid fscod 0x%02x\n", si->fscod);
        return -1;
    }

    si->sample_rate = rate;
    si->frame_words = ac3_fr_sz_code_tbl[si->fscod * 38 + si->frmsizecod];
    fprintf(stderr, "sample rate %d  ac3_frame_size in 16 bit words %d\n",
            rate, si->frame_words);
    return 0;
}


INTERPOSABLE int get_ac3_first_frame_info(frame_info_t *info)
{
    ac3_sync_info_t si;
    uint8_t sync[8];
    uint32_t word;
    uint32_t rate;

    memset(&si, 0, sizeof(si));

    word = *(const uint32_t *)(const void *)ac3_base;
    *(uint32_t *)(void *)sync = word;
    *(uint16_t *)(void *)(sync + 4) =
        *(const uint16_t *)(const void *)(ac3_base + 4);

    if (get_ac3_frame_sync_info(sync, 6, &si) == -1)
        return -1;

    switch (si.fscod) {
    case 0:
        rate = 48000;
        break;
    case 1:
        rate = 44100;
        break;
    case 2:
        rate = 32000;
        break;
    default:
        si.sample_rate = 0xFFFFFFFFu;
        fprintf(stderr, "invalid fscod 0x%02x\n", si.fscod);
        return -1;
    }

    si.sample_rate = rate;
    si.frame_words = ac3_fr_sz_code_tbl[si.fscod * 38 + si.frmsizecod];
    fprintf(stderr, "sample rate %d  ac3_frame_size in 16 bit words %d\n",
            rate, si.frame_words);

    info->u0c = rate;
    return 0;
}


INTERPOSABLE int get_ac3_frame(void *dst, int dst_size, frame_info_t *info)
{
    ac3_sync_info_t si;
    uint8_t sync[8];
    uint32_t word;
    uint32_t rate;
    uint32_t frame_words;

    memset(&si, 0, sizeof(si));
    __aeabi_memclr(dst, (size_t)dst_size);

    if (ac3_off >= ac3_len)
        return -1;

    word = *(const uint32_t *)(const void *)(ac3_base + ac3_off);
    *(uint32_t *)(void *)sync = word;
    *(uint16_t *)(void *)(sync + 4) =
        *(const uint16_t *)(const void *)(ac3_base + ac3_off + 4);

    if (get_ac3_frame_sync_info(sync, 6, &si) == -1)
        return -1;

    switch (si.fscod) {
    case 0:
        rate = 48000;
        break;
    case 1:
        rate = 44100;
        break;
    case 2:
        rate = 32000;
        break;
    default:
        fprintf(stderr, "invalid fscod 0x%02x\n", si.fscod);
        return -1;
    }

    si.sample_rate = rate;
    frame_words = ac3_fr_sz_code_tbl[si.fscod * 38 + si.frmsizecod];
    si.frame_words = frame_words;
    fprintf(stderr, "sample rate %d  ac3_frame_size in 16 bit words %d\n",
            rate, frame_words);


    info->codec_type = 1;
    info->frame_size = frame_words;
    ((uint8_t *)info)[8] = (uint8_t)si.bsmod;
    info->u0c = rate;
    info->u10 = si.swap_flag;

    if (((uint32_t)dst_size >> 1) - 64 > 0x740) {
        fprintf(stderr, "invalid ac3 frame size %u\n", (uint32_t)dst_size >> 1);
        goto no_frame;
    }

    if (ac3_off >= ac3_len) {
        fprintf(stderr, "%s : end of file. audio_data_index  %u\n",
                "read_ac3_frame", ac3_off);
        goto no_frame;
    }

    memcpy(dst, ac3_base + ac3_off, (size_t)frame_words * 2);
    ac3_off += frame_words * 2;
    return 0;

no_frame:
    fprintf(stderr, "%s No frame. errors\n", "get_ac3_frame");
    return -1;
}


INTERPOSABLE int init_chan_status(void *dst, const uint8_t *in)
{
    uint8_t bitpos = 0;

    set_bits(dst, 1, 1, &bitpos);

    bitpos = 1;
    if (in[0] == 1)
        set_bits(dst, 1, 1, &bitpos);

    bitpos = 2;
    set_bits(dst, 1, in[1], &bitpos);

    bitpos = 0x18;

    if (in[6] == 0 || in[6] == 2)
        set_bits(dst, 4, (in[6] == 0) ? 4 : 0xC, &bitpos);
    else if (in[6] != 1)
        fprintf(stderr, "%s Invalid sample_rate %u\n", "init_chan_status",
                in[6]);

    bitpos = 0x28;
    set_bits(dst, 2, in[0xA], &bitpos);
    return 0;
}


int get_ac3_frame_channel_info(uint8_t *out)
{
    const uint8_t *f = ac3_base;
    uint32_t normal;
    uint16_t sync;
    uint8_t b;

    out[0] = 1;

    sync = swap16((uint16_t)(f[0] | (f[1] << 8)));
    if (sync == 0x0B77) {
        normal = 1;
    } else if (sync == 0x770B) {
        normal = 0;
    } else {
        fprintf(stderr, "invalid sync_word 0x%04x\n", sync);
        return -1;
    }

    out[6] = f[normal ? 4 : 5] >> 6;

    b = f[normal ? 6 : 7];
    if (b > 0x1F) {
        unsigned bit = (f[normal ? 0xA : 0xB] >> 6) & 1;

        out[1] = (uint8_t)(bit ^ 1);
        out[0xA] = bit ? 3 : 0;
    } else {
        unsigned bit = f[normal ? 0xE : 0xF] >> 7;

        out[1] = (uint8_t)(bit ^ 1);
        out[0xA] = bit ? 3 : 0;
    }
    return 0;
}


INTERPOSABLE int get_channel_status_ac3(void *dst)
{
    uint8_t st[11];
    const uint8_t *f = ac3_base;
    uint32_t normal;
    uint16_t sync;
    uint8_t b;

    memset(st, 0, sizeof(st));
    st[0] = 1;

    sync = swap16((uint16_t)(f[0] | (f[1] << 8)));
    if (sync == 0x0B77) {
        normal = 1;
    } else if (sync == 0x770B) {
        normal = 0;
    } else {
        fprintf(stderr, "invalid sync_word 0x%04x\n", sync);
        return -1;
    }

    b = f[normal ? 4 : 5];
    st[6] = b >> 6;

    b = f[normal ? 6 : 7];
    if (b > 0x1F) {
        unsigned bit = (f[normal ? 0xA : 0xB] >> 6) & 1;

        st[0xA] = bit ? 3 : 0;
        st[1] = (uint8_t)(bit ^ 1);
    } else {
        unsigned bit = f[normal ? 0xE : 0xF] >> 7;

        st[0xA] = bit ? 3 : 0;
        st[1] = (uint8_t)(bit ^ 1);
    }

    init_chan_status(dst, st);
    return 0;
}


INTERPOSABLE void *init_audio_parser_ac3(void *data, int size)
{
    ac3_base = (uint8_t *)data;
    ac3_off = 0;
    ac3_len = (uint32_t)size;
    return data;
}


int get_dts_frame_sync_info(const uint8_t *in, int len, dts_sync_info_t *out)
{
    uint16_t sync;
    uint32_t word;
    uint32_t swapped;

    if (len != 5)
        return -1;

    sync = swap16((uint16_t)(in[0] | (in[1] << 8)));

    if (sync == 0x7FFE) {
        out->swap_flag = 1;
        word = *(const uint32_t *)(const void *)in;
        out->sync_word = word;
        out->word4 = (uint16_t)(in[4] >> 4);
        if (word == 0x0180FE7Fu)
            return 0;
        fprintf(stderr, "sync word not matching %x \n", word);
        return -1;
    }

    if (sync == 0xFE7F) {
        word = *(const uint32_t *)(const void *)in;
        swapped = ((word >> 8) & 0xFF00u) | (word >> 24);
        out->swap_flag = 0;
        out->sync_word = swapped;
        out->word4 = (uint16_t)(in[5] >> 4);
        fprintf(stderr, "sync word not matching %x \n", swapped);
        return -1;
    }


    fprintf(stderr, "invalid syncinfo word 0x%04x\n",
            *(const uint32_t *)(const void *)out);
    return -1;
}


INTERPOSABLE int get_dts_frame_info(dts_sync_info_t *info)
{
    uint8_t *hdr;
    uint16_t sync;
    uint32_t word;
    uint32_t idx;
    uint32_t rate;
    uint32_t acc;

    hdr = (uint8_t *)malloc(12);
    dts_hdr = hdr;
    if (!hdr) {
        fwrite("Failed to allocate memory for frame hdr\n", 40, 1, stderr);
        return -1;
    }
    memset(hdr, 0x30, 12);

    word = *(const uint32_t *)(const void *)dts_base;
    sync = swap16((uint16_t)word);

    if (sync == 0xFE7F) {
        info->swap_flag = 1;
        goto swapped;
    }
    if (sync == 0x7FFE) {
        info->swap_flag = 0;
        goto raw;
    }


    if (info->swap_flag == 0)
        goto raw;


swapped:
    for (unsigned i = 0; i < 6; i++) {
        uint16_t w = *(const uint16_t *)(const void *)(dts_base + dts_off + 4 + 2 * i);

        ((uint16_t *)(void *)hdr)[i] = swap16(w);
    }
    idx = (((uint16_t *)(void *)hdr)[2] >> 2) & 0xF;
    goto have_index;

raw:
    memcpy(hdr, dts_base + dts_off + 4, 11);
    idx = (hdr[4] >> 2) & 0xF;

have_index:
    switch (idx) {
    case 1:
        rate = 8000;
        break;
    case 2:
        rate = 16000;
        break;
    case 3:
        rate = 32000;
        break;
    case 6:
        rate = 11025;
        break;
    case 7:
        rate = 22050;
        break;
    case 8:
        rate = 44100;
        break;
    case 11:
        rate = 12000;
        break;
    case 12:
        rate = 24000;
        break;
    case 13:
        rate = 48000;
        break;
    default:
        fprintf(stderr, "Invalid sample rate %d\n", idx);
        return -1;
    }
    info->sample_rate = rate;

    acc = bits_extract(hdr, 7, 7);
    switch ((acc << 5) & 0x1FFFE0u) {
    case 0x7E0:
        ((uint8_t *)info)[0x10] = 13;
        break;
    case 0x3E0:
        ((uint8_t *)info)[0x10] = 12;
        break;
    case 0x1E0:
        ((uint8_t *)info)[0x10] = 11;
        break;
    default:
        fwrite("Invalid DTS type\n", 17, 1, stderr);
        break;
    }

    acc = bits_extract(hdr, 14, 14);
    info->nsamples = 1 + acc;

    free(hdr);
    return 0;
}


void *get_silent_frame(void *out_)
{
    uint8_t *out = (uint8_t *)out_;
    int bitpos = 0x2E;
    int nbits = 0xE;

    memcpy(out, dts_base + dts_off, 11);

    while (nbits != 0) {
        unsigned avail = 8 - ((unsigned)bitpos & 7);
        unsigned remaining = (uint8_t)nbits;
        unsigned fill = avail < remaining;
        unsigned chunk = fill ? avail : remaining;
        uint8_t mask = fill ? (uint8_t)(0xFFu << chunk)
                            : (uint8_t)(0xFFu >> chunk);

        out[(unsigned)bitpos >> 3] &= mask;
        nbits -= (int)chunk;
        bitpos += (int)chunk;
    }
    return out_;
}


INTERPOSABLE int get_dts_first_frame_info(frame_info_t *info)
{
    dts_sync_info_t si;
    uint32_t word;
    uint16_t sync;

    memset(&si, 0, sizeof(si));

    word = *(const uint32_t *)(const void *)dts_base;
    sync = swap16((uint16_t)word);

    if (sync == 0x7FFE) {
        si.swap_flag = 1;
        si.sync_word = word;
        si.word4 = (uint16_t)(dts_base[4] >> 4);
        if (word != 0x0180FE7Fu) {
            fprintf(stderr, "sync word not matching %x \n", word);
            goto fail;
        }
        if (get_dts_frame_info(&si) == -1)
            return -1;
        info->u08 = si.sample_rate;
        ((uint8_t *)info)[0x0C] = ((uint8_t *)&si)[0x10];
        return 0;
    }

    if (sync == 0xFE7F) {
        word = *(const uint32_t *)(const void *)dts_base;
        si.word4 = 0;
        si.swap_flag = 0;
        si.sync_word = ((word >> 8) & 0xFF00u) | (word >> 24);
        fprintf(stderr, "sync word not matching %x \n", si.sync_word);
    } else {
        fprintf(stderr, "invalid syncinfo word 0x%04x\n", 0);
    }

fail:
    fprintf(stderr, "return val of sync_info %d \n", -1);
    return -1;
}


INTERPOSABLE int get_dts_frame(void *dst, int dst_size, frame_info_t *info)
{
    dts_sync_info_t si;
    uint32_t word;
    uint16_t sync;
    uint32_t len;

    memset(&si, 0, sizeof(si));
    __aeabi_memclr(dst, (size_t)dst_size);

    if (dts_off >= dts_len) {
        fprintf(stderr, "audio_data_index %d audio_data_sz %d ",
                (int)dts_off, (int)dts_len);
        return -1;
    }

    word = *(const uint32_t *)(const void *)(dts_base + dts_off);
    sync = swap16((uint16_t)word);

    if (sync == 0x7FFE) {
        si.swap_flag = 1;
        si.sync_word = word;
        si.word4 = (uint16_t)(dts_base[dts_off + 4] >> 4);
        if (word != 0x0180FE7Fu) {
            fprintf(stderr, "sync word not matching %x \n", word);
            goto sync_info_fail;
        }
        if (get_dts_frame_info(&si) == -1)
            return -1;
    } else if (sync == 0xFE7F) {
        word = *(const uint32_t *)(const void *)(dts_base + dts_off);
        si.word4 = 0;
        si.swap_flag = 0;
        si.sync_word = ((word >> 8) & 0xFF00u) | (word >> 24);
        fprintf(stderr, "sync word not matching %x \n", si.sync_word);
        goto sync_info_fail;
    } else {
        fprintf(stderr, "invalid syncinfo word 0x%04x\n", 0);
        goto sync_info_fail;
    }


    info->codec_type = 2;
    info->frame_size = si.nsamples;
    info->u08 = si.sample_rate;
    ((uint8_t *)info)[0x0C] = ((uint8_t *)&si)[0x10];
    info->u10 = (si.swap_flag == 0) ? 1 : 0;

    if (((uint32_t)dst_size >> 1) - 256 > 0x1F00) {
        fprintf(stderr, "invalid dts frame size %u\n", (uint32_t)dst_size >> 1);
        goto no_frame;
    }

    if (dts_off >= dts_len) {
        fprintf(stderr, "%s : end of file. audio_data_index  %u\n",
                "read_dts_frame", dts_off);
        goto no_frame;
    }

    len = si.nsamples;
    memcpy(dst, dts_base + dts_off, (size_t)len);

    if (dts_off == 0) {
        if (*(const uint16_t *)(const void *)(dts_base + len) == 0xFE7F &&
            *(const uint16_t *)(const void *)(dts_base + len + 2) == 0x0180) {
            dts_frame_len = len;
        } else {
            const uint8_t *start = dts_base + (len & ~1u);
            const uint8_t *p = start - 2;
            uint32_t delta;

            do {
                p += 2;
            } while (*(const uint16_t *)(const void *)p != 0xFE7F);

            if (*(const uint16_t *)(const void *)(p + 2) == 0x0180) {
                delta = (uint32_t)(p - start);
                if (delta != 0)
                    dts_frame_len = delta + len - 1;
            }
        }
    }

    dts_off += dts_frame_len;
    return 0;

sync_info_fail:
    fprintf(stderr, "return val of sync_info %d \n", -1);
    return -1;

no_frame:
    fprintf(stderr, "%s No frame. errors\n", "get_dts_frame");
    return -1;
}


INTERPOSABLE int get_dts_frame_channel_info(uint8_t *out)
{
    uint8_t *hdr;
    uint16_t sync;
    uint32_t word;
    uint32_t val;

    out[0] = 1;

    hdr = (uint8_t *)malloc(12);
    dts_hdr = hdr;
    if (!hdr) {
        fwrite("Failed to allocate memory for frame hdr\n", 40, 1, stderr);
        return -1;
    }
    memset(hdr, 0x30, 12);

    word = *(const uint32_t *)(const void *)dts_base;
    sync = swap16((uint16_t)word);

    if (sync == 0xFE7F) {
        for (unsigned i = 0; i < 6; i++) {
            uint16_t w = *(const uint16_t *)(const void *)(dts_base + dts_off + 4 + 2 * i);

            w = swap16(w);
            ((uint16_t *)(void *)hdr)[i] = w;
            if (i == 2)
                val = w;
        }
    } else {
        memcpy(hdr, dts_base + dts_off + 4, 11);
        val = hdr[4];
    }

    out[6] = (uint8_t)((val >> 2) & 0xF);

    out[9] = (uint8_t)bits_extract(hdr, 38, 5);

    free(hdr);
    return 0;
}


INTERPOSABLE int get_channel_status_dts(void *dst)
{
    uint8_t st[11];
    uint8_t old;
    uint8_t idx;

    memset(st, 0, sizeof(st));

    if (get_dts_frame_channel_info(st) == -1)
        return 0;

    old = ((uint8_t *)dst)[0];
    ((uint8_t *)dst)[0] = (uint8_t)(old | 0x80);
    if (st[0] == 1) {
        ((uint8_t *)dst)[0] = (uint8_t)((old & 0x3F) | 0x40);
    }

    idx = st[6];
    if (idx == 3) {
        ((uint8_t *)dst)[3] = (uint8_t)((((uint8_t *)dst)[3] & 0x0F) | 0xC0);
    } else if (idx == 8) {
        ;
    } else if (idx == 0xD) {
        ((uint8_t *)dst)[3] = (uint8_t)((((uint8_t *)dst)[3] & 0x0F) | 0x40);
    } else {
        fprintf(stderr, "%s Invalid sample_rate %u\n", "init_chan_status", idx);
    }

    return 0;
}


INTERPOSABLE void *init_audio_parser_dts(void *data, int size)
{
    dts_base = (uint8_t *)data;
    dts_off = 0;
    dts_len = (uint32_t)size;
    return data;
}
