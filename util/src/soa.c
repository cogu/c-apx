/*****************************************************************************
* @file:   soa.c
* @author: Conny Gustafsson
* @date:   2011-08-20
* @brief:  Small Object Allocator ( An adaptation from "Modern C++ Design", chapter 4 )
*
* Copyright 2011 Conny Gustafsson
*
******************************************************************************/

#include "soa.h"
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#ifdef MEM_LEAK_CHECK
#include "CMemLeak.h"
#endif


#define DEFAULT_NUM_BLOCKS 255
#define AUTO_INITIALIZE_FSA 1

/**
* Initializes the small object allocator
*/
void soa_init( soa_t *allocator )
{
  memset(allocator->fsa,0,sizeof(soa_fsa_t*)*SOA_SMALL_OBJECT_MAX_SIZE);
}

/**
* Destroys the small object allocator
*/
void soa_destroy( soa_t *allocator )
{
  size_t i;
  for(i=0;i<SOA_SMALL_OBJECT_MAX_SIZE;i++)
  {
    if(allocator->fsa[i]!=0)
    {
      soa_fsa_destroy(allocator->fsa[i]);
      free(allocator->fsa[i]);
    }
  }
}

/**
* Initializes a fixed size allocator (a substructure to SmallObjAllocator) that will handle
* Alloc/Free of memory blocks of blockSize bytes
* 
*/
void soa_initFSA( soa_t *allocator, size_t blockSize, unsigned char numBlocks )
{
  assert((blockSize<=SOA_SMALL_OBJECT_MAX_SIZE) && (blockSize>0)) ;
  if(allocator->fsa[blockSize-1] == 0)
  {
    soa_fsa_t *ptr = (soa_fsa_t*) malloc(sizeof(soa_fsa_t));
    if(ptr!=0)
    {
      soa_fsa_init(ptr,blockSize,numBlocks);
      allocator->fsa[blockSize-1] = ptr;
    }    
  }
}
/**
* Allocates a block of memory of size bytes from the small object allocator
*/
void * soa_alloc( soa_t *allocator, size_t size )
{
  assert((size<=SOA_SMALL_OBJECT_MAX_SIZE) && (size>0)) ;
#if(AUTO_INITIALIZE_FSA)
  if(allocator->fsa[size-1] == 0)
  {
    soa_initFSA(allocator,size,DEFAULT_NUM_BLOCKS);
  }
#endif
  
  assert(allocator->fsa[size-1]);
  return soa_fsa_alloc(allocator->fsa[size-1]);
}

/**
* Returns a previously allocated block of memory of size bytes from the small object allocator
*/
void soa_free( soa_t *allocator, void* ptr, size_t size )
{
  assert((size<=SOA_SMALL_OBJECT_MAX_SIZE) && (size>0)) ;
  assert(allocator->fsa[size-1]);
  soa_fsa_free(allocator->fsa[size-1],ptr);
}
