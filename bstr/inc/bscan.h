/*****************************************************************************
* \file:    bscan.h
* \author:  Conny Gustafsson
* \date:    2017-08-04
* \brief:   Bounded strings text scan library. Used create simple text parsers
*
* Copyright (c) 2017 Conny Gustafsson
*
******************************************************************************/
#ifndef BSCAN_H
#define BSCAN_H
#include <stdint.h>

/***************** Public Function Declarations *******************/
const uint8_t *bscan_searchVal(const uint8_t *pBegin, const uint8_t *pEnd, uint8_t val);
const uint8_t *bscan_matchPair(const uint8_t *pBegin, const uint8_t *pEnd, uint8_t left, uint8_t right, uint8_t escapeChar);
const uint8_t *bscan_matchStr(const uint8_t *pBegin, const uint8_t *pEnd,const uint8_t *pStrBegin, const uint8_t *pStrEnd);
const uint8_t *bscan_searchUntil(const uint8_t *pBegin, const uint8_t *pEnd, uint8_t val);
const uint8_t *bscan_digit(const uint8_t *pBegin, const uint8_t *pEnd);
const uint8_t *bscan_toLong(const uint8_t *pBegin, const uint8_t *pEnd,long *data);
const uint8_t *bscan_toUnsignedLong(const uint8_t *pBegin, const uint8_t *pEnd,unsigned long *data);
const uint8_t *bscan_line(const uint8_t *pBegin, const uint8_t *pEnd);
const uint8_t *bscan_whilePredicate(const uint8_t *pBegin, const uint8_t *pEnd, int (*pred)(int c));
int bscan_pred_isHorizontalSpace(int c);

#endif //BSCAN_H
