project(apx_text_log_extension LANGUAGES C)

set (APX_TEXT_LOG_EXTENSION_HEADERS
    include/apx/extension/server_text_log_extension.h
    include/apx/extension/server_text_log.h
    include/apx/extension/text_log_base.h
)

set (APX_TEXT_LOG_EXTENSION_SOURCES
    src/server_text_log_extension.c
    src/server_text_log.c
    src/text_log_base.c
)

add_library(apx_text_log_extension ${LIBRARY_TYPE} ${APX_TEXT_LOG_EXTENSION_HEADERS} ${APX_TEXT_LOG_EXTENSION_SOURCES})
if (LEAK_CHECK)
    target_compile_definitions(apx_text_log_extension PRIVATE MEM_LEAK_CHECK)
endif()
if (UNIT_TEST)
    target_compile_definitions(apx_text_log_extension PRIVATE UNIT_TEST)
endif()
if(APX_DEBUG)
    target_compile_definitions(apx_text_log_extension PUBLIC APX_DEBUG_ENABLE=1)
endif()
target_link_libraries(apx_text_log_extension PRIVATE apx_core)
target_include_directories(apx_text_log_extension PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
set_target_properties(apx_text_log_extension PROPERTIES VERSION ${apx_core_VERSION} SOVERSION ${apx_core_VERSION_MAJOR})

install(
  TARGETS apx_text_log_extension
  LIBRARY DESTINATION lib
  COMPONENT Server
)
