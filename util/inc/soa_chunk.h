/*****************************************************************************
* @file:   soa_chunk.h
* @author: Conny Gustafsson
* @date:   2011-08-20
* @brief:  Chunk Allocator ( An adaptation from "Modern C++ Design", chapter 4 )
*
* Copyright 2011 Conny Gustafsson
*
******************************************************************************/
#ifndef SOA_CHUNK_H__
#define SOA_CHUNK_H__

#include <stdlib.h>

typedef struct soa_chunk_tag
{
  unsigned char *blockData;
  unsigned char firstBlock;
  unsigned char freeBlocks;
} soa_chunk_t;

void soa_chunk_init(soa_chunk_t *chunk, size_t blockSize, unsigned char numBlocks);
void soa_chunk_destroy(soa_chunk_t *chunk);
void *soa_chunk_alloc(soa_chunk_t *chunk,size_t blockSize);
void soa_chunk_free(soa_chunk_t *chunk,void *p, size_t blockSize);


#endif // SOA_CHUNK_H__
