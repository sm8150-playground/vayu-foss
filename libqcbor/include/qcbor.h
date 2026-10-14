/*
 * Copyright (c) 2016-2018, The Linux Foundation. All rights reserved.
 */



#ifndef __QCBOR__qcbor__
#define __QCBOR__qcbor__






#include <stdint.h>
#include <stdbool.h>
#include "UsefulBuf.h"



#define QCBOR_MAX_ARRAY_NESTING1 10



typedef struct __QCBORTrackNesting {

   struct {

      uint32_t  uStart;
      uint16_t  uCount;
      uint8_t   uMajorType;
      uint8_t   bBstrWrap;
   } pArrays[QCBOR_MAX_ARRAY_NESTING1+1],
   *pCurrentNesting;
} QCBORTrackNesting;



struct _QCBOREncodeContext {

   UsefulOutBuf      OutBuf;
   uint8_t           uError;
   QCBORTrackNesting nesting;
};



typedef struct __QCBORDecodeNesting  {

   struct {
      uint16_t uCount;
      uint8_t  uMajorType;
   } pMapsAndArrays[QCBOR_MAX_ARRAY_NESTING1+1],
   *pCurrent;
} QCBORDecodeNesting;



struct _QCBORDecodeContext {

   UsefulInputBuf InBuf;

   uint8_t        uDecodeMode;

   QCBORDecodeNesting nesting;
};









#define CBOR_MAJOR_TYPE_POSITIVE_INT 0


#define CBOR_MAJOR_TYPE_NEGATIVE_INT 1


#define CBOR_MAJOR_TYPE_BYTE_STRING  2


#define CBOR_MAJOR_TYPE_TEXT_STRING  3


#define CBOR_MAJOR_TYPE_ARRAY        4


#define CBOR_MAJOR_TYPE_MAP          5


#define CBOR_MAJOR_TYPE_OPTIONAL     6


#define CBOR_MAJOR_TYPE_SIMPLE       7



#define LEN_IS_ONE_BYTE    24
#define LEN_IS_TWO_BYTES   25
#define LEN_IS_FOUR_BYTES  26
#define LEN_IS_EIGHT_BYTES 27
#define ADDINFO_RESERVED1  28
#define ADDINFO_RESERVED2  29
#define ADDINFO_RESERVED3  30
#define LEN_IS_INDEFINITE  31



#define CBOR_TWENTY_FOUR   24




#define CBOR_TAG_DATE_STRING    0

#define CBOR_TAG_DATE_EPOCH     1
#define CBOR_TAG_POS_BIGNUM     2
#define CBOR_TAG_NEG_BIGNUM     3
#define CBOR_TAG_FRACTION       4
#define CBOR_TAG_BIGFLOAT       5

#define CBOR_TAG_ENC_AS_B64URL 21

#define CBOR_TAG_ENC_AS_B64    22

#define CBOR_TAG_ENC_AS_B16    23
#define CBOR_TAG_CBOR          24

#define CBOR_TAG_URI           32

#define CBOR_TAG_B64URL        33

#define CBOR_TAG_B64           34

#define CBOR_TAG_REGEX         35

#define CBOR_TAG_MIME          36

#define CBOR_TAG_BIN_UUID      37

#define CBOR_TAG_CBOR_MAGIC 55799
#define CBOR_TAG_NONE  UINT64_MAX



#define CBOR_SIMPLEV_FALSE   20
#define CBOR_SIMPLEV_TRUE    21
#define CBOR_SIMPLEV_NULL    22
#define CBOR_SIMPLEV_UNDEF   23
#define CBOR_SIMPLEV_ONEBYTE 24
#define HALF_PREC_FLOAT      25
#define SINGLE_PREC_FLOAT    26
#define DOUBLE_PREC_FLOAT    27
#define CBOR_SIMPLE_BREAK    31











typedef struct __EncodedCBORC {
   UsefulBufC Bytes;
   uint16_t   uItems;
} EncodedCBORC;

typedef struct __EncodedCBOR {
   UsefulBuf  Bytes;
   uint16_t   uItems;
} EncodedCBOR;



#define QCBOR_MAX_ITEMS_IN_ARRAY (UINT16_MAX)


#define QCBOR_MAX_ARRAY_NESTING  QCBOR_MAX_ARRAY_NESTING1





