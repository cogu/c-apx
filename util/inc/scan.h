#ifndef SCAN_H
#define SCAN_H
#include <stdint.h>

/***************** Public Function Declarations *******************/
const uint8_t *scan_searchVal(const uint8_t *pBegin, const uint8_t *pEnd, uint8_t val);
const uint8_t *scan_matchPair(const uint8_t *pBegin, const uint8_t *pEnd, uint8_t left, uint8_t right, uint8_t escapeChar);
const uint8_t *scan_matchStr(const uint8_t *pBegin, const uint8_t *pEnd,const uint8_t *pStrBegin, const uint8_t *pStrEnd);
const uint8_t *scan_searchUntil(const uint8_t *pBegin, const uint8_t *pEnd, uint8_t val);
const uint8_t *scan_digit(const uint8_t *pBegin, const uint8_t *pEnd);
const uint8_t *scan_toInt(const uint8_t *pBegin, const uint8_t *pEnd,int *data);
const uint8_t *scan_line(const uint8_t *pBegin, const uint8_t *pEnd);

#endif //SCAN_H
