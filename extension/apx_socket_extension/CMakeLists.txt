project(apx_socket_extension LANGUAGES C)

set (APX_SOCKET_EXTENSION_HEADERS
    include/apx/extension/socket_server_connection.h
    include/apx/extension/socket_server_extension.h
    include/apx/extension/socket_server.h
)

set (APX_SOCKET_EXTENSION_SOURCES
    src/socket_server_connection.c
    src/socket_server_extension.c
    src/socket_server.c
)

add_library(apx_socket_extension ${LIBRARY_TYPE} ${APX_SOCKET_EXTENSION_HEADERS} ${APX_SOCKET_EXTENSION_SOURCES})
if (LEAK_CHECK)
    target_compile_definitions(apx_socket_extension PRIVATE MEM_LEAK_CHECK)
endif()
if (UNIT_TEST)
    target_link_libraries(apx_socket_extension PRIVATE msocket_testsocket)
    target_compile_definitions(apx_socket_extension PRIVATE UNIT_TEST)
endif()
if(APX_DEBUG)
    target_compile_definitions(apx_socket_extension PUBLIC APX_DEBUG_ENABLE=1)
endif()
target_link_libraries(apx_socket_extension PRIVATE apx_core)
target_include_directories(apx_socket_extension PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
set_target_properties(apx_socket_extension PROPERTIES VERSION ${apx_core_VERSION} SOVERSION ${apx_core_VERSION_MAJOR})

if(APX_SERVER_ENABLE_SOCKET_EXTENSION)
    apx_register_server_extension(
        TARGET apx_socket_extension
        NAME "socket-server"
        HEADER "apx/extension/socket_server_extension.h"
        REGISTER_FN "apx_socketServerExtension_register"
    )
endif()

install(
  TARGETS apx_socket_extension
  LIBRARY DESTINATION lib
  COMPONENT Server
)
