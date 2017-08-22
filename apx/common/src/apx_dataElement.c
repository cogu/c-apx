#include <errno.h>
#include <malloc.h>
#include <assert.h>
#include <string.h>
#include "apx_dataElement.h"
#include "apx_error.h"
#include "pack.h"
#ifdef MEM_LEAK_CHECK
#include "CMemLeak.h"
#endif

#ifdef _MSC_VER
#define STRDUP _strdup
#else
#define STRDUP strdup
#endif


/**************** Private Function Declarations *******************/
static uint8_t *apx_dataElement_pack_sv(apx_dataElement_t *self, uint8_t *pBegin, uint8_t *pEnd, dtl_sv_t *sv);
static uint8_t *apx_dataElement_pack_record(apx_dataElement_t *self, uint8_t *pBegin, uint8_t *pEnd, dtl_av_t *av);

/**************** Private Variable Declarations *******************/


/****************** Public Function Definitions *******************/
apx_dataElement_t *apx_dataElement_new(int8_t baseType, const char *name)
{
   apx_dataElement_t *self = (apx_dataElement_t*) malloc(sizeof(apx_dataElement_t));
   if(self != 0)
   {
      int8_t result = apx_dataElement_create(self,baseType,name);
      if (result<0)
      {
         free(self);
         self=0;
      }
   }
   else
   {
      errno = ENOMEM;
   }
   return self;
}

void apx_dataElement_delete(apx_dataElement_t *self)
{
   if(self != 0)
   {
      apx_dataElement_destroy(self);
      free(self);
   }
}

void apx_dataElement_vdelete(void *arg)
{
   apx_dataElement_delete((apx_dataElement_t*) arg);
}

int8_t apx_dataElement_create(apx_dataElement_t *self, int8_t baseType, const char *name)
{
   if (self != 0)
   {
      if (name != 0)
      {
         self->name=STRDUP(name);
         if (self->name == 0)
         {
            errno = ENOMEM;
            return -1;
         }
      }
      else
      {
         self->name = 0;
      }
      self->baseType=baseType;
      if (baseType == APX_BASE_TYPE_RECORD)
      {
         self->childElements = adt_ary_new(apx_dataElement_vdelete);
      }
      else
      {
         self->childElements = (adt_ary_t*) 0;
      }
      self->arrayLen=0;
      self->min.s32=0;
      self->max.s32=0;
      self->packLen=0;
   }
   return 0;
}

void apx_dataElement_destroy(apx_dataElement_t *self)
{
   if (self != 0)
   {
      if (self->name != 0)
      {
         free(self->name);
      }
      if (self->childElements != 0)
      {
         adt_ary_delete(self->childElements);
      }
   }
}

void apx_dataElement_initRecordType(apx_dataElement_t *self)
{
   self->baseType=APX_BASE_TYPE_RECORD;
   if (self->childElements != 0)
   {
      adt_ary_delete(self->childElements);
   }
   self->childElements = adt_ary_new(apx_dataElement_vdelete);
}

