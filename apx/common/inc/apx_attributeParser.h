#ifndef APX_ATTRIBUTE_PARSER_H
#define APX_ATTRIBUTE_PARSER_H

//////////////////////////////////////////////////////////////////////////////
// INCLUDES
//////////////////////////////////////////////////////////////////////////////
#include <stdint.h>
#if defined(_MSC_PLATFORM_TOOLSET) && (_MSC_PLATFORM_TOOLSET<=110)
#include "msc_bool.h"
#else
#include <stdbool.h>
#endif
#include "adt_stack.h"
#include "apx_portAttributes.h"
#include "apx_error.h"


//////////////////////////////////////////////////////////////////////////////
// CONSTANTS AND DATA TYPES
//////////////////////////////////////////////////////////////////////////////
typedef struct apx_attributeParser_tag
{
   apx_error_t lastError;
   const uint8_t *pErrorNext;
   int32_t lastErrorPos;
   int16_t majorVersion;
   int16_t minorVersion;
}apx_attributeParser_t;

#ifdef UNIT_TEST
#define DYN_STATIC
#else
#define DYN_STATIC static
#endif

//////////////////////////////////////////////////////////////////////////////
// GLOBAL VARIABLES
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// GLOBAL FUNCTION PROTOTYPES
//////////////////////////////////////////////////////////////////////////////
void apx_attributeParser_create(apx_attributeParser_t *self);
void apx_attributeParser_destroy(apx_attributeParser_t *self);
void apx_attributeParser_setVersion(apx_attributeParser_t *self, int16_t majorVersion, int16_t minorVersion);
apx_error_t apx_attributeParser_parseObject(apx_attributeParser_t *self, apx_portAttributes_t *attributeObject);
const uint8_t* apx_attributeParser_parse(apx_attributeParser_t *self, const uint8_t *pBegin, const uint8_t *pEnd, apx_portAttributes_t *attr);
apx_error_t apx_attributeParser_getLastError(apx_attributeParser_t *self, const uint8_t **ppNext);

#ifdef UNIT_TEST
DYN_STATIC const uint8_t* apx_attributeParser_parseSingleAttribute(apx_attributeParser_t *self, const uint8_t *pBegin, const uint8_t *pEnd, apx_portAttributes_t *attr);
DYN_STATIC const uint8_t* apx_attributeParser_parseInitValue(apx_attributeParser_t *self, const uint8_t *pBegin, const uint8_t *pEnd, dtl_dv_t **ppInitValue);
DYN_STATIC const uint8_t* apx_attributeParser_parseArrayLength(apx_attributeParser_t *self, const uint8_t *pBegin, const uint8_t *pEnd, int32_t *pValue);
#endif


#endif //APX_ATTRIBUTE_PARSER_H
