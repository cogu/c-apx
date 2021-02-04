/*****************************************************************************
* \file      apx_file.h
* \author    Conny Gustafsson
* \date      2018-08-30
* \brief     APX file class
*
* Copyright (c) 2018-2021 Conny Gustafsson
* Permission is hereby granted, free of charge, to any person obtaining a copy of
* this software and associated documentation files (the "Software"), to deal in
* the Software without restriction, including without limitation the rights to
* use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
* the Software, and to permit persons to whom the Software is furnished to do so,
* subject to the following conditions:

* The above copyright notice and this permission notice shall be included in all
* copies or substantial portions of the Software.

* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
* IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
* FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
* COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
* IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
* CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*
******************************************************************************/
#ifndef APX_FILE_H
#define APX_FILE_H

//////////////////////////////////////////////////////////////////////////////
// INCLUDES
//////////////////////////////////////////////////////////////////////////////
#include "apx/types.h"
#include "apx/error.h"
#include "apx/file_info.h"
#include "apx/event_listener.h"
#include "adt_list.h"

#ifndef APX_EMBEDDED
# ifdef _WIN32
#  ifndef WIN32_LEAN_AND_MEAN
#   define WIN32_LEAN_AND_MEAN
#  endif
#  include <Windows.h>
# else
#  include <pthread.h>
# endif
#include "osmacro.h"
#endif


//////////////////////////////////////////////////////////////////////////////
// CONSTANTS AND DATA TYPES
//////////////////////////////////////////////////////////////////////////////
//forward declarations
struct apx_file_tag;
struct apx_fileManager_tag;

typedef apx_error_t (apx_file_open_close_notify_func)(void *arg, struct apx_file_tag *file);
typedef apx_error_t (apx_file_write_notify_func)(void *arg, struct apx_file_tag *file, uint32_t offset, const uint8_t *src, uint32_t len);
typedef apx_error_t (apx_file_read_const_data_func)(void *arg, struct apx_file_tag *file, uint32_t offset, uint8_t *dest, uint32_t len);

typedef struct apx_fileNotificationHandler_tag
{
   void *arg;
   apx_file_open_close_notify_func *open_notify; //Notifies file owner that the file was openened on remote end (used for local files)
   apx_file_open_close_notify_func* close_notify; //Notifies file owner that the file was closed on remote end (used for local files)
   apx_file_write_notify_func *write_notify; //Notifies file owner that the file has just been written to (used for remote files)
} apx_fileNotificationHandler_t;

typedef struct apx_file_tag
{
   bool is_file_open;
   bool has_first_write;

   apx_fileType_t apx_file_type;
   rmf_fileInfo_t file_info;
   apx_fileNotificationHandler_t notification_handler;
   struct apx_fileManager_tag *file_manager;
   adt_list_t event_listeners; //strong references to apx_fileEventListener2_t
   MUTEX_T lock;
} apx_file_t;

//////////////////////////////////////////////////////////////////////////////
// GLOBAL FUNCTION PROTOTYPES
//////////////////////////////////////////////////////////////////////////////
apx_error_t apx_file_create(apx_file_t *self, const rmf_fileInfo_t *file_info);
void apx_file_destroy(apx_file_t *self);
apx_file_t *apx_file_new(const rmf_fileInfo_t * file_info);
//apx_file_t* apx_file_clone(const apx_file_t* file);
void apx_file_delete(apx_file_t *self);
void apx_file_vdelete(void *arg);
void apx_file_open(apx_file_t *self);
void apx_file_close(apx_file_t *self);
void apx_file_set_notification_handler(apx_file_t *self, const apx_fileNotificationHandler_t *handler);
bool apx_file_has_first_write(apx_file_t *self);
void apx_file_set_first_write(apx_file_t *self);
bool apx_file_is_open(apx_file_t *self);
bool apx_file_is_local(apx_file_t *self);
bool apx_file_is_remote(apx_file_t *self);
bool apx_file_has_valid_address(apx_file_t* self);
struct apx_fileManager_tag* apx_file_get_file_manager(apx_file_t *self);
void apx_file_set_file_manager(apx_file_t *self, struct apx_fileManager_tag *file_manager);
void* apx_file_register_event_listener(apx_file_t* self, apx_fileEventListener2_t* handler_table);
void apx_file_unregister_event_listener(apx_file_t* self, void* handle);
apx_fileType_t apx_file_get_apx_file_type(const apx_file_t* self);
uint32_t apx_file_get_address(const apx_file_t *self);
uint32_t apx_file_get_address_without_flags(const apx_file_t* self);
void apx_file_set_address(apx_file_t* self, uint32_t address);
uint32_t apx_file_get_size(const apx_file_t *self);
const char *apx_file_get_name(const apx_file_t *self);
uint32_t apx_file_get_end_address(const apx_file_t* self);
uint32_t apx_file_get_end_address_without_flags(const apx_file_t* self);
bool apx_file_address_in_range(const apx_file_t* self, uint32_t address);
rmf_fileInfo_t const* apx_file_get_file_info(const apx_file_t* self);
rmf_fileInfo_t* apx_file_clone_file_info(const apx_file_t* self);
rmf_digestType_t apx_file_get_digest_type(apx_file_t const* self);
uint8_t const* apx_file_get_digest_data(const apx_file_t* self);
apx_error_t apx_file_open_notify(apx_file_t* self);
apx_error_t apx_file_write_notify(apx_file_t* self, uint32_t offset, const uint8_t* src, uint32_t len);

//global functions
char const* apx_file_type_to_extension(apx_fileType_t file_type);
bool apx_file_less_than(const apx_file_t* a, const apx_file_t* b);


#endif //APX_FILE_H