#define QCBOR_SUCCESS                     0


#define QCBOR_ERR_BUFFER_TOO_SMALL        1


#define QCBOR_ERR_ARRAY_NESTING_TOO_DEEP  2


#define QCBOR_ERR_ARRAY_TOO_LONG          3


#define QCBOR_ERR_TOO_MANY_CLOSES         4


#define QCBOR_ERR_UNSUPPORTED             5


#define QCBOR_ERR_HIT_END                 6


#define QCBOR_ERR_BUFFER_TOO_LARGE        7


#define QCBOR_ERR_INVALID_SIMPLE          8


#define QCBOR_ERR_INT_OVERFLOW            9


#define QCBOR_ERR_MAP_LABEL_TYPE          10


#define QCBOR_ERR_ARRAY_OR_MAP_STILL_OPEN 11


#define QCBOR_ERR_BAD_SIMPLE              12


#define QCBOR_ERR_DATE_OVERFLOW           13


#define QCBOR_ERR_INVALID_CBOR            14


#define QCBOR_ERR_BAD_OPT_TAG             15


#define QCBOR_ERR_EXTRA_BYTES             16



#define QCBOR_DECODE_MODE_NORMAL            0

#define QCBOR_DECODE_MODE_MAP_STRINGS_ONLY  1

#define QCBOR_DECODE_MODE_MAP_AS_ARRAY      2







#define QCBOR_TYPE_INT64          2

#define QCBOR_TYPE_UINT64         3

#define QCBOR_TYPE_ARRAY          4

#define QCBOR_TYPE_MAP            5

#define QCBOR_TYPE_BYTE_STRING    6

#define QCBOR_TYPE_TEXT_STRING    7

#define QCBOR_TYPE_FLOAT         26

#define QCBOR_TYPE_DOUBLE        27

#define QCBOR_TYPE_POSBIGNUM     9

#define QCBOR_TYPE_NEGBIGNUM     10

#define QCBOR_TYPE_DATE_STRING   11

#define QCBOR_TYPE_DATE_EPOCH    12

#define QCBOR_TYPE_UKNOWN_SIMPLE 13

#define QCBOR_TYPE_FALSE         20

#define QCBOR_TYPE_TRUE          21

#define QCBOR_TYPE_NULL          22

#define QCBOR_TYPE_UNDEF         23


#define QCBOR_TYPE_OPTTAG     254
#define QCBOR_TYPE_BREAK      255






typedef struct _QCBORItem {
   uint8_t  uDataType;
   uint8_t  uNestingLevel;
   uint8_t  uLabelType;

   union {
      int64_t     int64;
      uint64_t    uint64;

      UsefulBufC  string;
      uint16_t    uCount;
      float       fnum;
      double      dfnum;
      struct {
         int64_t  nSeconds;
         double   fSecondsFraction;
      } epochDate;
      UsefulBufC  dateString;
      UsefulBufC  bigNum;
      uint8_t     uSimple;

   } val;

   union {
      UsefulBufC  string;
      int64_t     int64;
      uint64_t    uint64;
   } label;

   uint64_t uTag;
   uint64_t uTagBits;

} QCBORItem;



#define QCBOR_TAGFLAG_DATE_STRING    (0x01LL << CBOR_TAG_DATE_STRING)
#define QCBOR_TAGFLAG_DATE_EPOCH     (0x01LL << CBOR_TAG_DATE_EPOCH)
#define QCBOR_TAGFLAG_POS_BIGNUM     (0x01LL << CBOR_TAG_POS_BIGNUM)
#define QCBOR_TAGFLAG_NEG_BIGNUM     (0x01LL << CBOR_TAG_NEG_BIGNUM)
#define QCBOR_TAGFLAG_FRACTION       (0x01LL << CBOR_TAG_FRACTION)
#define QCBOR_TAGFLAG_BIGFLOAT       (0x01LL << CBOR_TAG_BIGFLOAT)
#define QCBOR_TAGFLAG_ENC_AS_B64URL  (0x01LL << CBOR_TAG_ENC_AS_B64URL)
#define QCBOR_TAGFLAG_ENC_AS_B64     (0x01LL << CBOR_TAG_ENC_AS_B64)
#define QCBOR_TAGFLAG_ENC_AS_B16     (0x01LL << CBOR_TAG_ENC_AS_B16)
#define QCBOR_TAGFLAG_CBOR           (0x01LL << CBOR_TAG_CBOR)
#define QCBOR_TAGFLAG_URI            (0x01LL << CBOR_TAG_URI)
#define QCBOR_TAGFLAG_B64URL         (0x01LL << CBOR_TAG_B64URL)
#define QCBOR_TAGFLAG_B64            (0x01LL << CBOR_TAG_B64)
#define QCBOR_TAGFLAG_REGEX          (0x01LL << CBOR_TAG_REGEX)
#define QCBOR_TAGFLAG_MIME           (0x01LL << CBOR_TAG_MIME)
#define QCBOR_TAGFLAG_CBOR_MAGIC     (0x01ULL << 63)



