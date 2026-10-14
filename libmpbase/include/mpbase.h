/*
 * Copyright (C) The LineageOS Project
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef LIB_MPBASE_H
#define LIB_MPBASE_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif




typedef struct MpbaseVersion {
    uint64_t major;
    uint64_t minor;
    uint64_t patch;
    uint64_t build;
    const char *name;
    const char *build_date;
    const char *copyright;
} MpbaseVersion;



__attribute__((visibility("default")))
void *MMemCpy(void *dst, const void *src, size_t n);

__attribute__((visibility("default")))
void *MMemSet(void *dst, int value, size_t n);

__attribute__((visibility("default")))
void *MMemMove(void *dst, const void *src, size_t n);

__attribute__((visibility("default")))
long MMemCmp(const void *lhs, const void *rhs, size_t n);




__attribute__((visibility("default")))
void *MMemMgrCreate(void *pBase, size_t size);


__attribute__((visibility("default")))
void MMemMgrDestroy(void *pMgr);



__attribute__((visibility("default")))
void *MMemAlloc(void *pMgr, unsigned int size);

__attribute__((visibility("default")))
void *MMemAllocStatic(void *pMgr, unsigned int size);

__attribute__((visibility("default")))
void MMemFree(void *pMgr, void *ptr);

__attribute__((visibility("default")))
void MMemFreeStatic(void *pMgr, void *ptr);

__attribute__((visibility("default")))
void *MMemRealloc(void *pMgr, void *ptr, unsigned int size);

__attribute__((visibility("default")))
void *MMemReallocStatic(void *pMgr, void *ptr, unsigned int size);




__attribute__((visibility("default")))
size_t GetMaxAllocMemSize(void *pMgr);


__attribute__((visibility("default")))
MpbaseVersion *Mpbase_GetVersion(void);

#ifdef __cplusplus
}
#endif

#endif
