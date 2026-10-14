/*
 * Copyright (C) The LineageOS Project
 *
 * SPDX-License-Identifier: Apache-2.0
 */



#ifndef DEPTHMAPWRAPPER_H
#define DEPTHMAPWRAPPER_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif


typedef struct {
    union {
        uint32_t value32;
        uint64_t value64;
        void    *pointer;
    } value;
} WrapperDepthInput;


typedef struct {
    uint32_t type;
    uint32_t reserved;
    void    *address;
    union {
        uint32_t size;
        uint64_t size64;
    } length;
} WrapperDepthOutput;


typedef struct {
    uint64_t reserved00;
    uint64_t field08;
} WrapperDepthCamConfig;


#ifdef __cplusplus
class DepthMapWrapper {
public:
    DepthMapWrapper(int a, int b, int c, int d, unsigned long *pSize,
                    int *pStatus, unsigned char flag);
    int processFrame(WrapperDepthInput input, WrapperDepthCamConfig *config,
                     unsigned int count, WrapperDepthOutput *output);
    ~DepthMapWrapper();

    void *buffer;
};
#else
typedef struct DepthMapWrapper {
    void *buffer;
} DepthMapWrapper;
#endif


DepthMapWrapper *DepthMapWrapperCreate(int a, int b, int c, int d,
                                       unsigned long *pSize, int *pStatus,
                                       int tag);


int DepthMapWrapperProcessFrame(void *handle, WrapperDepthInput input,
                                WrapperDepthCamConfig *config,
                                unsigned int count,
                                WrapperDepthOutput *output);

void DepthMapWrapperDestroy(DepthMapWrapper *handle);

#ifdef __cplusplus
}
#endif

#endif