#define QCBOR_NO_INT_LABEL           INT64_MAX



static inline EncodedCBORC EncodedCBORConst(const EncodedCBOR ECBOR)
{
   return (EncodedCBORC){UsefulBufConst(ECBOR.Bytes), ECBOR.uItems};
}



typedef struct _QCBOREncodeContext QCBOREncodeContext;




void QCBOREncode_Init(QCBOREncodeContext *pCtx, void *pBuf, size_t uBufLen);






void QCBOREncode_AddInt64_3(QCBOREncodeContext *pCtx, const char *szLabel, int64_t nLabel, uint64_t uTag, int64_t nNum);
void QCBOREncode_AddUInt64_3(QCBOREncodeContext *pCtx, const char *szLabel, int64_t nLabel, uint64_t uTag, uint64_t uNum);

#define QCBOREncode_AddUInt64(pCtx, uNum) \
      QCBOREncode_AddUInt64_3((pCtx), NULL, QCBOR_NO_INT_LABEL, CBOR_TAG_NONE, (uNum))

#define QCBOREncode_AddUInt64ToMap(pCtx, szLabel, uNum) \
      QCBOREncode_AddUInt64_3((pCtx), (szLabel), QCBOR_NO_INT_LABEL, CBOR_TAG_NONE, (uNum))

#define QCBOREncode_AddUInt64ToMapN(pCtx, nLabel, uNum) \
      QCBOREncode_AddUInt64_3((pCtx), NULL, (nLabel), CBOR_TAG_NONE, (uNum))

#define QCBOREncode_AddInt64(pCtx, nNum) \
      QCBOREncode_AddInt64_3((pCtx), NULL, QCBOR_NO_INT_LABEL, CBOR_TAG_NONE, (nNum))

#define QCBOREncode_AddInt64ToMap(pCtx, szLabel, nNum) \
      QCBOREncode_AddInt64_3((pCtx), (szLabel), QCBOR_NO_INT_LABEL, CBOR_TAG_NONE, (nNum))

#define QCBOREncode_AddInt64ToMapN(pCtx, nLabel, nNum) \
      QCBOREncode_AddInt64_3((pCtx), NULL, (nLabel), CBOR_TAG_NONE, (nNum))





void QCBOREncode_AddFloat_3(QCBOREncodeContext *pCtx, const char *szLabel, int64_t nLabel, uint64_t uTag, float fNum);
void QCBOREncode_AddDouble_3(QCBOREncodeContext *pCtx, const char *szLabel, int64_t nLabel, uint64_t uTag, double dNum);

#define QCBOREncode_AddFloat(pCtx, fNum) \
      QCBOREncode_AddFloat_3((pCtx), NULL, QCBOR_NO_INT_LABEL, CBOR_TAG_NONE, (fNum))

#define QCBOREncode_AddFloatToMap(pCtx, szLabel, fNum) \
      QCBOREncode_AddFloat_3((pCtx), (szLabel), QCBOR_NO_INT_LABEL, CBOR_TAG_NONE, (fNum))

#define QCBOREncode_AddFloatToMapN(pCtx, nLabel, fNum) \
      QCBOREncode_AddFloat_3((pCtx), NULL, (nLabel), CBOR_TAG_NONE, (fNum))

#define QCBOREncode_AddDouble(pCtx, dNum) \
      QCBOREncode_AddDouble_3((pCtx), NULL, QCBOR_NO_INT_LABEL, CBOR_TAG_NONE, (dNum))

