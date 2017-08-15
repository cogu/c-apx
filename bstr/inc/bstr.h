/*****************************************************************************
* \file:    bstr.h
* \author:  Conny Gustafsson
* \date:    2017-08-04
* \brief:   Bounded strings library
*
* Copyright (c) 2017 Conny Gustafsson
*
******************************************************************************/
#ifndef BSTR_H
#define BSTR_H

#include <stdint.h>

/***************** Public Function Declarations *******************/
uint8_t *bstr_make(const uint8_t *pBegin, const uint8_t *pEnd);
uint8_t *bstr_make_x(const uint8_t *pBegin, const uint8_t *pEnd, uint16_t startOffset, uint16_t endOffset);
const uint8_t *bstr_searchVal(const uint8_t *pBegin, const uint8_t *pEnd, uint8_t val);
const uint8_t *bstr_matchPair(const uint8_t *pBegin, const uint8_t *pEnd, uint8_t left, uint8_t right, uint8_t escapeChar);
const uint8_t *bstr_matchStr(const uint8_t *pBegin, const uint8_t *pEnd,const uint8_t *pStrBegin, const uint8_t *pStrEnd);
const uint8_t *bstr_searchUntil(const uint8_t *pBegin, const uint8_t *pEnd, uint8_t val);
const uint8_t *bstr_digit(const uint8_t *pBegin, const uint8_t *pEnd);
const uint8_t *bstr_toLong(const uint8_t *pBegin, const uint8_t *pEnd,long *data);
const uint8_t *bstr_toUnsignedLong(const uint8_t *pBegin, const uint8_t *pEnd, uint8_t base, unsigned long *data);
const uint8_t *bstr_line(const uint8_t *pBegin, const uint8_t *pEnd);
const uint8_t *bstr_whilePredicate(const uint8_t *pBegin, const uint8_t *pEnd, int (*pred)(int c));
int bstr_pred_isHorizontalSpace(int c);
int bstr_pred_isDigit(int c);
int bstr_pred_isHexDigit(int c);
#endif //BSTR_H
