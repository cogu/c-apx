project(apx_tls_extension LANGUAGES C)

set(APX_TLS_EXTENSION_HEADERS
    include/apx/extension/tls_server_connection.h
    include/apx/extension/tls_server_extension.h
    include/apx/extension/tls_server.h
)

set(APX_TLS_EXTENSION_SOURCES
    src/tls_server_connection.c
    src/tls_server_extension.c
    src/tls_server.c
)

add_library(apx_tls_extension ${LIBRARY_TYPE} ${APX_TLS_EXTENSION_HEADERS} ${APX_TLS_EXTENSION_SOURCES})
if(LEAK_CHECK)
    target_compile_definitions(apx_tls_extension PRIVATE MEM_LEAK_CHECK)
endif()
if(UNIT_TEST)
    target_compile_definitions(apx_tls_extension PRIVATE UNIT_TEST)
endif()
if(APX_DEBUG)
    target_compile_definitions(apx_tls_extension PUBLIC APX_DEBUG_ENABLE=1)
endif()
target_link_libraries(apx_tls_extension PRIVATE apx_core)
target_include_directories(apx_tls_extension PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
set_target_properties(apx_tls_extension PROPERTIES VERSION ${apx_core_VERSION} SOVERSION ${apx_core_VERSION_MAJOR})

if(APX_SERVER_ENABLE_TLS_EXTENSION)
    apx_register_server_extension(
        TARGET apx_tls_extension
        NAME "tls-server"
        HEADER "apx/extension/tls_server_extension.h"
        REGISTER_FN "apx_tls_server_extension_register"
    )
endif()

install(
  TARGETS apx_tls_extension
  ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
  COMPONENT Server
)
