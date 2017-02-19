/*****************************************************************************
* @file:   		soa_chunk.c
* @author:		Conny Gustafsson
* @date:		2011-08-20
* @brief:		Chunk Allocator ( An adaptation from "Modern C++ Design", chapter 4 )
*
* Copyright 2011 Conny Gustafsson
*
******************************************************************************/
#include "soa_chunk.h"
#include <stdlib.h>
#include <assert.h>
#ifdef MEM_LEAK_CHECK
#include "CMemLeak.h"
#endif


void soa_chunk_init( soa_chunk_t *chunk, size_t blockSize, unsigned char numBlocks )
{
  unsigned char i;
  unsigned char *p;
  chunk->blockData = (unsigned char*) malloc(blockSize * numBlocks);
  if(chunk->blockData == 0) return;
  chunk->firstBlock = 0;
  chunk->freeBlocks = numBlocks;
  for(i=0, p=chunk->blockData; i<numBlocks; p+=blockSize)
  {
    *p = ++i;
  }
  assert(p==chunk->blockData+(blockSize * numBlocks));
}

void soa_chunk_destroy( soa_chunk_t *chunk )
{
  free(chunk->blockData);
}

void *soa_chunk_alloc( soa_chunk_t *chunk,size_t blockSize )
{
  unsigned char *p;
  if(chunk->freeBlocks == 0) return (void*) 0;  
  p = chunk->blockData + (chunk->firstBlock * blockSize);
  chunk->firstBlock = *p; //Index of next available block is stored in byte 0 of the free block
  chunk->freeBlocks--;
  return (void*) p;  
}

void soa_chunk_free( soa_chunk_t *chunk,void *p, size_t blockSize )
{
  size_t pOffset;
  size_t newFirstAvailableBlock;
  unsigned char *pChar = ((unsigned char*)p);
  assert( pChar >= chunk->blockData); //assert that p belongs to this chunk
  pOffset = pChar - chunk->blockData;
  assert(pOffset % blockSize == 0); //assert that p is aligned to the first byte of a block
  *pChar = chunk->firstBlock;       //store index of first available block in byte 0 of the freed block
  newFirstAvailableBlock = pOffset / blockSize;
  assert(newFirstAvailableBlock*blockSize == pOffset); //check for truncation error
  assert(newFirstAvailableBlock < 256); //check for index out of bounds error  
  chunk->firstBlock = (unsigned char) newFirstAvailableBlock;
  chunk->freeBlocks++;
}
