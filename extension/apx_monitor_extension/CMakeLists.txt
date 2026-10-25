project(apx_monitor_extension LANGUAGES C)

set (APX_MONITOR_EXTENSION_HEADERS
    include/apx/extension/diag_dispatcher.h
    include/apx/extension/log_streamer.h
    include/apx/extension/monitor_extension.h
    include/apx/extension/monitor_session.h
    include/apx/extension/server_monitor.h
)

set (APX_MONITOR_EXTENSION_SOURCES
    src/diag_dispatcher.c
    src/log_streamer.c
    src/monitor_extension.c
    src/monitor_session.c
    src/server_monitor.c
)

add_library(apx_monitor_extension ${LIBRARY_TYPE} ${APX_MONITOR_EXTENSION_HEADERS} ${APX_MONITOR_EXTENSION_SOURCES})
if (LEAK_CHECK)
    target_compile_definitions(apx_monitor_extension PRIVATE MEM_LEAK_CHECK)
endif()
if (UNIT_TEST)
    target_compile_definitions(apx_monitor_extension PRIVATE UNIT_TEST)
endif()
target_link_libraries(apx_monitor_extension PRIVATE apx_core)
target_include_directories(apx_monitor_extension PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
set_target_properties(apx_monitor_extension PROPERTIES VERSION ${apx_core_VERSION} SOVERSION ${apx_core_VERSION_MAJOR})

if(APX_SERVER_ENABLE_MONITOR_EXTENSION)
    apx_register_server_extension(
        TARGET apx_monitor_extension
        NAME "monitor"
        HEADER "apx/extension/monitor_extension.h"
        REGISTER_FN "apx_monitor_extension_register"
    )
endif()

install(
  TARGETS apx_monitor_extension
  ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
  COMPONENT Server
)
