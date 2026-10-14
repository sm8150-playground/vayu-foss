/*
 * Copyright (C) 2016-2018, The Linux Foundation. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "qcbor.h"

#include <string.h>


#define CBOR_MAJOR_NONE_TYPE_RAW  9



static void Nesting_Init(QCBORTrackNesting *pNesting)
{

   pNesting->pCurrentNesting = &pNesting->pArrays[0];

   pNesting->pCurrentNesting->uMajorType = CBOR_MAJOR_TYPE_ARRAY;
}

static int Nesting_Increase(QCBORTrackNesting *pNesting, uint8_t uMajorType, uint32_t uPos, bool bBstWrap)
{
   int nReturn = QCBOR_SUCCESS;

   if(pNesting->pCurrentNesting == &pNesting->pArrays[QCBOR_MAX_ARRAY_NESTING]) {

      nReturn = QCBOR_ERR_ARRAY_NESTING_TOO_DEEP;
   } else {
      pNesting->pCurrentNesting++;
      pNesting->pCurrentNesting->uCount     = 0;
      pNesting->pCurrentNesting->uStart     = uPos;
      pNesting->pCurrentNesting->uMajorType = uMajorType;
      pNesting->pCurrentNesting->bBstrWrap  = bBstWrap;
   }
   return nReturn;
}

static void Nesting_Decrease(QCBORTrackNesting *pNesting)
{
   pNesting->pCurrentNesting--;
}

static int Nesting_Increment(QCBORTrackNesting *pNesting, uint16_t uAmount)
{
   if(uAmount >= QCBOR_MAX_ITEMS_IN_ARRAY - pNesting->pCurrentNesting->uCount) {
      return QCBOR_ERR_ARRAY_TOO_LONG;
   }

   pNesting->pCurrentNesting->uCount += uAmount;
   return QCBOR_SUCCESS;
}

static uint16_t Nesting_GetCount(QCBORTrackNesting *pNesting)
{

   return pNesting->pCurrentNesting->uCount / (pNesting->pCurrentNesting->uMajorType - CBOR_MAJOR_TYPE_ARRAY + 1);
}

static uint32_t Nesting_GetStartPos(QCBORTrackNesting *pNesting)
{
   return pNesting->pCurrentNesting->uStart;
}

static uint8_t Nesting_GetMajorType(QCBORTrackNesting *pNesting)
{
   return pNesting->pCurrentNesting->uMajorType;
}

static int Nesting_IsInNest(QCBORTrackNesting *pNesting)
{
   return pNesting->pCurrentNesting == &pNesting->pArrays[0] ? 0 : 1;
}

static bool Nesting_IsBstrWrapped(QCBORTrackNesting *pNesting)
{
   return pNesting->pCurrentNesting->bBstrWrap;
}



void QCBOREncode_Init(QCBOREncodeContext *me, void *pBuf, size_t uBufLen)
{
   memset(me, 0, sizeof(QCBOREncodeContext));
   if(uBufLen >= UINT32_MAX) {
      me->uError = QCBOR_ERR_BUFFER_TOO_LARGE;
   } else {
      UsefulOutBuf_Init(&(me->OutBuf), pBuf, uBufLen);
      Nesting_Init(&(me->nesting));
   }
}



static void InsertEncodedTypeAndNumber(QCBOREncodeContext *me, uint8_t uMajorType, size_t uMinLen, uint64_t uNumber, size_t uPos)
{

   uMajorType <<= 5;

   if(uNumber > 0xffffffff || uMinLen >= 8) {
      UsefulOutBuf_InsertByte(&(me->OutBuf), uMajorType + LEN_IS_EIGHT_BYTES, uPos);
      UsefulOutBuf_InsertUint64(&(me->OutBuf), (uint64_t)uNumber, uPos+1);

   } else if(uNumber > 0xffff || uMinLen >= 4) {
      UsefulOutBuf_InsertByte(&(me->OutBuf), uMajorType + LEN_IS_FOUR_BYTES, uPos);
      UsefulOutBuf_InsertUint32(&(me->OutBuf), (uint32_t)uNumber, uPos+1);

   } else if (uNumber > 0xff) {

      UsefulOutBuf_InsertByte(&(me->OutBuf), uMajorType + LEN_IS_TWO_BYTES, uPos);
      UsefulOutBuf_InsertUint16(&(me->OutBuf), (uint16_t)uNumber, uPos+1);

   } else if(uNumber >= 24) {

      UsefulOutBuf_InsertByte(&(me->OutBuf), uMajorType + LEN_IS_ONE_BYTE, uPos);
      UsefulOutBuf_InsertByte(&(me->OutBuf), (uint8_t)uNumber, uPos+1);

   } else {

      UsefulOutBuf_InsertByte(&(me->OutBuf), uMajorType + (uint8_t)uNumber, uPos);
   }
}



static void AppendEncodedTypeAndNumber(QCBOREncodeContext *me, uint8_t uMajorType, uint64_t uNumber)
{

   InsertEncodedTypeAndNumber(me, uMajorType, 0, uNumber, UsefulOutBuf_GetEndPosition(&(me->OutBuf)));
}


static void AddBytesInternal(QCBOREncodeContext *me, const char *szLabel, int64_t nLabel, uint64_t uTag, UsefulBufC Bytes, uint8_t uMajorType, uint16_t uItems);



static void AddLabelAndOptionalTag(QCBOREncodeContext *me, const char *szLabel, int64_t nLabel, uint64_t uTag)
{
   if(szLabel) {
      UsefulBufC SZText = {szLabel, strlen(szLabel)};
      AddBytesInternal(me, NULL, nLabel, CBOR_TAG_NONE, SZText, CBOR_MAJOR_TYPE_TEXT_STRING, 0);
   } else if (QCBOR_NO_INT_LABEL != nLabel) {

      QCBOREncode_AddInt64_3(me, NULL, QCBOR_NO_INT_LABEL, CBOR_TAG_NONE, nLabel);
   }
   if(uTag != CBOR_TAG_NONE) {
      AppendEncodedTypeAndNumber(me, CBOR_MAJOR_TYPE_OPTIONAL, uTag);
   }
}



static void AddBytesInternal(QCBOREncodeContext *me, const char *szLabel, int64_t nLabel, uint64_t uTag, UsefulBufC Bytes, uint8_t uMajorType, uint16_t uItems)
{
   if(Bytes.len >= UINT32_MAX) {

      me->uError = QCBOR_ERR_BUFFER_TOO_LARGE;

   } else {

      AddLabelAndOptionalTag(me, szLabel, nLabel, uTag);

      if(!me->uError) {


         if(uMajorType != CBOR_MAJOR_NONE_TYPE_RAW) {
            AppendEncodedTypeAndNumber(me, uMajorType, Bytes.len);
         }


         UsefulOutBuf_AppendUsefulBuf(&(me->OutBuf), Bytes);


         me->uError = Nesting_Increment(&(me->nesting), uMajorType == CBOR_MAJOR_NONE_TYPE_RAW ? uItems : 1);
      }
   }
}



void QCBOREncode_AddBytes_3(QCBOREncodeContext *me, const char *szLabel, int64_t nLabel, uint64_t uTag, UsefulBufC Bytes)
{
   AddBytesInternal(me, szLabel, nLabel, uTag, Bytes, CBOR_MAJOR_TYPE_BYTE_STRING, 0);
}

void QCBOREncode_AddText_3(QCBOREncodeContext *me, const char *szLabel, int64_t nLabel, uint64_t uTag, UsefulBufC Bytes)
{
   AddBytesInternal(me, szLabel, nLabel, uTag, Bytes, CBOR_MAJOR_TYPE_TEXT_STRING, 0);
}

void QCBOREncode_AddRaw(QCBOREncodeContext *me, EncodedCBORC Raw)
{
   AddBytesInternal(me, NULL, QCBOR_NO_INT_LABEL, CBOR_TAG_NONE, Raw.Bytes, CBOR_MAJOR_NONE_TYPE_RAW, Raw.uItems);
}



static void OpenMapOrArrayInternal(QCBOREncodeContext *me, uint8_t uMajorType, const char *szLabel, uint64_t nLabel, uint64_t uTag, bool bBstrWrap)
{
   AddLabelAndOptionalTag(me, szLabel, nLabel, uTag);

   if(!me->uError) {

      me->uError = Nesting_Increment(&(me->nesting), 1);
      if(!me->uError) {

         me->uError = Nesting_Increase(&(me->nesting),
                                       uMajorType, (uint32_t)UsefulOutBuf_GetEndPosition(&(me->OutBuf)),
                                       bBstrWrap);
      }
   }
}



void QCBOREncode_OpenArray_3(QCBOREncodeContext *me, const char *szLabel, uint64_t nLabel, uint64_t uTag, bool bBstrWrap)
{
   OpenMapOrArrayInternal(me, CBOR_MAJOR_TYPE_ARRAY, szLabel, nLabel, uTag, bBstrWrap);
}

void QCBOREncode_OpenMap_3(QCBOREncodeContext *me, const char *szLabel, uint64_t nLabel, uint64_t uTag, uint8_t bBstrWrap)
{
   OpenMapOrArrayInternal(me, CBOR_MAJOR_TYPE_MAP, szLabel, nLabel, uTag, bBstrWrap);
}

void QCBOREncode_CloseArray(QCBOREncodeContext *me)
{
   if(!Nesting_IsInNest(&(me->nesting))) {
      me->uError = QCBOR_ERR_TOO_MANY_CLOSES;

   } else {


      const uint32_t uInsertPosition = Nesting_GetStartPos(&(me->nesting));

      InsertEncodedTypeAndNumber(me,
                                 Nesting_GetMajorType(&(me->nesting)),
                                 0,
                                 Nesting_GetCount(&(me->nesting)),
                                 uInsertPosition);

      if(Nesting_IsBstrWrapped(&(me->nesting))) {



         uint32_t uLenOfEncodedMapOrArray = (uint32_t)UsefulOutBuf_GetEndPosition(&(me->OutBuf)) - uInsertPosition;


         InsertEncodedTypeAndNumber(me,
                                    CBOR_MAJOR_TYPE_BYTE_STRING,
                                    0,
                                    uLenOfEncodedMapOrArray,
                                    uInsertPosition);
      }

      Nesting_Decrease(&(me->nesting));
   }
}



static void AddUInt64Internal(QCBOREncodeContext *me, const char *szLabel, int64_t nLabel, uint64_t uTag, uint8_t uMajorType, uint64_t n)
{
   AddLabelAndOptionalTag(me, szLabel, nLabel, uTag);
   if(!me->uError) {
      AppendEncodedTypeAndNumber(me, uMajorType, n);
      me->uError = Nesting_Increment(&(me->nesting), 1);
   }
}



void QCBOREncode_AddUInt64_3(QCBOREncodeContext *me, const char *szLabel, int64_t nLabel, uint64_t uTag, uint64_t uNum)
{
   AddUInt64Internal(me, szLabel, nLabel, uTag, CBOR_MAJOR_TYPE_POSITIVE_INT, uNum);
}


__attribute__((noinline))
void QCBOREncode_AddInt64_3(QCBOREncodeContext *me, const char *szLabel, int64_t nLabel, uint64_t uTag, int64_t nNum)
{
   uint8_t      uMajorType;
   uint64_t     uValue;


   if(nNum < 0) {
      uValue = (uint64_t)(-nNum - 1);
      uMajorType = CBOR_MAJOR_TYPE_NEGATIVE_INT;
   } else {
      uValue = (uint64_t)nNum;
      uMajorType = CBOR_MAJOR_TYPE_POSITIVE_INT;
   }
   AddUInt64Internal(me, szLabel, nLabel, uTag, uMajorType, uValue);
}



static void AddSimpleInternal(QCBOREncodeContext *me, const char *szLabel, int64_t nLabel, uint64_t uTag, size_t uSize, uint64_t uNum)
{
   AddLabelAndOptionalTag(me, szLabel, nLabel, uTag);
   if(!me->uError) {

      InsertEncodedTypeAndNumber(me,
                                 CBOR_MAJOR_TYPE_SIMPLE,
                                 uSize,
                                 uNum,
                                 UsefulOutBuf_GetEndPosition(&(me->OutBuf)));

      me->uError = Nesting_Increment(&(me->nesting), 1);
   }
}



__attribute__((noinline))
void QCBOREncode_AddRawSimple_3(QCBOREncodeContext *me, const char *szLabel, int64_t nLabel, uint64_t uTag, uint8_t uSimple)
{
   AddSimpleInternal(me, szLabel, nLabel, uTag, 0, uSimple);
}



void QCBOREncode_AddSimple_3(QCBOREncodeContext *me, const char *szLabel, int64_t nLabel, uint64_t uTag, uint8_t uSimple)
{
   if(uSimple < CBOR_SIMPLEV_FALSE || uSimple > CBOR_SIMPLEV_UNDEF) {
      me->uError = QCBOR_ERR_BAD_SIMPLE;
   } else {
      QCBOREncode_AddRawSimple_3(me, szLabel, nLabel, uTag, uSimple);
   }
}



void QCBOREncode_AddFloat_3(QCBOREncodeContext *me, const char *szLabel, int64_t nLabel, uint64_t uTag, float fNum)
{

   const float *pfNum  = &fNum;
   const uint32_t uNum = *(uint32_t *)pfNum;

   AddSimpleInternal(me, szLabel, nLabel, uTag, sizeof(float), uNum);
}

void QCBOREncode_AddDouble_3(QCBOREncodeContext *me, const char *szLabel, int64_t nLabel, uint64_t uTag, double dNum)
{

   const double *pdNum = &dNum;
   const uint64_t uNum = *(uint64_t *)pdNum;

   AddSimpleInternal(me, szLabel, nLabel, uTag, sizeof(double), uNum);
}



int QCBOREncode_Finish2(QCBOREncodeContext *me, EncodedCBOR *pEncodedCBOR)
{
   if(me->uError) {
      goto Done;
   }

   if (Nesting_IsInNest(&(me->nesting))) {
      me->uError = QCBOR_ERR_ARRAY_OR_MAP_STILL_OPEN;
      goto Done;
   }

   if(UsefulOutBuf_GetError(&(me->OutBuf))) {

      me->uError = QCBOR_ERR_BUFFER_TOO_SMALL;
      goto Done;
   }

   UsefulOutBuf_OutUBuf(&(me->OutBuf), &(pEncodedCBOR->Bytes));
   pEncodedCBOR->uItems = Nesting_GetCount(&(me->nesting));

Done:
   return me->uError;
}

int QCBOREncode_Finish(QCBOREncodeContext *me, size_t *puEncodedLen)
{
   EncodedCBOR Enc;

   int nReturn = QCBOREncode_Finish2(me, &Enc);

   if(nReturn == QCBOR_SUCCESS) {
      *puEncodedLen = Enc.Bytes.len;
   }

   return nReturn;
}