uint8_t *apx_dataElement_pack_dv(apx_dataElement_t *self, uint8_t *pBegin, uint8_t *pEnd, dtl_dv_t *dv)
{
   if ( (self != 0) && (pBegin != 0) && (pEnd != 0) && (pBegin <= pEnd) && (dv != 0) )
   {
      uint8_t *pNext = pBegin;
      uint8_t *pResult;
      dtl_dv_type_id dv_type;
      int8_t element_type;
      dv_type = dtl_dv_type(dv);
      element_type = self->baseType;

      if (element_type == APX_BASE_TYPE_NONE)
      {
         apx_setError(APX_ELEMENT_TYPE_ERROR);
         return 0;
      }
      else if (element_type == APX_BASE_TYPE_RECORD)
      {
         if (dv_type == DTL_DV_ARRAY)
         {
            if (self->arrayLen > 0)
            {
               uint32_t i;
               uint32_t num_dv_elem;
               dtl_av_t *av = (dtl_av_t*) dv;
               num_dv_elem = (uint32_t) dtl_av_length(av);
               if (num_dv_elem != self->arrayLen)
               {
                  apx_setError(APX_LENGTH_ERROR);
                  return 0;
               }

               for (i=0; i<self->arrayLen; i++)
               {
                  dtl_dv_t *child_dv = *dtl_av_get(av, (int32_t) i);
                  dv_type = dtl_dv_type(dv);
                  if (dv_type == DTL_DV_ARRAY)
                  {
                     pResult = apx_dataElement_pack_record(self, pNext, pEnd, (dtl_av_t*) child_dv);
                     if (pResult == 0 || (pResult == pNext) )
                     {
                        return 0;
                     }
                     pNext = pResult;
                  }
                  else
                  {
                     apx_setError(APX_DV_TYPE_ERROR); //expected array type from dv variable
                     return 0;
                  }
               }
            }
            else
            {
               pResult = apx_dataElement_pack_record(self, pNext, pEnd, (dtl_av_t*) dv);
               if (pResult == 0 || (pResult == pNext) )
               {
                  return 0;
               }
               pNext = pResult;
            }
         }
         else
         {
            apx_setError(APX_DV_TYPE_ERROR); //expected array type from dv variable
            return 0;
         }
      }
      else
      {
         //scalar base type
         if ( (self->arrayLen > 0) && (element_type != APX_BASE_TYPE_STRING))
         {
            if (dv_type == DTL_DV_ARRAY)
            {
               int32_t i;
               int32_t i32Length;
               dtl_av_t *av = (dtl_av_t*) dv;

               i32Length = dtl_av_length(av);
               if (i32Length != self->arrayLen)
               {
                  apx_setError(APX_LENGTH_ERROR);
                  return 0;
               }
               for (i = 0; i < i32Length; i++)
               {
                  dtl_dv_t *child_dv = *dtl_av_get(av, i);
                  if (dtl_dv_type(child_dv) == DTL_DV_SCALAR)
                  {
                     pResult = apx_dataElement_pack_sv(self, pNext, pEnd, (dtl_sv_t*) child_dv);
                     if (pResult == 0 || (pResult == pNext) )
                     {
                        return 0;
                     }
                     pNext = pResult;
                  }
                  else
                  {
                     apx_setError(APX_DV_TYPE_ERROR); //expected array type from dv variable
                     return 0;
                  }
               }
            }
            else
            {
               apx_setError(APX_DV_TYPE_ERROR); //expected array type from dv variable
               return 0;
            }
         }
         else
         {
            //simple base type, not array (assume scalar)
            if (dv_type == DTL_DV_SCALAR)
            {
               pResult = apx_dataElement_pack_sv(self, pNext, pEnd, (dtl_sv_t*) dv);
               if (pResult == 0 || (pResult == pNext) )
               {
                  return 0;
               }
               pNext = pResult;
            }
            else
            {
               apx_setError(APX_DV_TYPE_ERROR); //expected scalar type from dv variable
               return 0;
            }
         }
      }
      return pNext;
   }
   errno = EINVAL;
   return 0;
}

void apx_dataElement_setArrayLen(apx_dataElement_t *self, uint32_t arrayLen)
{
   if (self != 0)
   {
      self->arrayLen = arrayLen;
   }
}

uint32_t apx_dataElement_getArrayLen(apx_dataElement_t *self)
{
   if (self != 0)
   {
      return self->arrayLen;
   }
   return 0;
}


/***************** Private Function Definitions *******************/