#define QCBOREncode_AddDoubleToMap(pCtx, szLabel, dNum) \
      QCBOREncode_AddDouble_3((pCtx), (szLabel), QCBOR_NO_INT_LABEL, CBOR_TAG_NONE, (dNum))

#define QCBOREncode_AddDoubleToMapN(pCtx, nLabel, dNum) \
      QCBOREncode_AddDouble_3((pCtx), NULL, (nLabel), CBOR_TAG_NONE, (dNum))





static inline void QCBOREncode_AddDateEpoch_2(QCBOREncodeContext *pCtx, const char *szLabel, uint64_t nLabel, int64_t date)
{
   QCBOREncode_AddInt64_3(pCtx, szLabel, nLabel, CBOR_TAG_DATE_EPOCH, date);
}

#define QCBOREncode_AddDateEpoch(pCtx, date) \
      QCBOREncode_AddDateEpoch_2((pCtx), NULL, QCBOR_NO_INT_LABEL, (date))

#define QCBOREncode_AddDateEpochToMap(pCtx, szLabel, date) \
      QCBOREncode_AddDateEpoch_2((pCtx), (szLabel), QCBOR_NO_INT_LABEL, (date))

#define QCBOREncode_AddDateEpochToMapN(pCtx, nLabel, date) \
      QCBOREncode_AddDateEpoch_2((pCtx), NULL, (nLabel), (date))






void QCBOREncode_AddBytes_3(QCBOREncodeContext *pCtx, const char *szLabel, int64_t nLabel, uint64_t uTag, UsefulBufC Bytes);

#define QCBOREncode_AddBytes(pCtx, Bytes) \
      QCBOREncode_AddBytes_3((pCtx), NULL, QCBOR_NO_INT_LABEL, CBOR_TAG_NONE, (Bytes))

#define QCBOREncode_AddBytesToMap(pCtx, szLabel, Bytes) \
      QCBOREncode_AddBytes_3((pCtx), (szLabel), QCBOR_NO_INT_LABEL, CBOR_TAG_NONE, (Bytes))

#define QCBOREncode_AddBytesToMapN(pCtx, nLabel, Bytes) \
      QCBOREncode_AddBytes_3((pCtx), NULL, (nLabel), CBOR_TAG_NONE, (Bytes))


#define QCBOREncode_AddBinaryUUID(pCtx, Bytes) \
      QCBOREncode_AddBytes_3((pCtx), NULL, QCBOR_NO_INT_LABEL, CBOR_TAG_BIN_UUID, (Bytes))

#define QCBOREncode_AddBinaryUUIDToMap(pCtx, szLabel, Bytes) \
      QCBOREncode_AddBytes_3((pCtx), (szLabel), QCBOR_NO_INT_LABEL, CBOR_TAG_BIN_UUID, (Bytes))

#define QCBOREncode_AddBinaryUUIDToMapN(pCtx, nLabel, Bytes) \
      QCBOREncode_AddBytes_3((pCtx), NULL, (nLabel), CBOR_TAG_BIN_UUID, (Bytes))


#define QCBOREncode_AddPositiveBignum(pCtx, Bytes) \
      QCBOREncode_AddBytes_3((pCtx), NULL, QCBOR_NO_INT_LABEL, CBOR_TAG_POS_BIGNUM, (Bytes))

#define QCBOREncode_AddPositiveBignumToMap(pCtx, szLabel, Bytes) \
      QCBOREncode_AddBytes_3((pCtx), (szLabel), QCBOR_NO_INT_LABEL, CBOR_TAG_POS_BIGNUM, (Bytes))

#define QCBOREncode_AddPositiveBignumToMapN(pCtx, nLabel, Bytes) \
      QCBOREncode_AddBytes_3((pCtx), NULL, (nLabel), CBOR_TAG_POS_BIGNUM, (Bytes))


#define QCBOREncode_AddNegativeBignum(pCtx, Bytes) \
      QCBOREncode_AddBytes_3((pCtx), NULL, QCBOR_NO_INT_LABEL, CBOR_TAG_NEG_BIGNUM, (Bytes))

#define QCBOREncode_AddNegativeBignumToMap(pCtx, szLabel, Bytes) \
      QCBOREncode_AddBytes_3((pCtx), (szLabel), QCBOR_NO_INT_LABEL, CBOR_TAG_NEG_BIGNUM, (Bytes))

