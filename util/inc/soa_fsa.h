/*****************************************************************************
* @file:   		soa_fsa.h
* @author:		Conny Gustafsson
* @date:		2011-08-20
* @brief:		Fixed Size Allocator ( An adaptation from "Modern C++ Design", chapter 4 )
*
* Copyright 2011 Conny Gustafsson
*
******************************************************************************/
#ifndef SOA_FSA_H__
#define SOA_FSA_H__
#include "soa_chunk.h"

typedef struct soa_fsa_tag
{
  size_t blockSize;
  unsigned char numBlocks;
  soa_chunk_t *chunks, *allocChunk, *deallocChunk;
  size_t chunks_len;
} soa_fsa_t;

/***************** Public Function Declarations *******************/
void soa_fsa_init(soa_fsa_t *allocator,size_t blockSize, unsigned char numBlocks);
void soa_fsa_destroy(soa_fsa_t *allocator);
void *soa_fsa_alloc(soa_fsa_t *allocator);
void soa_fsa_free(soa_fsa_t *allocator, void* ptr);

#endif //SOA_FSA_H__
