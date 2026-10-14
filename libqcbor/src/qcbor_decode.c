/*
 * Copyright (C) 2016-2018, The Linux Foundation. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "qcbor.h"

#include <math.h>



static int IsMapOrArray(uint8_t uDataType)
{
   return uDataType == QCBOR_TYPE_MAP || uDataType == QCBOR_TYPE_ARRAY;
}

static int DecodeNesting_IsNested(const QCBORDecodeNesting *pNesting)
{
   return pNesting->pCurrent != &(pNesting->pMapsAndArrays[0]);
}

static int DecodeNesting_TypeIsMap(const QCBORDecodeNesting *pNesting)
{
   if(!DecodeNesting_IsNested(pNesting)) {
      return 0;
   }

   return CBOR_MAJOR_TYPE_MAP == pNesting->pCurrent->uMajorType;
}


static void DecodeNesting_Decrement(QCBORDecodeNesting *pNesting)
{
   if(!DecodeNesting_IsNested(pNesting)) {
      return;
   }


   pNesting->pCurrent->uCount--;


   while(0 == pNesting->pCurrent->uCount && DecodeNesting_IsNested(pNesting)) {
      pNesting->pCurrent--;
      pNesting->pCurrent->uCount--;
   }
}

static int DecodeNesting_Descend(QCBORDecodeNesting *pNesting, uint8_t uMajorType, int uCount)
{
   int nReturn = QCBOR_SUCCESS;

   if(uCount > (int)QCBOR_MAX_ITEMS_IN_ARRAY) {
      nReturn = QCBOR_ERR_ARRAY_TOO_LONG;
      goto Done;
   }

   if(pNesting->pCurrent >= &(pNesting->pMapsAndArrays[QCBOR_MAX_ARRAY_NESTING])) {
      nReturn = QCBOR_ERR_ARRAY_NESTING_TOO_DEEP;
      goto Done;
   }

   pNesting->pCurrent++;

   pNesting->pCurrent->uMajorType = uMajorType;
   pNesting->pCurrent->uCount     = uCount;

Done:
   return nReturn;
}

static uint8_t DecodeNesting_GetLevel(QCBORDecodeNesting *pNesting)
{
   return pNesting->pCurrent - &(pNesting->pMapsAndArrays[0]);
}

static void DecodeNesting_Init(QCBORDecodeNesting *pNesting)
{
   pNesting->pCurrent = &(pNesting->pMapsAndArrays[0]);
}



void QCBORDecode_Init(QCBORDecodeContext *me, UsefulBufC EncodedCBOR, int8_t nDecodeMode)
{
   memset(me, 0, sizeof(QCBORDecodeContext));
   UsefulInputBuf_Init(&(me->InBuf), EncodedCBOR);

   me->uDecodeMode = nDecodeMode;
   DecodeNesting_Init(&(me->nesting));
}



static int DecodeTypeAndNumber(UsefulInputBuf *pUInBuf, int *pnMajorType, uint64_t *puNumber, uint8_t *puAdditionalInfo)
{
   int nReturn;


   const uint8_t InitialByte = UsefulInputBuf_GetByte(pUInBuf);


   const uint8_t uTmpMajorType   = InitialByte >> 5;
   const uint8_t uAdditionalInfo = InitialByte & 0x1f;


   uint64_t uTmpValue;
   switch(uAdditionalInfo) {

      case LEN_IS_ONE_BYTE:
         uTmpValue = UsefulInputBuf_GetByte(pUInBuf);
         break;

      case LEN_IS_TWO_BYTES:
         uTmpValue = UsefulInputBuf_GetUint16(pUInBuf);
         break;

      case LEN_IS_FOUR_BYTES:
         uTmpValue = UsefulInputBuf_GetUint32(pUInBuf);
         break;

      case LEN_IS_EIGHT_BYTES:
         uTmpValue = UsefulInputBuf_GetUint64(pUInBuf);
         break;

      case ADDINFO_RESERVED1:
      case ADDINFO_RESERVED2:
      case ADDINFO_RESERVED3:
      case LEN_IS_INDEFINITE:
         nReturn = QCBOR_ERR_UNSUPPORTED;
         goto Done;

      default:
         uTmpValue = uAdditionalInfo;
         break;
   }



   if(UsefulInputBuf_GetError(pUInBuf)) {
      nReturn = QCBOR_ERR_HIT_END;
      goto Done;
   }


   nReturn           = QCBOR_SUCCESS;
   *pnMajorType      = uTmpMajorType;
   *puNumber         = uTmpValue;
   *puAdditionalInfo = uAdditionalInfo;

Done:
   return nReturn;
}



static int DecodeInteger(int nMajorType, uint64_t uNumber, QCBORItem *pDecodedItem)
{
   int nReturn = QCBOR_SUCCESS;

   if(nMajorType == CBOR_MAJOR_TYPE_POSITIVE_INT) {
      if (uNumber <= INT64_MAX) {
         pDecodedItem->val.int64 = (int64_t)uNumber;
         pDecodedItem->uDataType = QCBOR_TYPE_INT64;

      } else {
         pDecodedItem->val.uint64 = uNumber;
         pDecodedItem->uDataType  = QCBOR_TYPE_UINT64;

      }
   } else {
      if(uNumber <= INT64_MAX) {
         pDecodedItem->val.int64 = -uNumber-1;
         pDecodedItem->uDataType = QCBOR_TYPE_INT64;

      } else {

         nReturn = QCBOR_ERR_INT_OVERFLOW;
      }
   }

   return nReturn;
}



static double DecodeHalfFloat(uint64_t uNumber)
{
   const uint16_t uHalf = (uint16_t)uNumber;

   const uint32_t uMantissa = ((uHalf >> 8) & 0xff) | ((uHalf & 0x03) << 8);
   const uint32_t uExponent = (uHalf >> 2) & 0x1f;

   double dValue;

   if(uExponent == 0x1f) {
      dValue = (uMantissa == 0) ? (double)INFINITY : (double)NAN;
   } else if(uExponent != 0) {
      dValue = ldexp((double)(uMantissa | 0x400), (int)uExponent - 25);
   } else {
      dValue = ldexp((double)uMantissa, -24);
   }

   if((int8_t)uHalf < 0) {
      dValue = -dValue;
   }

   return dValue;
}



#if QCBOR_TYPE_FALSE != CBOR_SIMPLEV_FALSE
#error QCBOR_TYPE_FALSE macro value wrong
#endif

#if QCBOR_TYPE_TRUE != CBOR_SIMPLEV_TRUE
#error QCBOR_TYPE_TRUE macro value wrong
#endif

#if QCBOR_TYPE_NULL != CBOR_SIMPLEV_NULL
#error QCBOR_TYPE_NULL macro value wrong
#endif

#if QCBOR_TYPE_UNDEF != CBOR_SIMPLEV_UNDEF
#error QCBOR_TYPE_UNDEF macro value wrong
#endif

#if QCBOR_TYPE_DOUBLE != DOUBLE_PREC_FLOAT
#error QCBOR_TYPE_DOUBLE macro value wrong
#endif

#if QCBOR_TYPE_FLOAT != SINGLE_PREC_FLOAT
#error QCBOR_TYPE_FLOAT macro value wrong
#endif

#if QCBOR_TYPE_OPTTAG != 254
#error QCBOR_TYPE_OPTTAG macro value wrong
#endif


static int DecodeSimple(uint8_t uAdditionalInfo, uint64_t uNumber, QCBORItem *pDecodedItem)
{
   int nReturn = QCBOR_SUCCESS;


   pDecodedItem->uDataType = uAdditionalInfo;

   switch(uAdditionalInfo) {
      case ADDINFO_RESERVED1:
      case ADDINFO_RESERVED2:
      case ADDINFO_RESERVED3:
      case CBOR_SIMPLE_BREAK:
         nReturn = QCBOR_ERR_UNSUPPORTED;
         break;

      case CBOR_SIMPLEV_FALSE:
      case CBOR_SIMPLEV_TRUE:
      case CBOR_SIMPLEV_NULL:
      case CBOR_SIMPLEV_UNDEF:
         break;

      case CBOR_SIMPLEV_ONEBYTE:
         if(uNumber <= CBOR_SIMPLE_BREAK) {

            nReturn = QCBOR_ERR_INVALID_CBOR;
            goto Done;
         }


      default:
         pDecodedItem->uDataType   = QCBOR_TYPE_UKNOWN_SIMPLE;

         pDecodedItem->val.uSimple = (uint8_t)uNumber;
         break;

      case HALF_PREC_FLOAT:
         pDecodedItem->val.dfnum  = DecodeHalfFloat(uNumber);
         pDecodedItem->uDataType  = QCBOR_TYPE_DOUBLE;
         break;

      case SINGLE_PREC_FLOAT:
      {
         const uint32_t uTmp = (uint32_t)uNumber;
         memcpy(&pDecodedItem->val.fnum, &uTmp, sizeof(uTmp));
         break;
      }

      case DOUBLE_PREC_FLOAT:
         memcpy(&pDecodedItem->val.dfnum, &uNumber, sizeof(uNumber));
         break;
   }

Done:
   return nReturn;
}



static int DecodeBytes(int nMajorType, uint64_t uNumber, UsefulInputBuf *pUInBuf, QCBORItem *pDecodedItem)
{
   const void *pBytes = UsefulInputBuf_GetBytes(pUInBuf, uNumber);

   int nReturn = QCBOR_ERR_HIT_END;

   if(pBytes != NULL) {
      pDecodedItem->val.string.ptr = pBytes;
      pDecodedItem->val.string.len = uNumber;
      pDecodedItem->uDataType      = (nMajorType == CBOR_MAJOR_TYPE_BYTE_STRING) ? QCBOR_TYPE_BYTE_STRING : QCBOR_TYPE_TEXT_STRING;
      nReturn = QCBOR_SUCCESS;
   }

   return nReturn;
}



static int DecodeDateString(QCBORItem Item, QCBORItem *pDecodedItem)
{
   if(Item.uDataType != QCBOR_TYPE_TEXT_STRING) {
      return QCBOR_ERR_BAD_OPT_TAG;
   }
   pDecodedItem->val.dateString = Item.val.string;
   pDecodedItem->uDataType = QCBOR_TYPE_DATE_STRING;
   pDecodedItem->uTagBits = Item.uTagBits;
   pDecodedItem->uTag = Item.uTag;
   return QCBOR_SUCCESS;
}



static int DecodeBigNum(QCBORItem Item, QCBORItem *pDecodedItem, uint64_t uTagFlags)
{
   if(Item.uDataType != QCBOR_TYPE_BYTE_STRING) {
      return QCBOR_ERR_BAD_OPT_TAG;
   }
   pDecodedItem->val.bigNum     = Item.val.string;
   pDecodedItem->uDataType      = uTagFlags & QCBOR_TAGFLAG_POS_BIGNUM ? QCBOR_TYPE_POSBIGNUM : QCBOR_TYPE_NEGBIGNUM;
   pDecodedItem->uTagBits       = Item.uTagBits;
   pDecodedItem->uTag           = Item.uTag;
   return QCBOR_SUCCESS;
}



static int DecodeDateEpoch(QCBORItem Item, QCBORItem *pDecodedItem)
{
   int nReturn = QCBOR_SUCCESS;

   pDecodedItem->uTagBits                       = Item.uTagBits;
   pDecodedItem->uTag                           = Item.uTag;
   pDecodedItem->uDataType                      = QCBOR_TYPE_DATE_EPOCH;
   pDecodedItem->val.epochDate.fSecondsFraction = 0;

   switch (Item.uDataType) {

      case QCBOR_TYPE_INT64:
         pDecodedItem->val.epochDate.nSeconds = Item.val.int64;
         break;

      case QCBOR_TYPE_UINT64:
         if(Item.val.uint64 > INT64_MAX) {
            nReturn = QCBOR_ERR_DATE_OVERFLOW;
            goto Done;
         }
         pDecodedItem->val.epochDate.nSeconds = Item.val.uint64;
         break;

      case QCBOR_TYPE_FLOAT:
      case QCBOR_TYPE_DOUBLE:
      {
         const double dNum = (Item.uDataType == QCBOR_TYPE_FLOAT) ?
                                 (double)Item.val.fnum : Item.val.dfnum;
         if(dNum > (double)INT64_MAX) {
            nReturn = QCBOR_ERR_DATE_OVERFLOW;
            goto Done;
         }
         pDecodedItem->val.epochDate.nSeconds = (int64_t)dNum;
         pDecodedItem->val.epochDate.fSecondsFraction = dNum - (double)pDecodedItem->val.epochDate.nSeconds;
         break;
      }

      default:
         nReturn = QCBOR_ERR_BAD_OPT_TAG;
   }

Done:
   return nReturn;
}



static int GetAnItem(UsefulInputBuf *pUInBuf, QCBORItem *pDecodedItem, int bCalledFromDecodeOptional);


static int DecodeOptional(UsefulInputBuf *pUInBuf, uint64_t uInputTag, QCBORItem *pDecodedItem)
{
   int      nReturn = QCBOR_SUCCESS;

   uint64_t uTagFlags = 0;
   uint64_t uTagToProcess = uInputTag;


   QCBORItem Item = {0};

   do {
      if(uTagToProcess < 63) {
         uTagFlags |= 0x01LL << uTagToProcess;
      } else if(uTagToProcess == CBOR_TAG_CBOR_MAGIC) {
         uTagFlags |= QCBOR_TAGFLAG_CBOR_MAGIC;
      }


      nReturn = GetAnItem(pUInBuf, &Item, 1);
      if(nReturn) {

         goto Done;
      }

      if(Item.uDataType != QCBOR_TYPE_OPTTAG) {
         break;
      }

      uTagToProcess = Item.uTag;
   } while (1);




   Item.uTagBits = uTagFlags;
   Item.uTag = uTagToProcess;

   switch(uTagFlags & (QCBOR_TAGFLAG_DATE_STRING | QCBOR_TAGFLAG_DATE_EPOCH | QCBOR_TAGFLAG_POS_BIGNUM | QCBOR_TAGFLAG_NEG_BIGNUM)) {
      case 0:

         *pDecodedItem = Item;
         break;

      case QCBOR_TAGFLAG_DATE_STRING:
         nReturn = DecodeDateString(Item, pDecodedItem);
         break;

      case QCBOR_TAGFLAG_DATE_EPOCH:
         nReturn = DecodeDateEpoch(Item, pDecodedItem);
         break;

      case QCBOR_TAGFLAG_POS_BIGNUM:
      case QCBOR_TAGFLAG_NEG_BIGNUM:
         nReturn = DecodeBigNum(Item, pDecodedItem, uTagFlags);
         break;

      default:

         nReturn = QCBOR_ERR_BAD_OPT_TAG;
   }

Done:
   return nReturn;
}



#if QCBOR_TYPE_ARRAY != CBOR_MAJOR_TYPE_ARRAY
#error QCBOR_TYPE_ARRAY value not lined up with major type
#endif
#if QCBOR_TYPE_MAP != CBOR_MAJOR_TYPE_MAP
#error QCBOR_TYPE_MAP value not lined up with major type
#endif


static int GetAnItem(UsefulInputBuf *pUInBuf, QCBORItem *pDecodedItem, int bCalledFromDecodeOptional)
{
   int nReturn;


   int      uMajorType;
   uint64_t uNumber;
   uint8_t  uAdditionalInfo;

   nReturn = DecodeTypeAndNumber(pUInBuf, &uMajorType, &uNumber, &uAdditionalInfo);


   if(nReturn) {
      goto Done;
   }

   pDecodedItem->uTagBits = 0;
   pDecodedItem->uTag     = 0;


   switch (uMajorType) {
      case CBOR_MAJOR_TYPE_POSITIVE_INT:
      case CBOR_MAJOR_TYPE_NEGATIVE_INT:
         nReturn = DecodeInteger(uMajorType, uNumber, pDecodedItem);
         break;

      case CBOR_MAJOR_TYPE_BYTE_STRING:
      case CBOR_MAJOR_TYPE_TEXT_STRING:
         nReturn = DecodeBytes(uMajorType, uNumber, pUInBuf, pDecodedItem);
         break;

      case CBOR_MAJOR_TYPE_ARRAY:
      case CBOR_MAJOR_TYPE_MAP:

         if(uNumber > QCBOR_MAX_ITEMS_IN_ARRAY) {
            nReturn = QCBOR_ERR_ARRAY_TOO_LONG;
            goto Done;
         }
         pDecodedItem->val.uCount = uNumber;
         pDecodedItem->uDataType  = uMajorType;
         break;

      case CBOR_MAJOR_TYPE_OPTIONAL:
         pDecodedItem->uTag      = uNumber;
         pDecodedItem->uDataType = QCBOR_TYPE_OPTTAG;
         if(!bCalledFromDecodeOptional) {

            nReturn = DecodeOptional(pUInBuf, uNumber, pDecodedItem);
         }
         break;

      case CBOR_MAJOR_TYPE_SIMPLE:
         nReturn = DecodeSimple(uAdditionalInfo, uNumber, pDecodedItem);
         break;

      default:
         nReturn = QCBOR_ERR_UNSUPPORTED;
         break;
   }

Done:
   return nReturn;
}



int QCBORDecode_GetNext(QCBORDecodeContext *me, QCBORItem *pDecodedItem)
{
   int nReturn;

   if(!UsefulInputBuf_BytesUnconsumed(&(me->InBuf))) {
      nReturn = QCBOR_ERR_HIT_END;
      goto Done;
   }

   nReturn = GetAnItem(&(me->InBuf), pDecodedItem, 0);
   if(nReturn) {
      goto Done;
   }


   if(DecodeNesting_TypeIsMap(&(me->nesting)) && me->uDecodeMode != QCBOR_DECODE_MODE_MAP_AS_ARRAY) {



      QCBORItem LabelItem = *pDecodedItem;
      nReturn = GetAnItem(&(me->InBuf), pDecodedItem, 0);
      if(nReturn) {
         goto Done;
      }

      if(LabelItem.uDataType == QCBOR_TYPE_TEXT_STRING) {

         pDecodedItem->label.string = LabelItem.val.string;
         pDecodedItem->uLabelType = QCBOR_TYPE_TEXT_STRING;
      } else if (QCBOR_DECODE_MODE_MAP_STRINGS_ONLY == me->uDecodeMode) {

         nReturn = QCBOR_ERR_MAP_LABEL_TYPE;
         goto Done;
      } else if(LabelItem.uDataType == QCBOR_TYPE_INT64) {
         pDecodedItem->label.int64 = LabelItem.val.int64;
         pDecodedItem->uLabelType = QCBOR_TYPE_INT64;
      } else if(LabelItem.uDataType == QCBOR_TYPE_UINT64) {
         pDecodedItem->label.uint64 = LabelItem.val.uint64;
         pDecodedItem->uLabelType = QCBOR_TYPE_UINT64;
      } else if(LabelItem.uDataType == QCBOR_TYPE_BYTE_STRING) {
         pDecodedItem->label.string = LabelItem.val.string;
         pDecodedItem->uLabelType = QCBOR_TYPE_BYTE_STRING;
      } else {

         nReturn = QCBOR_ERR_MAP_LABEL_TYPE;
         goto Done;
      }
   }


   pDecodedItem->uNestingLevel = DecodeNesting_GetLevel(&(me->nesting));


   if(IsMapOrArray(pDecodedItem->uDataType) && pDecodedItem->val.uCount) {
      nReturn = DecodeNesting_Descend(&(me->nesting), pDecodedItem->uDataType, pDecodedItem->val.uCount);
   } else {

      DecodeNesting_Decrement(&(me->nesting));
   }

Done:
   return nReturn;
}



int QCBORDecode_Finish(QCBORDecodeContext *me)
{
   return UsefulInputBuf_BytesUnconsumed(&(me->InBuf)) ? QCBOR_ERR_EXTRA_BYTES : QCBOR_SUCCESS;
}
