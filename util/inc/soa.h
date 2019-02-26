/*****************************************************************************
* @file:   soa.h
* @author: Conny Gustafsson
* @date:   2011-08-20
* @brief:  Small Object Allocator ( An adaptation from "Modern C++ Design", chapter 4 )
*
* Copyright 2011 Conny Gustafsson
*
******************************************************************************/
#ifndef SOA_H__
#define SOA_H__
#include "soa_fsa.h"

#define SOA_SMALL_OBJECT_MAX_SIZE 32 //maximum size (in bytes) of object to be considered "small"
#define SOA_DEFAULT_NUM_BLOCKS 255u

typedef struct soa_tag
{
  soa_fsa_t* fsa[SOA_SMALL_OBJECT_MAX_SIZE];
} soa_t;

/***************** Public Function Declarations *******************/
void soa_init(soa_t *allocator);
void soa_destroy(soa_t *allocator);
void soa_initFSA(soa_t *allocator, size_t blockSize, unsigned char numBlocks);
void *soa_alloc(soa_t *allocator, size_t size);
void soa_free(soa_t *allocator, void* ptr, size_t size);


#endif //SOA_H__