#define QCBOREncode_AddNegativeBignumToMapN(pCtx, nLabel, Bytes) \
      QCBOREncode_AddBytes_3((pCtx), NULL, (nLabel), CBOR_TAG_NEG_BIGNUM, (Bytes))





void QCBOREncode_AddText_3(QCBOREncodeContext *pCtx, const char *szLabel, int64_t nLabel, uint64_t uTag, UsefulBufC Bytes);

#define QCBOREncode_AddText(pCtx, Bytes) \
      QCBOREncode_AddText_3((pCtx), NULL, QCBOR_NO_INT_LABEL, CBOR_TAG_NONE, (Bytes))

#define QCBOREncode_AddTextToMap(pCtx, szLabel, Bytes) \
      QCBOREncode_AddText_3((pCtx), (szLabel), QCBOR_NO_INT_LABEL, CBOR_TAG_NONE, (Bytes))

#define QCBOREncode_AddTextToMapN(pCtx, nLabel, Bytes) \
      QCBOREncode_AddText_3((pCtx), NULL, (nLabel), CBOR_TAG_NONE, (Bytes))

inline static void QCBOREncode_AddSZString_3(QCBOREncodeContext *pCtx, const char *szLabel, int64_t nLabel, uint64_t uTag, const char *szString) {
   QCBOREncode_AddText_3(pCtx, szLabel, nLabel, uTag, SZToUsefulBufC(szString));
}

#define QCBOREncode_AddSZString(pCtx, szString) \
      QCBOREncode_AddSZString_3((pCtx), NULL, QCBOR_NO_INT_LABEL, CBOR_TAG_NONE, (szString))

#define QCBOREncode_AddSZStringToMap(pCtx, szLabel, szString) \
      QCBOREncode_AddSZString_3((pCtx), (szLabel), QCBOR_NO_INT_LABEL, CBOR_TAG_NONE, (szString))

#define QCBOREncode_AddSZStringToMapN(pCtx, nLabel, szString) \
      QCBOREncode_AddSZString_3((pCtx), NULL, (nLabel), CBOR_TAG_NONE, (szString))

#define QCBOREncode_AddURI(pCtx, Bytes) \
      QCBOREncode_AddText_3((pCtx), NULL, QCBOR_NO_INT_LABEL, CBOR_TAG_URI, (Bytes))

#define QCBOREncode_AddURIToMap(pCtx, szLabel, Bytes) \
      QCBOREncode_AddText_3((pCtx), (szLabel), QCBOR_NO_INT_LABEL, CBOR_TAG_URI, (Bytes))

#define QCBOREncode_AddURIToMapN(pCtx, nLabel, Bytes) \
      QCBOREncode_AddText_3((pCtx), NULL, (nLabel), CBOR_TAG_URI, (Bytes))

#define QCBOREncode_AddB64Text(pCtx, Bytes) \
      QCBOREncode_AddText_3((pCtx), NULL, QCBOR_NO_INT_LABEL, CBOR_TAG_B64, (Bytes))

#define QCBOREncode_AddB64TextToMap(pCtx, szLabel, Bytes) \
      QCBOREncode_AddText_3((pCtx), (szLabel), QCBOR_NO_INT_LABEL, CBOR_TAG_B64, (Bytes))

#define QCBOREncode_AddB64TextToMapN(pCtx, nLabel, Bytes) \
      QCBOREncode_AddText_3((pCtx), NULL, (nLabel), CBOR_TAG_B64, (Bytes))

#define QCBOREncode_AddB64URLText(pCtx, Bytes) \
      QCBOREncode_AddText_3((pCtx), NULL, QCBOR_NO_INT_LABEL, CBOR_TAG_B64URL, (Bytes))

#define QCBOREncode_AddB64URLTextToMap(pCtx, szLabel, Bytes) \
      QCBOREncode_AddText_3((pCtx), (szLabel), QCBOR_NO_INT_LABEL, CBOR_TAG_B64URL, (Bytes))

#define QCBOREncode_AddB64URLTextToMapN(pCtx, nLabel, Bytes) \
      QCBOREncode_AddText_3((pCtx), NULL, (nLabel), CBOR_TAG_B64URL, (Bytes))

