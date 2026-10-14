/*
 * Copyright (C) The LineageOS Project
 *
 * SPDX-License-Identifier: Apache-2.0
 */



#include <stdlib.h>
#include <string.h>

#include "mpbase.h"


#define MP_HDR 16


#define MP_END ((uint64_t)-1)

typedef struct MemBlock {
    uint64_t size;
    uint64_t used;
} MemBlock;

static MpbaseVersion g_version = {
    0, 1, 0, 4,

    "ArcSoft_Mpbase",
    "Apr 24 2019",
    "Copyright 2014-2015, ArcSoft Inc.",
};



void *MMemCpy(void *dst, const void *src, size_t n)
{
    return memcpy(dst, src, n);
}

void *MMemSet(void *dst, int value, size_t n)
{
    return memset(dst, (unsigned char)value, n);
}

void *MMemMove(void *dst, const void *src, size_t n)
{
    return memmove(dst, src, n);
}

long MMemCmp(const void *lhs, const void *rhs, size_t n)
{

    return memcmp(lhs, rhs, n);
}



void *MMemMgrCreate(void *pBase, size_t size)
{
    uintptr_t base, total;
    MemBlock *blk, *end;
    int64_t free_size;

    if (pBase == NULL)
        return NULL;

    base = ((uintptr_t)pBase + 7u) & ~(uintptr_t)7u;
    total = (((uintptr_t)pBase + size) - base) & ~(uintptr_t)7u;


    free_size = (int64_t)total - 2 * (int64_t)MP_HDR;
    if (free_size <= 0)
        return NULL;

    blk = (MemBlock *)base;
    blk->size = (uint64_t)free_size;
    blk->used = 0;

    end = (MemBlock *)(base + total - MP_HDR);
    end->size = MP_END;
    end->used = 0;

    return (void *)base;
}

void MMemMgrDestroy(void *pMgr)
{

    (void)pMgr;
}



void *MMemAllocStatic(void *pMgr, unsigned int size)
{
    MemBlock *blk;
    MemBlock *next;
    uint64_t need;
    uint64_t rest;

    if (pMgr == NULL)
        return NULL;
    if (size == 0)
        return NULL;

    blk = (MemBlock *)pMgr;
    need = (uint64_t)((size + 7u) & ~7u);
    if (blk->size == MP_END)
        return NULL;


    for (;;) {
        next = (MemBlock *)((uintptr_t)blk + blk->size + MP_HDR);
        if (blk->used != 0 || (int64_t)need > (int64_t)blk->size) {
            blk = next;
            if (blk->size == MP_END)
                return NULL;
            continue;
        }
        break;
    }

    rest = blk->size - need - MP_HDR;
    if ((int64_t)rest > 31) {
        MemBlock *tail = (MemBlock *)((uintptr_t)blk + need + MP_HDR);

        tail->size = rest;
        tail->used = 0;
        blk->size = need;
    }

    blk->used = 1;
    return (void *)((uintptr_t)blk + MP_HDR);
}

void MMemFreeStatic(void *pMgr, void *ptr)
{
    uintptr_t base = (uintptr_t)pMgr;
    MemBlock *prev, *cur, *next;
    uint64_t off;

    if (pMgr == NULL || ptr == NULL)
        return;

    if ((uintptr_t)ptr == base + MP_HDR) {

        cur = (MemBlock *)base;
        off = cur->size + MP_HDR;
        cur->used = 0;
        next = (MemBlock *)((uintptr_t)cur + off);
        if (next->size != MP_END && next->used == 0)
            cur->size = off + next->size;
        return;
    }


    prev = (MemBlock *)base;
    for (;;) {
        cur = (MemBlock *)((uintptr_t)prev + prev->size + MP_HDR);
        if ((uintptr_t)ptr == (uintptr_t)cur + MP_HDR)
            break;
        prev = cur;
    }

    off = cur->size + MP_HDR;
    cur->used = 0;

    next = (MemBlock *)((uintptr_t)cur + off);
    if (next->size != MP_END && next->used == 0)
        cur->size = off + next->size;

    if (prev->used == 0)
        prev->size = prev->size + cur->size + MP_HDR;
}



void *MMemReallocStatic(void *pMgr, void *ptr, unsigned int size)
{
    MemBlock *cur;
    MemBlock *next;
    uint64_t need;
    uint64_t rest;

    if (pMgr == NULL)
        return NULL;
    if (size == 0)
        return NULL;
    if (ptr == NULL)
        return MMemAllocStatic(pMgr, size);

    need = (uint64_t)((size + 7u) & ~7u);

    cur = (MemBlock *)pMgr;
    while ((uintptr_t)ptr != (uintptr_t)cur + MP_HDR)
        cur = (MemBlock *)((uintptr_t)cur + cur->size + MP_HDR);

    if ((int64_t)cur->size >= (int64_t)need) {

        rest = cur->size - need - MP_HDR;
        if ((int64_t)rest > 32) {
            MemBlock *tail = (MemBlock *)((uintptr_t)cur + need + MP_HDR);


            next = (MemBlock *)((uintptr_t)cur + cur->size + MP_HDR);

            tail->size = rest;
            tail->used = 0;
            cur->size = need;

            if (next->size != MP_END && next->used == 0)
                tail->size = tail->size + next->size + MP_HDR;
        }
        return ptr;
    }


    next = (MemBlock *)((uintptr_t)cur + cur->size + MP_HDR);
    if (next->size != MP_END && next->used == 0 &&
        (int64_t)need <= (int64_t)(cur->size + next->size)) {
        cur->size = cur->size + next->size + MP_HDR;
        rest = cur->size - need - MP_HDR;
        if ((int64_t)rest > 31) {
            MemBlock *tail = (MemBlock *)((uintptr_t)cur + need + MP_HDR);

            tail->size = rest;
            tail->used = 0;
            cur->size = need;
        }
        return ptr;
    }


    {
        void *fresh = MMemAllocStatic(pMgr, (unsigned int)need);

        if (fresh == NULL)
            return NULL;
        MMemCpy(fresh, ptr, cur->size);
        MMemFreeStatic(pMgr, ptr);
        return fresh;
    }
}



void *MMemAlloc(void *pMgr, unsigned int size)
{
    if (pMgr != NULL)
        return MMemAllocStatic(pMgr, size);
    return malloc(size);
}

void MMemFree(void *pMgr, void *ptr)
{
    if (pMgr != NULL)
        MMemFreeStatic(pMgr, ptr);
    else
        free(ptr);
}

void *MMemRealloc(void *pMgr, void *ptr, unsigned int size)
{
    if (pMgr != NULL)
        return MMemReallocStatic(pMgr, ptr, size);
    return realloc(ptr, size);
}



size_t GetMaxAllocMemSize(void *pMgr)
{
    MemBlock *blk;
    uint64_t best = 0;

    if (pMgr == NULL)
        return 2;

    blk = (MemBlock *)pMgr;
    if (blk->size == MP_END)
        return (size_t)best;

    for (;;) {
        if ((int64_t)best < (int64_t)blk->size && blk->used == 0)
            best = blk->size;
        blk = (MemBlock *)((uintptr_t)blk + blk->size + MP_HDR);
        if (blk->size == MP_END)
            break;
    }

    return (size_t)best;
}

MpbaseVersion *Mpbase_GetVersion(void)
{
    g_version.major = 0;
    g_version.minor = 1;
    g_version.patch = 0;
    g_version.build = 4;
    g_version.name = "ArcSoft_Mpbase_0.1.0.4";
    return &g_version;
}
