/*
 * Copyright (c) 2016-2018, The Linux Foundation. All rights reserved.
 */



#ifndef _UsefulBuf_h
#define _UsefulBuf_h


#include <stdint.h>
#include <string.h>
#include <stddef.h>






typedef struct __UsefulBuf {
   void  *ptr;
   size_t len;
} UsefulBuf;


typedef struct __UsefulBufC {
   const void *ptr;
   size_t      len;
} UsefulBufC;






static inline UsefulBufC UsefulBuf_Const(const UsefulBuf UB)
{
   return (UsefulBufC){UB.ptr, UB.len};
}


static inline UsefulBufC UsefulBufConst(const UsefulBuf UB)
{
   return (UsefulBufC){UB.ptr, UB.len};
}



static inline UsefulBuf UsefulBufC_Unconst(const UsefulBufC UBC)
{
   return (UsefulBuf){(void *)UBC.ptr, UBC.len};
}



#define UsefulBuf_IsNULL(UB)  (!(UB).ptr)

#define NULLUsefulBufC        ((UsefulBufC) {NULL, 0})

#define NULLUsefulBuf         ((UsefulBuf) {NULL, 0})




#define UsefulBuf_IsEmpty(UB)  (!(UB).len)

#define UsefulBuf_IsNULLOrEmpty(UB) (UsefulBuf_IsNULL(UB) || UsefulBuf_IsEmpty(UB))





static inline UsefulBufC SZToUsefulBufC(const char *szString){
   return ((UsefulBufC) {szString, strlen(szString)});
}



int UsefulBuf_Copy(UsefulBuf *pDest, const UsefulBufC Src);




void UsefulBuf_Set(UsefulBuf *pDest, uint8_t value);




inline static int UsefulBuf_CopyPtr(UsefulBuf *pDest, const void *ptr, size_t len)
{
   return UsefulBuf_Copy(pDest, (UsefulBufC){ptr, len});
}



int UsefulBuf_Compare(const UsefulBufC UB1, const UsefulBufC UB2);



size_t UsefulBuf_FindBytes(UsefulBufC BytesToSearch, UsefulBufC BytesToFind);




#define SZLiteralToUsefulBufC(szString) \
      ((UsefulBufC) {(szString), sizeof(szString)-1})



#define ByteArrayLiteralToUsefulBufC(pBytes) \
      ((UsefulBufC) {(pBytes), sizeof(pBytes)})