#define QCBOREncode_AddRegex(pCtx, Bytes) \
      QCBOREncode_AddText_3((pCtx), NULL, QCBOR_NO_INT_LABEL, CBOR_TAG_REGEX, (Bytes))

#define QCBOREncode_AddRegexToMap(pCtx, szLabel, Bytes) \
      QCBOREncode_AddText_3((pCtx), (szLabel), QCBOR_NO_INT_LABEL, CBOR_TAG_REGEX, (Bytes))

#define QCBOREncode_AddRegexToMapN(pCtx, nLabel, Bytes) \
      QCBOREncode_AddText_3((pCtx), NULL, (nLabel), CBOR_TAG_REGEX, (Bytes))

#define QCBOREncode_AddMIMEData(pCtx, Bytes) \
      QCBOREncode_AddText_3((pCtx), NULL, QCBOR_NO_INT_LABEL, CBOR_TAG_MIME, (Bytes))

#define QCBOREncode_AddMIMEDataToMap(pCtx, szLabel, Bytes) \
      QCBOREncode_AddText_3((pCtx), (szLabel), QCBOR_NO_INT_LABEL, CBOR_TAG_MIME, (Bytes))

#define QCBOREncode_AddMIMEDataToMapN(pCtx, nLabel, Bytes) \
      QCBOREncode_AddText_3((pCtx), NULL, (nLabel), CBOR_TAG_MIME, (Bytes))





#define QCBOREncode_AddDateString(pCtx, szDate) \
      QCBOREncode_AddSZString_3((pCtx), NULL, QCBOR_NO_INT_LABEL, CBOR_TAG_DATE_STRING, (szDate))

#define QCBOREncode_AddDateStringToMap(pCtx, szLabel, szDate)  \
      QCBOREncode_AddSZString_3(pCtx, (szLabel), QCBOR_NO_INT_LABEL, CBOR_TAG_DATE_STRING, (szDate))

#define QCBOREncode_AddDateStringToMapN(pCtx, nLabel, szDate)  \
      QCBOREncode_AddSZString_3(pCtx, NULL, (nLabel), CBOR_TAG_DATE_STRING, (szDate))





void QCBOREncode_AddSimple_3(QCBOREncodeContext *pCtx, const char *szLabel, int64_t nLabel, uint64_t uTag, uint8_t uSimple);

#define QCBOREncode_AddSimple(pCtx, uSimple) \
      QCBOREncode_AddSimple_3((pCtx), NULL,  QCBOR_NO_INT_LABEL, CBOR_TAG_NONE, (uSimple))

#define QCBOREncode_AddSimpleToMap(pCtx, szLabel, uSimple) \
      QCBOREncode_AddSimple_3((pCtx), (szLabel),  QCBOR_NO_INT_LABEL, CBOR_TAG_NONE, (uSimple))

#define QCBOREncode_AddSimpleToMapN(pCtx, nLabel, uSimple) \
      QCBOREncode_AddSimple_3((pCtx), NULL, (nLabel), CBOR_TAG_NONE, (uSimple))




inline static void QCBOREncode_AddBool_3(QCBOREncodeContext *pCtx, const char *szLabel, int64_t nLabel, uint64_t uTag, bool b) {
   uint8_t uSimple = CBOR_SIMPLE_BREAK;
   if(b == true || b == false)
      uSimple = CBOR_SIMPLEV_FALSE + b;;
   QCBOREncode_AddSimple_3(pCtx, szLabel, nLabel, uTag, uSimple);
}

#define QCBOREncode_AddBool(pCtx, bool) \
   QCBOREncode_AddBool_3((pCtx), NULL,  QCBOR_NO_INT_LABEL, CBOR_TAG_NONE, (bool))

#define QCBOREncode_AddBoolToMap(pCtx, szLabel, bool) \
   QCBOREncode_AddBool_3((pCtx), (szLabel),  QCBOR_NO_INT_LABEL, CBOR_TAG_NONE, (bool))

#define QCBOREncode_AddBoolToMapN(pCtx, nLabel, bool) \
   QCBOREncode_AddBool_3((pCtx), NULL, (nLabel), CBOR_TAG_NONE, (bool))




void QCBOREncode_OpenArray_3(QCBOREncodeContext *pCtx, const char *szLabel, uint64_t nLabel, uint64_t uTag, bool bBstrWrap);

