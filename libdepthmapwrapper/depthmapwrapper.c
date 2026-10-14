/*
 * Copyright (C) The LineageOS Project
 *
 * SPDX-License-Identifier: Apache-2.0
 */



#include <stdint.h>
#include <stdlib.h>

#include "depthmapwrapper.h"


#define DMP_BUFFER_SIZE  13000000UL
#define DMP_CONFIG_VALUE 0x00e4e3ebUL


#define DMP_ENTRY_LENGTH32_A 1
#define DMP_ENTRY_LENGTH32_B 2
#define DMP_ENTRY_LENGTH64   3


__attribute__((visibility("default")))
void *_ZN15DepthMapWrapperC2EiiiiPmPih(DepthMapWrapper *self, int a, int b,
                                       int c, int d, unsigned long *pSize,
                                       int *pStatus, unsigned char flag)
{
    (void)a;
    (void)b;
    (void)c;
    (void)d;
    (void)flag;

    void *buffer = malloc(1);

    self->buffer = buffer;
    __asm__ volatile("" ::: "memory");
    *pStatus = 0;
    *pSize = DMP_BUFFER_SIZE;

    return buffer;
}

extern __typeof(_ZN15DepthMapWrapperC2EiiiiPmPih) _ZN15DepthMapWrapperC1EiiiiPmPih
    __attribute__((alias("_ZN15DepthMapWrapperC2EiiiiPmPih"), visibility("default")));


__attribute__((visibility("default")))
void _ZN15DepthMapWrapperD2Ev(DepthMapWrapper *self)
{
    if (self->buffer != NULL) {
        free(self->buffer);
        self->buffer = NULL;
    }
}

extern __typeof(_ZN15DepthMapWrapperD2Ev) _ZN15DepthMapWrapperD1Ev
    __attribute__((alias("_ZN15DepthMapWrapperD2Ev"), visibility("default")));


static int dmp_process_frame(DepthMapWrapper *self, WrapperDepthCamConfig *config,
                             unsigned int count, WrapperDepthOutput *output)
{
    unsigned int i;
    int valid = 1;

    if (output == NULL || count == 0 || config == NULL || self->buffer == NULL)
        return 0;

    for (i = 0; i < count; i++) {
        switch (output[i].type) {
        case DMP_ENTRY_LENGTH32_A:
        case DMP_ENTRY_LENGTH32_B:
            if (output[i].address == NULL || output[i].length.size == 0)
                valid = 0;
            break;
        case DMP_ENTRY_LENGTH64:
            if (output[i].address == NULL || output[i].length.size64 == 0)
                valid = 0;
            break;
        default:
            valid = 0;
            break;
        }
    }

    config->field08 = DMP_CONFIG_VALUE;

    return valid;
}

__attribute__((visibility("default")))
int _ZN15DepthMapWrapper12processFrameE17WrapperDepthInputP21WrapperDepthCamConfigjP18WrapperDepthOutput(
    DepthMapWrapper *self, WrapperDepthInput input, WrapperDepthCamConfig *config,
    unsigned int count, WrapperDepthOutput *output)
{
    (void)input;

    return dmp_process_frame(self, config, count, output);
}

__attribute__((visibility("default")))
DepthMapWrapper *DepthMapWrapperCreate(int a, int b, int c, int d,
                                       unsigned long *pSize, int *pStatus,
                                       int tag)
{
    DepthMapWrapper *self;
    void *buffer;

    (void)a;
    (void)b;
    (void)c;
    (void)d;
    (void)tag;

    self = malloc(sizeof(*self));
    buffer = malloc(1);

    *pStatus = 0;
    self->buffer = buffer;
    *pSize = DMP_BUFFER_SIZE;

    return self;
}

__attribute__((visibility("default")))
int DepthMapWrapperProcessFrame(void *handle, WrapperDepthInput input,
                                WrapperDepthCamConfig *config,
                                unsigned int count,
                                WrapperDepthOutput *output)
{
    (void)input;

    return dmp_process_frame(handle, config, count, output);
}

__attribute__((visibility("default")))
void DepthMapWrapperDestroy(DepthMapWrapper *handle)
{
    if (handle == NULL)
        return;

    if (handle->buffer != NULL)
        free(handle->buffer);

    free(handle);
}
