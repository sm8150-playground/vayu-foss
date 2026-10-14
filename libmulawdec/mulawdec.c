/*
 * Copyright (C) The LineageOS Project
 *
 * SPDX-License-Identifier: Apache-2.0
 */



#include "mulawdec.h"


#define MULAW_FULL_SCALE 0x1F5F


const uint16_t INDEX_OFFSET[MULAW_SEGMENTS] = {
    0x0070, 0x0060, 0x0050, 0x0040, 0x0030, 0x0020, 0x0010, 0x0001,
};

const uint16_t INTERVAL_SIZE[MULAW_SEGMENTS] = {
    8, 7, 6, 5, 4, 3, 2, 1,
};

const uint16_t MID_OFFSET[MULAW_SEGMENTS] = {
    0x0080, 0x0040, 0x0020, 0x0010, 0x0008, 0x0004, 0x0002, 0x0001,
};

const uint16_t SAMP_OFFSET[MULAW_SEGMENTS] = {
    0x0FDF, 0x07DF, 0x03DF, 0x01DF, 0x00DF, 0x005F, 0x001F, 0x0001,
};

int mulaw_dec_process(short *out_buf, char *in_buf, unsigned int in_bytes)
{
    unsigned int i;

    for (i = 0; i < in_bytes; i++) {
        const uint8_t octet = (uint8_t)in_buf[i];
        const uint8_t x = (uint8_t)(~octet) & 0x7F;
        int32_t magnitude;

        if (x == 0) {
            magnitude = 0;
        } else if (x == 0x7F) {
            magnitude = MULAW_FULL_SCALE;
        } else {
            const uint8_t segment = 7 - (x >> 4);

            magnitude = MID_OFFSET[segment] + SAMP_OFFSET[segment] +
                        ((int32_t)(x - INDEX_OFFSET[segment])
                         << INTERVAL_SIZE[segment]);
        }

        if (!(octet & 0x80))
            magnitude = -magnitude;

        out_buf[i] = (short)(magnitude << 2);
    }

    return 0;
}
