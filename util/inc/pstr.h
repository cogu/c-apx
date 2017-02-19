#ifndef PSTR_H
#define PSTR_H

#include <stdint.h>


/***************** Public Function Declarations *******************/
uint8_t *pstr_make(const uint8_t *pBegin, const uint8_t *pEnd);
uint8_t *pstr_make_x(const uint8_t *pBegin, const uint8_t *pEnd, uint16_t startOffset, uint16_t endOffset);

#endif //PSTR_H
