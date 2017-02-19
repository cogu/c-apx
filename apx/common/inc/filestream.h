#ifndef FILESTREAM_H
#define FILESTREAM_H
#include <stdint.h>

typedef struct ifstream_handler_t{
   //user-defined argument
   void *arg;
   //events
   void (*open)(void *arg);
   void (*close)(void *arg);
   void (*write)(void *arg,const uint8_t *pChunk, uint32_t chunkLen);
}ifstream_handler_t;

typedef struct ifstream_t{
   ifstream_handler_t handler;
}ifstream_t;


/***************** Public Function Declarations *******************/
void ifstream_create(ifstream_t *self, ifstream_handler_t *handler);
void ifstream_destroy(ifstream_t *self);
ifstream_t *ifstream_new(ifstream_handler_t *handler);
void ifstream_delete(ifstream_t *self);

void ifstream_open(ifstream_t *self);
void ifstream_write(ifstream_t *self, const uint8_t *pChunk, uint32_t chunkLen);
void ifstream_close(ifstream_t *self);
int ifstream_readTextFile(ifstream_t *self,const char *filename);


#endif //FILESTREAM_H
