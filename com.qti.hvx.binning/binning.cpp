/*
 * Copyright (C) The LineageOS Project
 *
 * SPDX-License-Identifier: Apache-2.0
 */



#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "hvx_binning.h"


#define HVX_TAG "ISPHVX"
#define HVX_LOG_ERROR 6


extern "C" int __android_log_print(int prio, const char *tag, const char *fmt,
                                   ...);


#define FMT_FUNC_TABLE "%s():%d func_table %p\n\n"
#define FMT_OEM_INFO "%s():%d failed: pOEMData %p pHVXInfo %p\n\n"
#define FMT_DSP_RET "%s():%d failed: pDSPDynamicConfig ret %d\n\n"
#define FMT_FAIL_PTRS "%s():%d failed: %p %p %p\n\n"
#define FMT_ADSP_RET "%s():%d failed: adspConfig_call ret %d\n\n"
#define FMT_CLOSE "%s():%d failed: hvx_lib_close pOEMData %p\n\n"
#define FMT_DATA "%s():%d failed: data %p\n\n"


#define L_ENTRY 191
#define L_INIT 38
#define L_RESOLUTION 62
#define L_GETINFO 101
#define L_SETCONFIG 147
#define L_SETCONFIG_ADSP 134
#define L_SETCONFIG_DSP 142
#define L_DESTROY 168


#define BINNING_NAME "binning"
#define OEM_DATA_SIZE 4

#define LOG(prio, ...) __android_log_print((prio), HVX_TAG, __VA_ARGS__)


static int32_t BinningHVXInitialize(void **pOEMData)
{
    void *data = calloc(1, OEM_DATA_SIZE);
    if (data == NULL) {
        LOG(HVX_LOG_ERROR, FMT_DATA, "BinningHVXInitialize", L_INIT,
            (void *)NULL);
        return 1;
    }

    *pOEMData = data;
    return 0;
}


static int32_t BinningResolutionInfo(void *pOEMData,
                                     HVXResolutionInfo **pResolutionInfo)
{

    if (pOEMData == NULL || pResolutionInfo == NULL) {
        LOG(HVX_LOG_ERROR, FMT_OEM_INFO, "BinningResolutionInfo", L_RESOLUTION,
            pOEMData, pResolutionInfo);
        return 1;
    }

    HVXResolutionInfo *info = *pResolutionInfo;
    const int32_t width = info->inputWidth;
    const int32_t height = info->inputHeight;

    info->field08 = 0;

    info->outputWidth = (int32_t)((uint32_t)width >> 1);
    info->outputHeight = (int32_t)((uint32_t)height >> 1);
    return 0;
}


static int32_t BinningGetHvxInfo(void *pOEMData, HVXGetSetInfo *pInfo)
{
    if (pOEMData == NULL || pInfo == NULL) {
        LOG(HVX_LOG_ERROR, FMT_OEM_INFO, "BinningGetHvxInfo", L_GETINFO,
            pOEMData, pInfo);
        return 1;
    }

    pInfo->field10 = 1;
    strlcpy(pInfo->name, BINNING_NAME, sizeof(pInfo->name));

    if (pInfo->field00 != 0)
        pInfo->field34 = 1;
    if (pInfo->field04 >= 1)
        pInfo->field38 = 1;

    return 0;
}


static int32_t BinningSetConfig(void *pOEMData, HVXConfig *pConfig,
                                const HVXDSPConfig *pDSPConfig,
                                int32_t *pResult)
{
    if (pOEMData == NULL || pConfig == NULL || pDSPConfig == NULL ||
        pDSPConfig->write == NULL || pDSPConfig->adspConfig_call == NULL) {
        LOG(HVX_LOG_ERROR, FMT_FAIL_PTRS, "BinningSetConfig", L_SETCONFIG,
            pOEMData, (void *)NULL, pDSPConfig);
        return 1;
    }

    int32_t ret = pDSPConfig->adspConfig_call(pResult);
    if (ret != 0)
        LOG(HVX_LOG_ERROR, FMT_ADSP_RET, "BinningSetConfig", L_SETCONFIG_ADSP,
            ret);

    ret = pDSPConfig->write(pResult, pOEMData, OEM_DATA_SIZE);
    if (ret != 0)
        LOG(HVX_LOG_ERROR, FMT_DSP_RET, "BinningSetConfig", L_SETCONFIG_DSP,
            ret);

    return ret;
}


static int32_t BinningDestroy(void *pOEMData)
{
    if (pOEMData == NULL) {
        LOG(HVX_LOG_ERROR, FMT_CLOSE, "BinningDestroy", L_DESTROY, (void *)NULL);
        return 1;
    }

    free(pOEMData);
    return 0;
}

int32_t ChiISPHVXAlgorithmEntry(HVXFuncTable *funcTable)
{
    if (funcTable == NULL) {
        LOG(HVX_LOG_ERROR, FMT_FUNC_TABLE, "ChiISPHVXAlgorithmEntry", L_ENTRY,
            (void *)NULL);
        return 1;
    }

    funcTable->initialize = BinningHVXInitialize;
    funcTable->resolutionInfo = BinningResolutionInfo;
    funcTable->getHvxInfo = BinningGetHvxInfo;
    funcTable->setConfig = BinningSetConfig;
    funcTable->destroy = BinningDestroy;

    return 0;
}