static uint8_t *apx_dataElement_pack_sv(apx_dataElement_t *self, uint8_t *pBegin, uint8_t *pEnd, dtl_sv_t *sv)
{
   if ( (self != 0) && (pBegin != 0) && (pEnd != 0) && (pBegin <= pEnd) && (sv != 0) )
   {
      uint8_t *pNext = pBegin;
      const char *cstr;
      size_t len;
      switch(self->baseType)
      {
      case APX_BASE_TYPE_NONE:
         break;
      case APX_BASE_TYPE_UINT8:
         if (pNext + sizeof(uint8_t) <= pEnd)
         {
            packU8(pNext, (uint8_t) dtl_sv_get_u32(sv));
         }
         else
         {
            apx_setError(APX_LENGTH_ERROR);
            return 0;
         }
         break;
      case APX_BASE_TYPE_UINT16:
         if (pNext  + sizeof(uint16_t) <= pEnd)
         {
            packU16LE(pNext, (uint16_t) dtl_sv_get_u32(sv));
         }
         else
         {
            apx_setError(APX_LENGTH_ERROR);
            return 0;
         }
         break;
      case APX_BASE_TYPE_UINT32:
         if (pNext  + sizeof(uint32_t) <= pEnd)
         {
            packU32LE(pNext, dtl_sv_get_u32(sv));
         }
         else
         {
            apx_setError(APX_LENGTH_ERROR);
            return 0;
         }
         break;
      case APX_BASE_TYPE_UINT64:
#if  defined(__GNUC__) && defined(__LP64__)
         if (pNext  + sizeof(uint64_t) <= pEnd)
         {
            packU64LE(pNext, dtl_sv_get_u64(sv));
         }
         else
         {
            apx_setError(APX_LENGTH_ERROR);
            return 0;
         }
#else
         apx_setError(APX_UNSUPPORTED_ERROR);
         return 0;
#endif
         break;
      case APX_BASE_TYPE_SINT8:
         if (pNext  + sizeof(uint8_t) <= pEnd)
         {
            packU8(pNext, (uint8_t) dtl_sv_get_i32(sv));
         }
         else
         {
            apx_setError(APX_LENGTH_ERROR);
            return 0;
         }
         break;
      case APX_BASE_TYPE_SINT16:
         if (pNext  + sizeof(uint16_t) <= pEnd)
         {
            packU16LE(pNext, (uint16_t) dtl_sv_get_i32(sv));
         }
         else
         {
            apx_setError(APX_LENGTH_ERROR);
            return 0;
         }
         break;
      case APX_BASE_TYPE_SINT32:
         if (pNext  + sizeof(uint32_t) <= pEnd)
         {
            packU32LE(pNext, (uint32_t) dtl_sv_get_i32(sv));
         }
         else
         {
            apx_setError(APX_LENGTH_ERROR);
            return 0;
         }
         break;
      case APX_BASE_TYPE_SINT64:
#if  defined(__GNUC__) && defined(__LP64__)
         if (pNext  + sizeof(uint64_t) <= pEnd)
         {
            packU64LE(pNext, (uint64_t) dtl_sv_get_i64(sv));
         }
         else
         {
            apx_setError(APX_LENGTH_ERROR);
            return 0;
         }
#else
         apx_setError(APX_UNSUPPORTED_ERROR);
         return 0;
#endif
         break;
      case APX_BASE_TYPE_STRING:
         cstr = dtl_sv_get_cstr(sv);
         if (cstr == 0)
         {
            apx_setError(APX_DV_TYPE_ERROR);
            return 0;
         }
         len = strlen(cstr);
         if (len > self->arrayLen)
         {
            apx_setError(APX_LENGTH_ERROR);
            return 0;
         }
         memcpy(pNext, cstr, len);
         pNext[len] = 0;
         pNext+=self->arrayLen;
         break;
      case APX_BASE_TYPE_RECORD:
         apx_setError(APX_DV_TYPE_ERROR);
         return 0;
         break;
      }
      return pNext;
   }
   errno = EINVAL;
   return 0;
}

void apx_dataElement_appendChild(apx_dataElement_t *self, apx_dataElement_t *child)
{
   if( (self != 0) && (child != 0) && (self->baseType == APX_BASE_TYPE_RECORD))
   {
      if (self->childElements == 0)
      {
         self->childElements = adt_ary_new(apx_dataElement_vdelete);
      }
      adt_ary_push(self->childElements, child);
   }
}

int32_t apx_dataElement_getNumChild(apx_dataElement_t *self)
{
   if ( (self != 0) && (self->childElements != 0) )
   {
      return adt_ary_length(self->childElements);
   }
   return -1;
}

apx_dataElement_t *apx_dataElement_getChildAt(apx_dataElement_t *self, int32_t index)
{
   if ( (self != 0) && (self->childElements != 0) )
   {
      void **ptr = adt_ary_get(self->childElements, index);
      if (ptr != 0)
      {
         return (apx_dataElement_t*) *ptr;
      }
   }
   return 0;
}

static uint8_t *apx_dataElement_pack_record(apx_dataElement_t *self, uint8_t *pBegin, uint8_t *pEnd, dtl_av_t *av)
{
   if ( (self != 0) && (pBegin != 0) && (pEnd != 0) && (pBegin <= pEnd) && (av != 0))
   {
      int32_t i;
      int32_t num_dv_elem;
      int32_t num_child_elem;
      uint8_t *pNext = pBegin;
      uint8_t *pResult;
      num_dv_elem = dtl_av_length(av);
      num_child_elem = apx_dataElement_getNumChild(self);
      if (num_dv_elem != num_child_elem)
      {
         apx_setError(APX_LENGTH_ERROR);
         return 0;
      }

      for (i = 0; i < num_child_elem; i++)
      {
         apx_dataElement_t *child_element;
         dtl_dv_t *child_dv = *dtl_av_get(av, i);
         child_element = (apx_dataElement_t*) adt_ary_value(self->childElements, i);
         pResult = apx_dataElement_pack_dv(child_element, pNext, pEnd, child_dv);
         if (pResult == 0 || (pResult == pNext) )
         {
            return 0;
         }
         pNext = pResult;
      }
      return pNext;
   }
   errno = EINVAL;
   return 0;
}