#define QCBOREncode_OpenArray(pCtx) \
      QCBOREncode_OpenArray_3((pCtx), NULL, QCBOR_NO_INT_LABEL, CBOR_TAG_NONE, 0)

#define QCBOREncode_OpenArrayInMap(pCtx, szLabel) \
      QCBOREncode_OpenArray_3((pCtx), (szLabel), QCBOR_NO_INT_LABEL, CBOR_TAG_NONE, 0)

#define QCBOREncode_OpenArrayInMapN(pCtx, nLabel) \
      QCBOREncode_OpenArray_3((pCtx), NULL, (nLabel), CBOR_TAG_NONE, 0)




void QCBOREncode_OpenMap_3(QCBOREncodeContext *pCtx, const char *szLabel,  uint64_t nLabel, uint64_t uTag, uint8_t bBstrWrap);

#define QCBOREncode_OpenMap(pCtx) \
      QCBOREncode_OpenMap_3((pCtx), NULL, QCBOR_NO_INT_LABEL, CBOR_TAG_NONE, 0)

#define QCBOREncode_OpenMapInMap(pCtx, szLabel) \
      QCBOREncode_OpenMap_3((pCtx), (szLabel), QCBOR_NO_INT_LABEL, CBOR_TAG_NONE, 0)

#define QCBOREncode_OpenMapInMapN(pCtx, nLabel) \
      QCBOREncode_OpenMap_3((pCtx), NULL, (nLabel), CBOR_TAG_NONE, 0)




void QCBOREncode_CloseArray(QCBOREncodeContext *pCtx);

#define QCBOREncode_CloseMap(pCtx)  QCBOREncode_CloseArray(pCtx)





void QCBOREncode_AddRaw(QCBOREncodeContext *pCtx, EncodedCBORC Encoded);





void QCBOREncode_AddRawSimple_3(QCBOREncodeContext *pCtx, const char *szLabel, int64_t nLabel, uint64_t uTag, uint8_t uSimple);





int QCBOREncode_Finish(QCBOREncodeContext *pCtx, size_t *uEncodedLen);





int QCBOREncode_Finish2(QCBOREncodeContext *pCtx, EncodedCBOR *pEncodedCBOR);








typedef struct _QCBORDecodeContext QCBORDecodeContext;




void QCBORDecode_Init(QCBORDecodeContext *pCtx, UsefulBufC EncodedCBOR, int8_t nMode);




int QCBORDecode_GetNext(QCBORDecodeContext *pCtx, QCBORItem *pDecodedItem);




int QCBORDecode_Finish(QCBORDecodeContext *pCtx);





static inline int QCBOR_Int64ToInt32(int64_t src, int32_t *dest)
{
   if(src > INT32_MAX || src < INT32_MIN) {
      return -1;
   } else {
      *dest = (int32_t) src;
   }
   return 0;
}

static inline int QCBOR_Int64ToInt16(int64_t src, int16_t *dest)
{
   if(src > INT16_MAX || src < INT16_MIN) {
      return -1;
   } else {
      *dest = (int16_t) src;
   }
   return 0;
}

static inline int QCBOR_Int64ToInt8(int64_t src, int8_t *dest)
{
   if(src > INT8_MAX || src < INT8_MIN) {
      return -1;
   } else {
      *dest = (int8_t) src;
   }
   return 0;
}

static inline int QCBOR_Int64ToUInt32(int64_t src, uint32_t *dest)
{
   if(src > UINT32_MAX || src < 0) {
      return -1;
   } else {
      *dest = (uint32_t) src;
   }
   return 0;
}

static inline int QCBOR_Int64UToInt16(int64_t src, uint16_t *dest)
{
   if(src > UINT16_MAX || src < 0) {
      return -1;
   } else {
      *dest = (uint16_t) src;
   }
   return 0;
}

static inline int QCBOR_Int64ToUInt8(int64_t src, uint8_t *dest)
{
   if(src > UINT8_MAX || src < 0) {
      return -1;
   } else {
      *dest = (uint8_t) src;
   }
   return 0;
}

static inline int QCBOR_Int64ToUInt64(int64_t src, uint64_t *dest)
{
   if(src > 0) {
      return -1;
   } else {
      *dest = (uint64_t) src;
   }
   return 0;
}



#endif

