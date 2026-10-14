/*
 * Copyright (C) 2016-2018, The Linux Foundation. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "UsefulBuf.h"

#include <string.h>



#define USEFUL_OUT_BUF_MAGIC  (0x0B0F)



int UsefulBuf_Copy(UsefulBuf *pDest, const UsefulBufC Src)
{
   if(Src.len > pDest->len) {
      return 1;
   }


   memcpy(pDest->ptr, Src.ptr, Src.len);

   pDest->len = Src.len;

   return 0;
}



int UsefulBuf_Compare(const UsefulBufC UB1, const UsefulBufC UB2)
{

   if(UB1.len < UB2.len) {
      return -1;
   } else if (UB1.len > UB2.len) {
      return 1;
   }

   return memcmp(UB1.ptr, UB2.ptr, UB1.len);
}



void UsefulBuf_Set(UsefulBuf *pDest, uint8_t value)
{
   memset(pDest->ptr, value, pDest->len);
}



size_t UsefulBuf_FindBytes(UsefulBufC BytesToSearch, UsefulBufC BytesToFind)
{
   if(BytesToSearch.len < BytesToFind.len) {
      return SIZE_MAX;
   }

   for(size_t uPos = 0; uPos <= BytesToSearch.len - BytesToFind.len; uPos++) {
      if(!UsefulBuf_Compare((UsefulBufC){((uint8_t *)BytesToSearch.ptr) + uPos, BytesToFind.len}, BytesToFind)) {
         return uPos;
      }
   }

   return SIZE_MAX;
}



void UsefulOutBuf_Init(UsefulOutBuf *me, void *pStorage, size_t uStorageSize)
{
   me->magic  = USEFUL_OUT_BUF_MAGIC;
   UsefulOutBuf_Reset(me);

   me->UB.ptr = pStorage;
   me->size   = uStorageSize;


   const uintptr_t uPtrM = UINTPTR_MAX - uStorageSize;
   if(pStorage && (uintptr_t)pStorage > uPtrM) {
      me->err = 1;
   }
}



void UsefulOutBuf_InsertUsefulBuf(UsefulOutBuf *me, UsefulBufC NewData, size_t uInsertionPos)
{
   if(me->err) {

      return;
   }



   if(me->magic != USEFUL_OUT_BUF_MAGIC) {
      me->err = 1;
      return;
   }


   if(me->UB.len > me->size) {
      me->err = 1;
      return;
   }



   if(!UsefulOutBuf_WillItFit(me, NewData.len)) {

      me->err = 1;
      return;
   }



   if(uInsertionPos > me->UB.len) {

      me->err = 1;
      return;
   }


   uint8_t *pSourceOfMove       = ((uint8_t *)me->UB.ptr) + uInsertionPos;
   size_t   uNumBytesToMove     = me->UB.len - uInsertionPos;
   uint8_t *pDestinationOfMove  = pSourceOfMove + NewData.len;
   size_t   uRoomInDestination  = me->size - (uInsertionPos + NewData.len);


   if(uNumBytesToMove <= uRoomInDestination && me->UB.ptr) {
      memmove(pDestinationOfMove, pSourceOfMove, uNumBytesToMove);
   }


   uint8_t *pInsertionPoint = ((uint8_t *)me->UB.ptr) + uInsertionPos;
   uRoomInDestination       = me->size - uInsertionPos;
   if(me->UB.ptr && NewData.len <= uRoomInDestination) {
      memmove(pInsertionPoint, NewData.ptr, NewData.len);
   }

   me->UB.len += NewData.len;
}






int UsefulOutBuf_OutUBuf(UsefulOutBuf *me, UsefulBuf *O)
{
   if(me->err) {
      return me->err;
   }

   if(me->magic != USEFUL_OUT_BUF_MAGIC) {
      me->err = 1;
      return 1;
   }

   *O = me->UB;
   return 0;
}



int UsefulOutBuf_CopyOut(UsefulOutBuf *me, void *pBuf, size_t uBufSize, size_t *puCopied)
{
   UsefulBuf B;
   if(UsefulOutBuf_OutUBuf(me, &B)) {
      return 1;
   }

   if(B.len > uBufSize) {
      return 1;
   }

   memmove(pBuf, B.ptr, B.len);

   *puCopied = me->UB.len;

   return 0;
}



const void *UsefulInputBuf_GetBytes(UsefulInputBuf *me, size_t uAmount)
{

   if(me->err) {
      return NULL;
   }

   if(!UsefulInputBuf_BytesAvailable(me, uAmount)) {

      me->err = 1;
      return NULL;
   }


   const void * const result = ((uint8_t *)me->UB.ptr) + me->cursor;
   me->cursor += uAmount;
   return result;
}


