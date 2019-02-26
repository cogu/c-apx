/*****************************************************************************
* @file:   		soa.c
* @author:		Conny Gustafsson
* @date:		2011-08-20
* @brief:		Small Object Allocator ( An adaptation from "Modern C++ Design", chapter 4 )
*
* Copyright 2011 Conny Gustafsson
*
******************************************************************************/
#include "soa_fsa.h"
#include <stdlib.h>
#include <assert.h>
#ifdef MEM_LEAK_CHECK
#include "CMemLeak.h"
#endif


void soa_fsa_init( soa_fsa_t *allocator,size_t blockSize, unsigned char numBlocks )
{  
  allocator->blockSize = blockSize;
  allocator->numBlocks = numBlocks;
  allocator->allocChunk = 0;
  allocator->deallocChunk = 0;
  allocator->chunks_len = 0;
  allocator->chunks = 0;
}

void soa_fsa_destroy( soa_fsa_t *allocator )
{
  size_t i;
  soa_chunk_t *p;
  if(allocator->chunks)
  {
    for(i=0,p=allocator->chunks;i<allocator->chunks_len;i++,p++)
    {
      soa_chunk_destroy(p);
    }
    free(allocator->chunks);
  }
}

void * soa_fsa_alloc( soa_fsa_t *allocator )
{
  if((allocator->allocChunk == 0) || (allocator->allocChunk->freeBlocks == 0 ) ) //No free blocks in this chunk or no chunk available
  {
    //linear search through all chunks to find a free block
    size_t i;
    soa_chunk_t *chunk;    
    allocator->allocChunk = 0; //invalidate allocChunk
    for(i=0,chunk=allocator->chunks;i<allocator->chunks_len;i++,chunk++)
    {
      if(chunk->freeBlocks > 0) //space available?
      {
        allocator->allocChunk = chunk;
        break;
      }
    }
    if(allocator->allocChunk == 0) //We still have not found a chunk with a free block?
    {
      soa_chunk_t *ptr,*chunk;
      //grow chunk array by one
      allocator->chunks_len++;
      if (allocator->chunks == 0)
      {
         ptr = (soa_chunk_t*) malloc(allocator->chunks_len * sizeof(soa_chunk_t));
      }
      else
      {
         ptr = (soa_chunk_t*) realloc(allocator->chunks,allocator->chunks_len * sizeof(soa_chunk_t));
      }

      if(ptr)
      {
        if(ptr != allocator->chunks)
        {
          //The memory has moved, all pointers into allocator->chunks must be invalidated
          allocator->allocChunk = 0;
          allocator->deallocChunk = 0;
        }
        allocator->chunks = ptr;
        chunk = allocator->chunks+allocator->chunks_len-1; //pointer to last chunk
        soa_chunk_init(chunk,allocator->blockSize,allocator->numBlocks); //call constructor on newly created chunk
        allocator->allocChunk = chunk;        
      }
      else
      {
        return (void*) 0;
      }
    }
  }
  assert(allocator->allocChunk);
  assert(allocator->allocChunk->freeBlocks > 0);      
  return soa_chunk_alloc(allocator->allocChunk,allocator->blockSize);
}

void soa_fsa_free( soa_fsa_t *allocator, void* ptr )
{
  size_t chunkSizeBytes = allocator->blockSize*allocator->numBlocks;
  unsigned char *a, *b; //[a..b] is a (memory) range, where a is the first byte, and b is the last byte (in the range)
  unsigned char *p = (unsigned char*) ptr;
  
  if(allocator->deallocChunk) //Is this deallocation in the same chunk as last time?
  {
    a = allocator->deallocChunk->blockData;
    b = allocator->deallocChunk->blockData+chunkSizeBytes;
    if( (a <= p) && (p<=b)) //Does p fall in the range [a..b]?
    {
      //Do nothing (deallocChunk is valid)
    }
    else
    {
      allocator->deallocChunk = 0; //invalidate deallocChunk
    }
  }
  if(allocator->deallocChunk == 0)
  {
    //which chunk does p belong to? (linear search)
    size_t i;
    soa_chunk_t *chunk;
    for(i=0,chunk=allocator->chunks;i<allocator->chunks_len;i++,chunk++)
    {
      a = chunk->blockData;
      b = chunk->blockData+chunkSizeBytes;
      if( (a <= p) && (p<=b)) //Does p fall in the range [a..b]?
      {
        //Found it!
        allocator->deallocChunk = chunk;
        break;
      }
    }
  }
  assert(allocator->deallocChunk); //If this fails it means that ptr did not originate from this allocator
  soa_chunk_free(allocator->deallocChunk,ptr,allocator->blockSize);
}
