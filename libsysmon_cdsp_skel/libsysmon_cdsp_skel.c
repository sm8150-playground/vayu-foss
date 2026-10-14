/*
 * Copyright (C) The LineageOS Project
 *
 * SPDX-License-Identifier: Apache-2.0
 */



#include <errno.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include <android/log.h>






#define AEE_SUCCESS       0
#define AEE_EBADPARM      14
#define AEE_EUNSUPPORTED  20


#define REMOTE_SCALARS_METHOD(sc)     (((sc) >> 24) & 0x1f)
#define REMOTE_SCALARS_INBUFS(sc)     (((sc) >> 16) & 0x0ff)
#define REMOTE_SCALARS_OUTBUFS(sc)    (((sc) >> 8) & 0x0ff)
#define REMOTE_SCALARS_INHANDLES(sc)  (((sc) >> 4) & 0x0f)
#define REMOTE_SCALARS_OUTHANDLES(sc) ((sc) & 0x0f)

#define REMOTE_SCALARS_LENGTH(sc) \
    (REMOTE_SCALARS_INBUFS(sc) + REMOTE_SCALARS_OUTBUFS(sc) + \
     REMOTE_SCALARS_INHANDLES(sc) + REMOTE_SCALARS_OUTHANDLES(sc))

#define REMOTE_SCALARS_MAKEX(nAttr, nMethod, nIn, nOut, noIn, noOut) \
    ((((uint32_t)(nAttr) & 0x7) << 29) | (((uint32_t)(nMethod) & 0x1f) << 24) | \
     (((uint32_t)(nIn) & 0xff) << 16) | (((uint32_t)(nOut) & 0xff) << 8) | \
     (((uint32_t)(noIn) & 0x0f) << 4) | ((uint32_t)(noOut) & 0x0f))

typedef uint32_t remote_handle;
typedef uint64_t remote_handle64;

typedef struct {
    void *pv;
    size_t nLen;
} remote_buf;

typedef struct {
    int32_t fd;
    uint32_t offset;
} remote_dma_handle;

typedef union {
    remote_buf buf;
    remote_handle h;
    remote_handle64 h64;
    remote_dma_handle dma;
} remote_arg;





#define HAP_LEVEL_HIGH 2


#define HAP_SOURCE_FILE "vendor/qcom/proprietary/adsprpc/src/sysmon_cdsp_imp.c"


#define HAP_LINE_VOTE_OK      45
#define HAP_LINE_OPEN_FAILED  50
#define HAP_LINE_VOTE_INVALID 55


extern void HAP_debug_v2(int level, const char *file, int line,
                         const char *format, ...) __attribute__((weak));

extern char *__progname;


__attribute__((visibility("default")))
void HAP_debug(const char *msg, int level, const char *filename, int line)
{
    __android_log_print(level, __progname, "%s:%d: %s", filename, line, msg);
}


static inline void _HAP_debug_v2(int level, const char *file, int line,
                                 const char *format, ...)
{
    char buf[256];
    va_list args;

    va_start(args, format);
    vsnprintf(buf, sizeof(buf), format, args);
    va_end(args);
    HAP_debug(buf, level, file, line);
}


#define FARF_HIGH(line, ...)                                             \
    do {                                                                 \
        if (HAP_debug_v2 == 0) {                                         \
            _HAP_debug_v2(HAP_LEVEL_HIGH, HAP_SOURCE_FILE, line,         \
                          __VA_ARGS__);                                  \
        } else {                                                         \
            HAP_debug_v2(HAP_LEVEL_HIGH, HAP_SOURCE_FILE, line,          \
                         __VA_ARGS__);                                   \
        }                                                                \
    } while (0)





#define _TRY(ee, func)                     \
    do {                                   \
        if (AEE_SUCCESS != ((ee) = (func))) \
            goto ee##bail;                 \
    } while (0)

#define _CATCH(exception) exception##bail: if (exception != AEE_SUCCESS)

#define _ASSERT(nErr, ff) _TRY(nErr, (ff) ? AEE_SUCCESS : AEE_EBADPARM)





#define SYSMON_L3_CDSP_SET_FREQ                                           \
    "/sys/devices/platform/soc/soc:qcom,l3-cdsp/devfreq/soc:qcom,l3-cdsp" \
    "/userspace/set_freq"


#define VOTE_OK          0
#define VOTE_OPEN_FAILED 1
#define VOTE_REJECTED    2

