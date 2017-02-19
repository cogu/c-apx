#include <malloc.h>
#include <stdio.h>
#include <string.h>
#include "filestream.h"
#include "apx_parser.h"
#include "apx_stream.h"
#include "apx_node.h"
#ifdef MEM_LEAK_CHECK
#include "CMemLeak.h"
#endif


void apx_parser_create(apx_parser_t *self)
{
   if (self !=0)
   {
      adt_ary_create(&self->nodeList,apx_node_vdelete);
      self->currentNode=0;
   }
}

void apx_parser_destroy(apx_parser_t *self)
{
   if (self !=0)
   {
      if (self->currentNode != 0)
      {
         apx_node_delete(self->currentNode);
      }
      adt_ary_destroy(&self->nodeList);
   }
}

int32_t apx_parser_getNumNodes(apx_parser_t *self)
{
   if (self != 0)
   {
      return adt_ary_length(&self->nodeList);
   }
   return 0;
}

apx_node_t *apx_parser_getNode(apx_parser_t *self, int32_t index)
{
   if (self != 0)
   {
      void **ptr = adt_ary_get(&self->nodeList,index);
      if (ptr != 0)
      {
         return (apx_node_t*) *ptr;
      }
   }
   return 0;
}

/**
 * clear all pointers in &self->nodeList. Note that this will cause memory leak in case the user doesn't manually delete the nodes later.
 */
void apx_parser_clearNodes(apx_parser_t *self)
{
   if (self != 0)
   {
      //clears the list without trigger any calls to destructor
      adt_ary_destructorEnable(&self->nodeList, 0);
      adt_ary_clear(&self->nodeList);
      adt_ary_destructorEnable(&self->nodeList, 1);
   }
}

#if defined(_WIN32) || defined(__GNUC__)
/**
 * convenience function for automatically parsing an apx file from the file system
 */
apx_node_t *apx_parser_parseFile(apx_parser_t *self, const char *filename)
{
   ifstream_handler_t ifstream_handler;
   ifstream_t ifstream;
   apx_istream_t apx_istream;
   apx_istream_handler_t apx_istream_handler;

   memset(&ifstream_handler,0,sizeof(ifstream_handler));
   memset(&apx_istream_handler,0,sizeof(apx_istream_handler));
   ifstream_handler.open = apx_istream_vopen;
   ifstream_handler.close = apx_istream_vclose;
   ifstream_handler.write = apx_istream_vwrite;
   ifstream_handler.arg = (void *) &apx_istream;

   apx_istream_handler.arg = self;
   apx_istream_handler.open = apx_parser_vopen;
   apx_istream_handler.close = apx_parser_vclose;
   apx_istream_handler.node = apx_parser_vnode;
   apx_istream_handler.datatype = apx_parser_vdatatype;
   apx_istream_handler.provide = apx_parser_vprovide;
   apx_istream_handler.require = apx_parser_vrequire;
   apx_istream_handler.node_end = apx_parser_vnode_end;


   apx_istream_create(&apx_istream,&apx_istream_handler);
   ifstream_create(&ifstream,&ifstream_handler);

   ifstream_readTextFile(&ifstream,filename);

   ifstream_destroy(&ifstream);
   apx_istream_destroy(&apx_istream);
   return apx_parser_getNode(self,-1);
}
#endif

//event handlers
void apx_parser_open(apx_parser_t *self)
{
   (void) self;
}

void apx_parser_close(apx_parser_t *self)
{
   if ( (self!=0) && (self->currentNode!=0) )
   {
      apx_node_resolvePortSignatures(self->currentNode);
      adt_ary_push(&self->nodeList,self->currentNode);
      self->currentNode=0;
   }
}

void apx_parser_node(apx_parser_t *self, const char *name) //N"<name>"
{
   if (self != 0)
   {
      if (self->currentNode!=0)
      {
         apx_node_resolvePortSignatures(self->currentNode);
         adt_ary_push(&self->nodeList,self->currentNode);
         self->currentNode=0;
      }
      self->currentNode=apx_node_new(name);
   }
}

void apx_parser_datatype(apx_parser_t *self, const char *name, const char *dsg, const char *attr)
{
   if ( (self != 0) && (self->currentNode != 0) )
   {
      apx_node_createDataType(self->currentNode,name,dsg,attr);
   }
}

void apx_parser_require(apx_parser_t *self, const char *name, const char *dsg, const char *attr)
{
   if ( (self != 0) && (self->currentNode != 0) )
   {
      apx_node_createRequirePort(self->currentNode,name,dsg,attr);
   }
}

void apx_parser_provide(apx_parser_t *self, const char *name, const char *dsg, const char *attr)
{
   apx_node_createProvidePort(self->currentNode,name,dsg,attr);
}

void apx_parser_node_end(apx_parser_t *self)
{
   if ( (self != 0) && (self->currentNode!=0) )
   {
      apx_node_resolvePortSignatures(self->currentNode);
      adt_ary_push(&self->nodeList,self->currentNode);
      self->currentNode=0;
   }
}

//void event handlers
void apx_parser_vopen(void *arg)
{
   apx_parser_open((apx_parser_t*) arg);
}

void apx_parser_vclose(void *arg)
{
   apx_parser_close((apx_parser_t*) arg);
}

void apx_parser_vnode(void *arg, const char *name)
{
   apx_parser_node((apx_parser_t*) arg,name);
}

void apx_parser_vdatatype(void *arg, const char *name, const char *dsg, const char *attr)
{
   apx_parser_datatype((apx_parser_t*) arg,name,dsg,attr);
}

void apx_parser_vrequire(void *arg, const char *name, const char *dsg, const char *attr)
{
   apx_parser_require((apx_parser_t*) arg,name,dsg,attr);
}

void apx_parser_vprovide(void *arg, const char *name, const char *dsg, const char *attr)
{
   apx_parser_provide((apx_parser_t*) arg,name,dsg,attr);
}

void apx_parser_vnode_end(void *arg)
{
   apx_parser_node_end((apx_parser_t*) arg);
}

