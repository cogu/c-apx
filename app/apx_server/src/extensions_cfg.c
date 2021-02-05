/*****************************************************************************
* \file      file.c
* \author    Conny Gustafsson
* \date      2021-01-01
* \brief     Description
*
* Copyright (c) 2021 Conny Gustafsson
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
//////////////////////////////////////////////////////////////////////////////
// INCLUDES
//////////////////////////////////////////////////////////////////////////////
#include "extensions_cfg.h"
#include "apx/extension/socket_server_extension.h"
//#include "apx_serverTextLogExtension.h"

//////////////////////////////////////////////////////////////////////////////
// PRIVATE CONSTANTS AND DATA TYPES
//////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
// PRIVATE FUNCTION PROTOTYPES
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// PUBLIC FUNCTIONS
//////////////////////////////////////////////////////////////////////////////
apx_error_t register_apx_server_extensions(apx_server_t* server, dtl_hv_t* config)
{
   apx_error_t result;
   /*  result = apx_serverTextLogExtension_register(server, dtl_hv_get_cstr(config, APX_SERVER_TEXTLOG_CFG_KEY));
     if (result != APX_NO_ERROR)
     {
        return result;
     }
  */
   result = apx_socketServerExtension_register(server, dtl_hv_get_cstr(config, APX_SOCKET_SERVER_EXT_CFG_KEY));
   if (result != APX_NO_ERROR)
   {
      return result;
   }
   return APX_NO_ERROR;
}


//////////////////////////////////////////////////////////////////////////////
// PRIVATE FUNCTIONS
//////////////////////////////////////////////////////////////////////////////