__attribute__((visibility("default")))
int sysmon_cdsp_vote_l3(int32_t nFreqKHz)
{
    char buf[16] = {0};
    int len;
    FILE *fp;


    if (nFreqKHz) {
        len = snprintf(buf, sizeof(buf), "%d000", nFreqKHz);
    } else {
        len = snprintf(buf, sizeof(buf), "%d", 0);
    }


    fp = fopen(SYSMON_L3_CDSP_SET_FREQ, "wb");
    if ((len < 1) || (len >= (int)sizeof(buf)) || (nFreqKHz < 0) || (fp == NULL)) {
        if (fp != NULL) {
            FARF_HIGH(HAP_LINE_VOTE_INVALID,
                      "Received incorrect L3 frequency vote from CDSP for %d KHz",
                      nFreqKHz);
            return VOTE_REJECTED;
        }

        FARF_HIGH(HAP_LINE_OPEN_FAILED,
                  "Failed to open CDSP L3 frequency userspace governor (%s)",
                  strerror(errno != 0 ? errno : -1));
        return VOTE_OPEN_FAILED;
    }


    fwrite(buf, 1, (size_t)len + 1, fp);
    fclose(fp);

    FARF_HIGH(HAP_LINE_VOTE_OK,
              "Got L3 frequency request from CDSP for %dKHz", nFreqKHz);
    return VOTE_OK;
}





static int _skel_method(int (*_pfn)(int32_t), uint32_t _sc, remote_arg *_pra)
{
    remote_arg *_praEnd;
    int32_t _in0[1];
    int32_t *_primIn;
    int _nErr = AEE_SUCCESS;

    _praEnd = _pra + REMOTE_SCALARS_LENGTH(_sc);
    _ASSERT(_nErr, (_pra + 1) <= _praEnd);
    _ASSERT(_nErr, _pra[0].buf.nLen >= 4);
    _primIn = (int32_t *)_pra[0].buf.pv;
    _in0[0] = *_primIn;
    _TRY(_nErr, _pfn(_in0[0]));
    _CATCH(_nErr) {}
    return _nErr;
}

__attribute__((visibility("default")))
int sysmon_cdsp_skel_invoke(uint32_t _sc, remote_arg *_pra)
{
    switch (REMOTE_SCALARS_METHOD(_sc)) {
    case 0:
        return _skel_method((int (*)(int32_t))sysmon_cdsp_vote_l3, _sc, _pra);
    }
    return AEE_EUNSUPPORTED;
}


typedef struct Type Type;

#define INHERIT_TYPE                                            \
    int32_t nativeSize;                                         \
    union {                                                     \
        struct {                                                \
            const uintptr_t p1;                                 \
            const uintptr_t p2;                                 \
        } _cast;                                                \
        struct {                                                \
            uint32_t iid;                                       \
            uint32_t bNotNil;                                   \
        } object;                                               \
        struct {                                                \
            const Type *arrayType;                              \
            int32_t nItems;                                     \
        } array;                                                \
        struct {                                                \
            const Type *seqType;                                \
            int32_t nMaxLen;                                    \
        } seqSimple;                                            \
        struct {                                                \
            uint32_t bFloating;                                 \
            uint32_t bSigned;                                   \
        } prim;                                                 \
        const void *seqComplex;                                 \
        const void *unionType;                                  \
        const void *structType;                                 \
        int32_t stringMaxLen;                                   \
        uint8_t bInterfaceNotNil;                               \
    } param;                                                    \
    uint8_t type;                                               \
    uint8_t nativeAlignment

typedef struct {
    INHERIT_TYPE;
    uint8_t mode;
    uint8_t bNotNil;
} Parameter;

typedef struct {
    uint32_t uScalars;
    int32_t primInSize;
    int32_t primROutSize;
    int maxArgs;
    int numParams;
    const Parameter *const *params;
    uint8_t primInAlignment;
    uint8_t primROutAlignment;
} Method;

typedef struct {
    int nMethods;
    const Method *const *methodArray;
    int nIIds;
    const uint32_t *iids;
    const uint16_t *methodStringArray;
    const uint16_t *methodStrings;
    const char *strings;
} Interface;


static const Parameter parameters[1] = {
    {0x4, {{(const uintptr_t)0, (const uintptr_t)1}}, 2, 0x4, 0, 0}};
static const Parameter *const parameterArrays[1] = {(&(parameters[0]))};
static const Method methods[1] = {
    {REMOTE_SCALARS_MAKEX(0, 0, 0x1, 0x0, 0x0, 0x0), 0x4, 0x0, 1, 1,
     (&(parameterArrays[0])), 0x4, 0x0}};
static const Method *const methodArrays[1] = {&(methods[0])};
static const char strings[16] = "freqKHz\0vote_l3\0";
static const uint16_t methodStrings[2] = {8, 0};
static const uint16_t methodStringsArrays[1] = {0};

__attribute__((visibility("default")))
const Interface sysmon_cdsp_slim = {
    1, &(methodArrays[0]), 0, 0, &(methodStringsArrays[0]), methodStrings,
    strings};
