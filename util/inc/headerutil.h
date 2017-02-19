#ifndef HEADER_UTIL_H
#define HEADER_UTIL_H

#include <stdint.h>

#define HEADERUTIL16_MAX_NUM_SHORT ((uint16_t) 127u)
#define HEADERUTIL16_MAX_NUM_LONG ((uint16_t) 32895u) //128+32767

#define HEADERUTIL32_MAX_NUM_SHORT ((uint32_t) 127u)
#define HEADERUTIL32_MAX_NUM_LONG ((uint32_t) 2147483647ul) //should be the same as INT_MAX

//backward compatibility macros
#define headerutil_numEncode headerutil_numEncode16
#define headerutil_numDecode headerutil_numDecode16

uint8_t *headerutil_numEncode16(uint8_t *buf, uint32_t maxBufLen, uint16_t value);
const uint8_t *headerutil_numDecode16(const uint8_t *pBegin, const uint8_t *pEnd, uint16_t *value);

uint8_t *headerutil_numEncode32(uint8_t *buf, uint32_t maxBufLen, uint32_t value);
const uint8_t *headerutil_numDecode32(const uint8_t *pBegin, const uint8_t *pEnd, uint32_t *value);

#endif //HEADER_UTIL_H