#define  MakeUsefulBufOnStack(name, size) \
   uint8_t    __pBuf##name[(size)];\
   UsefulBuf  name = {__pBuf##name , sizeof( __pBuf##name )}






typedef struct __UsefulOutBuf {
   UsefulBuf  UB;
   size_t     size;
   uint16_t   magic;
   uint8_t    err;
} UsefulOutBuf;



void UsefulOutBuf_Init(UsefulOutBuf *me, void *pStorage, size_t uStorageSize);




#define  MakeUsefulOutBufOnStack(name, size) \
   uint8_t       __pBuf##name[(size)];\
   UsefulOutBuf  name;\
   UsefulOutBuf_Init(&(name), __pBuf##name, (size));




static inline void UsefulOutBuf_Reset(UsefulOutBuf *me)
{
   me->UB.len = 0;
   me->err    = 0;
}



static inline size_t UsefulOutBuf_GetEndPosition(UsefulOutBuf *me)
{
   return me->UB.len;
}



static inline int UsefulOutBuf_AtStart(UsefulOutBuf *me)
{
   return 0 == me->UB.len;
}



void UsefulOutBuf_InsertUsefulBuf(UsefulOutBuf *me, UsefulBufC NewData, size_t uPos);




static inline void UsefulOutBuf_InsertData(UsefulOutBuf *me, const void *pBytes, size_t uLen, size_t uPos)
{
   UsefulBufC Data = {pBytes, uLen};
   UsefulOutBuf_InsertUsefulBuf(me, Data, uPos);
}



static inline void UsefulOutBuf_InsertString(UsefulOutBuf *me, const char *szString, size_t uPos)
{
   UsefulOutBuf_InsertUsefulBuf(me, (UsefulBufC){szString, strlen(szString)}, uPos);
}



static inline void UsefulOutBuf_InsertByte(UsefulOutBuf *me, uint8_t byte, size_t uPos)
{
   UsefulOutBuf_InsertData(me, &byte, 1, uPos);
}



static inline void UsefulOutBuf_InsertUint16(UsefulOutBuf *me, uint16_t uInteger16, size_t uPos)
{

   uint8_t tmp[2];
   tmp[0] = (uInteger16 & 0xff00) >> 8;
   tmp[1] = (uInteger16 & 0xff);
   UsefulOutBuf_InsertData(me, tmp, 2, uPos);
}



static inline void UsefulOutBuf_InsertUint32(UsefulOutBuf *me, uint32_t uInteger32, size_t uPos)
{

   uint8_t tmp[4];
   tmp[0] = (uInteger32 & 0xff000000) >> 24;
   tmp[1] = (uInteger32 & 0xff0000) >> 16;
   tmp[2] = (uInteger32 & 0xff00) >> 8;
   tmp[3] = (uInteger32 & 0xff);
   UsefulOutBuf_InsertData(me, tmp, 4, uPos);
}



static inline void UsefulOutBuf_InsertUint64(UsefulOutBuf *me, uint64_t uInteger64, size_t uPos)
{

   uint8_t tmp[8];
   tmp[0] = (uInteger64 & 0xff00000000000000) >> 56;
   tmp[1] = (uInteger64 & 0xff000000000000) >> 48;
   tmp[2] = (uInteger64 & 0xff0000000000) >> 40;
   tmp[3] = (uInteger64 & 0xff00000000) >> 32;
   tmp[4] = (uInteger64 & 0xff000000) >> 24;
   tmp[5] = (uInteger64 & 0xff0000) >> 16;
   tmp[6] = (uInteger64 & 0xff00) >> 8;
   tmp[7] = (uInteger64 & 0xff);
   UsefulOutBuf_InsertData(me, tmp, 8, uPos);
}



static inline void UsefulOutBuf_InsertFloat(UsefulOutBuf *me, float f, size_t uPos)
{


   UsefulOutBuf_InsertUint32(me, *(uint32_t *)&f, uPos);
}



static inline void UsefulOutBuf_InsertDouble(UsefulOutBuf *me, double d, size_t uPos)
{

   UsefulOutBuf_InsertUint64(me, *(uint64_t *)&d, uPos);
}




static inline void UsefulOutBuf_AppendUsefulBuf(UsefulOutBuf *me, UsefulBufC NewData)
{

   UsefulOutBuf_InsertUsefulBuf(me, NewData, UsefulOutBuf_GetEndPosition(me));
}




static inline void UsefulOutBuf_AppendData(UsefulOutBuf *me, const void *pBytes, size_t uLen)
{
   UsefulBufC Data = {pBytes, uLen};
   UsefulOutBuf_AppendUsefulBuf(me, Data);
}



static inline void UsefulOutBuf_AppendString(UsefulOutBuf *me, const char *szString)
{
   UsefulOutBuf_AppendUsefulBuf(me, (UsefulBufC){szString, strlen(szString)});
}



static inline void UsefulOutBuf_AppendByte(UsefulOutBuf *me, uint8_t byte)
{
   UsefulOutBuf_AppendData(me, &byte, 1);
}


static inline void UsefulOutBuf_AppendUint16(UsefulOutBuf *me, uint16_t uInteger16){
   UsefulOutBuf_InsertUint16(me, uInteger16, UsefulOutBuf_GetEndPosition(me));
}


static inline void UsefulOutBuf_AppendUint32(UsefulOutBuf *me, uint32_t uInteger32){
   UsefulOutBuf_InsertUint32(me, uInteger32, UsefulOutBuf_GetEndPosition(me));
}


static inline void UsefulOutBuf_AppendUint64(UsefulOutBuf *me, uint64_t uInteger64){
   UsefulOutBuf_InsertUint64(me, uInteger64, UsefulOutBuf_GetEndPosition(me));
}



static inline void UsefulOutBuf_AppendFloat(UsefulOutBuf *me, float f){
   UsefulOutBuf_InsertFloat(me, f, UsefulOutBuf_GetEndPosition(me));
}


static inline void UsefulOutBuf_AppendDouble(UsefulOutBuf *me, double d){
   UsefulOutBuf_InsertDouble(me, d, UsefulOutBuf_GetEndPosition(me));
}



static inline int UsefulOutBuf_GetError(UsefulOutBuf *me)
{
   return me->err;
}




static inline size_t UsefulOutBuf_RoomLeft(UsefulOutBuf *me)
{
   return me->size - me->UB.len;
}




static inline int UsefulOutBuf_WillItFit(UsefulOutBuf *me, size_t uLen)
{
   return uLen <= UsefulOutBuf_RoomLeft(me);
}




int UsefulOutBuf_OutUBuf(UsefulOutBuf *me, UsefulBuf *O);




int UsefulOutBuf_CopyOut(UsefulOutBuf *me, void *pBuf, size_t uBufSize, size_t *puCopied);















#define UIB_MAGIC (0xB00F)

typedef struct __UsefulInputBuf {
   UsefulBufC UB;
   size_t     cursor;
   uint16_t   magic;
   uint8_t    err;
} UsefulInputBuf;





static inline void UsefulInputBuf_Init(UsefulInputBuf *me, UsefulBufC UB)
{
   me->cursor = 0;
   me->err    = 0;
   me->magic  = UIB_MAGIC;
   me->UB     = UB;
}



static inline size_t UsefulInputBuf_Tell(UsefulInputBuf *me) {
   return me->cursor;
}




static inline void UsefulInputBuf_Seek(UsefulInputBuf *me, size_t uPos)
{
   if(uPos > me->UB.len) {
      me->err = 1;
   } else {
      me->cursor = uPos;
   }
}



static inline size_t UsefulInputBuf_BytesUnconsumed(UsefulInputBuf *me)
{


   if(me->magic != UIB_MAGIC)
      return 0;





   if(me->cursor > me->UB.len)
      return 0;


   return me->UB.len - me->cursor;
}




static inline int UsefulInputBuf_BytesAvailable(UsefulInputBuf *me, size_t uLen)
{
   return UsefulInputBuf_BytesUnconsumed(me) >= uLen ? 1 : 0;
}






const void * UsefulInputBuf_GetBytes(UsefulInputBuf *me, size_t uNum);



static inline UsefulBufC UsefulInputBuf_GetUsefulBuf(UsefulInputBuf *me, size_t uNum)
{
   const void *pResult = UsefulInputBuf_GetBytes(me, uNum);

   UsefulBufC UBR = {NULL, 0};

   if(pResult) {
      UBR.len = uNum;
      UBR.ptr = pResult;
   }

   return UBR;
}



static inline uint8_t UsefulInputBuf_GetByte(UsefulInputBuf *me)
{
   const void *pResult = UsefulInputBuf_GetBytes(me, sizeof(uint8_t));

   return pResult ? *(uint8_t *)pResult : 0;
}


static inline uint16_t UsefulInputBuf_GetUint16(UsefulInputBuf *me)
{
   const uint8_t *pResult = (const uint8_t *)UsefulInputBuf_GetBytes(me, sizeof(uint16_t));

   if(!pResult) {
      return 0;
   }

   return  ((uint16_t)pResult[0] << 8) + (uint16_t)pResult[1];
}


static inline uint32_t UsefulInputBuf_GetUint32(UsefulInputBuf *me)
{
   const uint8_t *pResult = (const uint8_t *)UsefulInputBuf_GetBytes(me, sizeof(uint32_t));

   if(!pResult) {
      return 0;
   }

   return ((uint32_t)pResult[0]<<24) + ((uint32_t)pResult[1]<<16) + ((uint32_t)pResult[2]<<8) + (uint32_t)pResult[3];
}


static inline uint64_t UsefulInputBuf_GetUint64(UsefulInputBuf *me)
{
   const uint8_t *pResult = (const uint8_t *)UsefulInputBuf_GetBytes(me, sizeof(uint64_t));

   if(!pResult) {
      return 0;
   }

   return   ((uint64_t)pResult[0]<<56) +
            ((uint64_t)pResult[1]<<48) +
            ((uint64_t)pResult[2]<<40) +
            ((uint64_t)pResult[3]<<32) +
            ((uint64_t)pResult[4]<<24) +
            ((uint64_t)pResult[5]<<16) +
            ((uint64_t)pResult[6]<<8)  +
            (uint64_t)pResult[7];
}



static inline float UsefulInputBuf_GetFloat(UsefulInputBuf *me)
{
   uint32_t uResult = UsefulInputBuf_GetUint32(me);

   return uResult ? *(float *)&uResult : 0;

}


static inline double UsefulInputBuf_GetDouble(UsefulInputBuf *me)
{
   uint64_t uResult = UsefulInputBuf_GetUint64(me);

   return uResult ? *(double *)&uResult : 0;
}




static inline int UsefulInputBuf_GetError(UsefulInputBuf *me)
{
   return me->err;
}


#endif


