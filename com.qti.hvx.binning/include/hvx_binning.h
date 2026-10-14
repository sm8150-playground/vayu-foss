/*
 * Copyright (C) The LineageOS Project
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef LIB_HVX_BINNING_H
#define LIB_HVX_BINNING_H

#include <stdint.h>




struct HVXConfig;

struct HVXInputData;


typedef struct HVXDSPConfig {
    int32_t (*write)(int32_t *pResult, void *data, int32_t size);
    int32_t (*adspConfig_call)(int32_t *pResult);
} HVXDSPConfig;


typedef struct HVXResolutionInfo {
    int32_t inputWidth;
    int32_t inputHeight;
    int32_t field08;
    int32_t field0c;
    int32_t outputWidth;
    int32_t outputHeight;
} HVXResolutionInfo;


typedef struct HVXGetSetInfo {
    int32_t field00;
    int32_t field04;
    int32_t field08;
    int32_t field0c;
    int32_t field10;
    char name[32];
    int32_t field34;
    int32_t field38;
} HVXGetSetInfo;


typedef struct HVXFuncTable {

    int32_t (*initialize)(void **pOEMData);

    int32_t (*resolutionInfo)(void *pOEMData,
                              HVXResolutionInfo **pResolutionInfo);

    int32_t (*getHvxInfo)(void *pOEMData,
                          HVXGetSetInfo *pInfo);

    int32_t (*setConfig)(void *pOEMData, HVXConfig *config,
                         const HVXDSPConfig *dspConfig,
                         int32_t *pResult);
    void *unused20;

    int32_t (*execute)(void *pOEMData, const HVXInputData *input,
                       const HVXDSPConfig *dspConfig,
                       int32_t *pResult);

    int32_t (*destroy)(void *pOEMData);
} HVXFuncTable;

#ifdef __cplusplus
extern "C" {
#endif


__attribute__((visibility("default")))
int32_t ChiISPHVXAlgorithmEntry(HVXFuncTable *funcTable);

#ifdef __cplusplus
}
#endif

#endif
