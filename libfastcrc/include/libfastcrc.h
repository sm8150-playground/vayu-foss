/*
 * Copyright (C) The LineageOS Project
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef LIB_FASTCRC_H
#define LIB_FASTCRC_H

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif


struct VideoCRCChecker;
struct MisrFrame;
struct CrcFrame;


struct MisrFrame;


struct CrcFrame;

#define FASTCRC_CHECKER_BYTES 0x580


__attribute__((visibility("default")))
int VideoCRCCheckerInit(struct VideoCRCChecker *chk, int writeMode, int mode);


__attribute__((visibility("default")))
int VideoCRCCheckerConfigure(struct VideoCRCChecker *chk, const char *dir);


__attribute__((visibility("default")))
int VideoCRCCheckerDestroy(struct VideoCRCChecker *chk);


__attribute__((visibility("default")))
void VideoCRCCheckerSetCRCCheckMode(struct VideoCRCChecker *chk, int mode);


__attribute__((visibility("default")))
uint32_t VideoCRCCheckerGetPrevCRC(struct VideoCRCChecker *chk);
__attribute__((visibility("default")))
uint32_t VideoCRCCheckerGetPrevMISR(struct VideoCRCChecker *chk);


__attribute__((visibility("default")))
int VideoCRCCheckerGetPendingCRCCount(struct VideoCRCChecker *chk);
__attribute__((visibility("default")))
uint32_t VideoCRCCheckerTotalCRCCount(struct VideoCRCChecker *chk);
__attribute__((visibility("default")))
int VideoCRCCheckerGetPendingMISRCount(struct VideoCRCChecker *chk);
__attribute__((visibility("default")))
uint32_t VideoCRCCheckerTotalMISRCount(struct VideoCRCChecker *chk);


__attribute__((visibility("default")))
int VideoCRCCheckerSkipFrameCRCCheck(struct VideoCRCChecker *chk);
__attribute__((visibility("default")))
int VideoCRCCheckerSkipFrameMISRCheck(struct VideoCRCChecker *chk);


__attribute__((visibility("default")))
int VideoCRCCheckerCheckCRCofYUVframe(struct VideoCRCChecker *chk,
                                      const unsigned char *buf,
                                      struct CrcFrame *info, int flag,
                                      uint32_t *out);


__attribute__((visibility("default")))
int VideoCRCGetCRCFileInfo(struct VideoCRCChecker *chk, const char **out,
                           int which);


__attribute__((visibility("default")))
void VideoCRCSetSkipOpbMisr(struct VideoCRCChecker *chk, int skip);


extern __attribute__((visibility("default"))) int CRCVideoInterlaceType;


__attribute__((visibility("default")))
void __aeabi_memcpy(void *dst, const void *src, size_t n);
__attribute__((visibility("default")))
void __aeabi_memcpy4(void *dst, const void *src, size_t n);
__attribute__((visibility("default")))
void __aeabi_memcpy8(void *dst, const void *src, size_t n);
__attribute__((visibility("default")))
void __aeabi_memset(void *dst, size_t n, int c);
__attribute__((visibility("default")))
void __aeabi_memset4(void *dst, size_t n, int c);
__attribute__((visibility("default")))
void __aeabi_memset8(void *dst, size_t n, int c);
__attribute__((visibility("default")))
void __aeabi_memclr(void *dst, size_t n);
__attribute__((visibility("default")))
void __aeabi_memclr4(void *dst, size_t n);
__attribute__((visibility("default")))
void __aeabi_memclr8(void *dst, size_t n);

#ifdef __cplusplus
}
#endif

#endif
