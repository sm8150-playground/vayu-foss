/*
 * Copyright (C) The LineageOS Project
 *
 * SPDX-License-Identifier: Apache-2.0
 */



#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "hvx_addconstant.h"


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


#define L_ENTRY 238
#define L_INIT 38
#define L_RESOLUTION 62
#define L_GETINFO 100
#define L_SETCONFIG 151
#define L_SETCONFIG_ADSP 135
#define L_SETCONFIG_DSP 145
#define L_EXECUTE 195
#define L_EXECUTE_DSP 188
#define L_DESTROY 214


#define ADD_CONSTANT_NAME "add_constant"
#define OEM_DATA_SIZE 4


#define ADD_CONSTANT_BASE 5
#define ADD_CONSTANT_STEP 5
#define ADD_CONSTANT_LIMIT 99

#define LOG(prio, ...) __android_log_print((prio), HVX_TAG, __VA_ARGS__)


static int32_t AddConstantHVXInitialize(void **pOEMData)
{
    void *data = calloc(1, OEM_DATA_SIZE);
    if (data == NULL) {
        LOG(HVX_LOG_ERROR, FMT_DATA, "AddConstantHVXInitialize", L_INIT,
            (void *)NULL);
        return 1;
    }

    *pOEMData = data;
    return 0;
}


static int32_t AddConstantResolutionInfo(void *pOEMData,
                                         HVXResolutionInfo **pResolutionInfo)
{
    if (pOEMData == NULL || pResolutionInfo == NULL || *pResolutionInfo == NULL) {
        LOG(HVX_LOG_ERROR, FMT_OEM_INFO, "AddConstantResolutionInfo",
            L_RESOLUTION, pOEMData, pResolutionInfo);
        return 1;
    }

    HVXResolutionInfo *info = *pResolutionInfo;
    info->outputWidth = info->inputWidth;
    info->outputHeight = info->inputHeight;
    info->field08 = 0;
    return 0;
}


static int32_t AddConstantGetHvxInfo(void *pOEMData, HVXGetSetInfo *pInfo)
{
    if (pOEMData == NULL || pInfo == NULL) {
        LOG(HVX_LOG_ERROR, FMT_OEM_INFO, "AddConstantGetHvxInfo", L_GETINFO,
            pOEMData, pInfo);
        return 1;
    }

    pInfo->field10 = 1;
    strlcpy(pInfo->name, ADD_CONSTANT_NAME, sizeof(pInfo->name));

    if (pInfo->field00 != 0)
        pInfo->field34 = 1;
    if (pInfo->field04 >= 1)
        pInfo->field38 = 1;

    return 0;
}


static int32_t AddConstantSetConfig(void *pOEMData, HVXConfig *pConfig,
                                    const HVXDSPConfig *pDSPConfig,
                                    int32_t *pResult)
{
    if (pOEMData == NULL || pConfig == NULL || pDSPConfig == NULL ||
        pDSPConfig->write == NULL || pDSPConfig->adspConfig_call == NULL) {
        LOG(HVX_LOG_ERROR, FMT_FAIL_PTRS, "AddConstantSetConfig", L_SETCONFIG,
            pOEMData, (void *)NULL, pDSPConfig);
        return 1;
    }

    int32_t ret = pDSPConfig->adspConfig_call(pResult);
    if (ret != 0)
        LOG(HVX_LOG_ERROR, FMT_ADSP_RET, "AddConstantSetConfig",
            L_SETCONFIG_ADSP, ret);

    *(int32_t *)pOEMData = ADD_CONSTANT_BASE;

    ret = pDSPConfig->write(pResult, pOEMData, OEM_DATA_SIZE);
    if (ret != 0)
        LOG(HVX_LOG_ERROR, FMT_DSP_RET, "AddConstantSetConfig", L_SETCONFIG_DSP,
            ret);

    return ret;
}


static int32_t AddConstantExecute(void *pOEMData, const HVXInputData *pInput,
                                  const HVXDSPConfig *pDSPConfig,
                                  int32_t *pResult)
{
    if (pOEMData == NULL || pDSPConfig == NULL || pInput == NULL ||
        pDSPConfig->write == NULL) {
        LOG(HVX_LOG_ERROR, FMT_FAIL_PTRS, "AddConstantExecute", L_EXECUTE,
            pOEMData, pDSPConfig, pInput);
        return 1;
    }


    uint32_t value = (uint32_t)*(int32_t *)pOEMData + ADD_CONSTANT_STEP;
    *(int32_t *)pOEMData = (value > (uint32_t)ADD_CONSTANT_LIMIT)
                               ? ADD_CONSTANT_BASE
                               : (int32_t)value;

    int32_t ret = pDSPConfig->write(pResult, pOEMData, OEM_DATA_SIZE);
    if (ret != 0)
        LOG(HVX_LOG_ERROR, FMT_DSP_RET, "AddConstantExecute", L_EXECUTE_DSP,
            ret);

    return ret;
}


static int32_t AddConstantDestroy(void *pOEMData)
{
    if (pOEMData == NULL) {
        LOG(HVX_LOG_ERROR, FMT_CLOSE, "AddConstantDestroy", L_DESTROY,
            (void *)NULL);
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

    funcTable->initialize = AddConstantHVXInitialize;
    funcTable->resolutionInfo = AddConstantResolutionInfo;
    funcTable->getHvxInfo = AddConstantGetHvxInfo;
    funcTable->setConfig = AddConstantSetConfig;
    funcTable->execute = AddConstantExecute;
    funcTable->destroy = AddConstantDestroy;

    return 0;
}
