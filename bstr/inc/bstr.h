#ifndef BSTR_H
#define BSTR_H

#include <stdint.h>


/***************** Public Function Declarations *******************/
uint8_t *bstr_make(const uint8_t *pBegin, const uint8_t *pEnd);
uint8_t *bstr_make_x(const uint8_t *pBegin, const uint8_t *pEnd, uint16_t startOffset, uint16_t endOffset);

#endif //BSTR_H
