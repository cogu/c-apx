cmake_minimum_required(VERSION 3.20)


project(apx_core LANGUAGES C VERSION 0.3.3)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(apx_core_ALPHA_BETA_VERSION "a3")


set(ADT_RBFH_ENABLE ON CACHE BOOL "This must always be enabled in c-apx" FORCE)

include(adt/cmake/BuildTypes.cmake)
include(adt/cmake/LeakCheck.cmake)
include(adt/cmake/UnitTest.cmake)
include(cmake/SetEnv.cmake)
include(cmake/ApxExtension.cmake)

option(APX_BUILD_SERVER "Build the APX server executable" ON)
option(APX_SERVER_ENABLE_SOCKET_EXTENSION "Enable Socket Server Extension" ON)
option(APX_SERVER_ENABLE_TEXT_LOG_EXTENSION "Enable Server Text Log Extension" ON)
option(APX_SERVER_ENABLE_MONITOR_EXTENSION "Enable Server Monitor Extension" ON)
set(APX_SERVER_EXTENSION_DIRS "" CACHE STRING "Semicolon-separated list of out-of-tree APX server extension directories")
set(APX_SANITIZERS "" CACHE STRING "Comma-separated sanitizers to enable (for example: address,undefined or thread)")
set_property(CACHE APX_SANITIZERS PROPERTY STRINGS "" "address" "address,undefined" "thread")
option(APX_DEBUG "Enable debug-level printouts?" OFF)

if(APX_SANITIZERS)
    if(CMAKE_C_COMPILER_ID MATCHES "GNU|Clang|AppleClang")
        add_compile_options(-fsanitize=${APX_SANITIZERS} -fno-omit-frame-pointer)
        add_link_options(-fsanitize=${APX_SANITIZERS})
    else()
        message(FATAL_ERROR "APX_SANITIZERS is only supported with GCC or Clang")
    endif()
endif()

if (LEAK_CHECK)
    message(STATUS "LEAK_CHECK=${LEAK_CHECK} (C-APX)")
endif()

if (UNIT_TEST OR MSVC)
    set(BUILD_SHARED_LIB_DEFAULT OFF) #We still have some work to do before enabling DLL builds on Windows
else()
    set(BUILD_SHARED_LIB_DEFAULT ON)
endif()

option(BUILD_SHARED_LIBS "Build ${PROJECT_NAME} as a shared library." ${BUILD_SHARED_LIB_DEFAULT})

if(BUILD_SHARED_LIBS)
    set(LIBRARY_TYPE SHARED)
else()
    set(LIBRARY_TYPE STATIC)
endif()
message(STATUS "LIBRARY_TYPE=${LIBRARY_TYPE}")

configure_file(cmake/apx_build_cfg.h.in apx_build_cfg.h)

set(CMAKE_THREAD_PREFER_PTHREAD TRUE)
set(THREADS_PREFER_PTHREAD_FLAG TRUE)
find_package(Threads REQUIRED)

### apx unit tests

set (APX_COMMON_TEST_SUITE
    apx_core/test/testsuite_allocator.c
    apx_core/test/testsuite_attribute_parser.c
    apx_core/test/testsuite_client_socket_connection.c
    apx_core/test/testsuite_client_socket_monitor_connection.c
    apx_core/test/testsuite_client_test_connection.c
    apx_core/test/testsuite_client.c
    apx_core/test/testsuite_compiler_pack.c
    apx_core/test/testsuite_compiler_unpack.c
    apx_core/test/testsuite_computation.c
    apx_core/test/testsuite_data_element.c
    apx_core/test/testsuite_decoder.c
    apx_core/test/testsuite_file_info.c
    apx_core/test/testsuite_file_manager_receiver.c
    apx_core/test/testsuite_file_map.c
    apx_core/test/testsuite_file.c
    apx_core/test/testsuite_node_data.c
    apx_core/test/testsuite_node_manager_client.c
    apx_core/test/testsuite_node_manager_server.c
    apx_core/test/testsuite_node.c
    apx_core/test/testsuite_parser.c
    apx_core/test/testsuite_port_connection_change_entry.c
    apx_core/test/testsuite_port_connector_change_table.c
    apx_core/test/testsuite_port_signature_map.c
    apx_core/test/testsuite_program.c
    apx_core/test/testsuite_remotefile.c
    apx_core/test/testsuite_server_connection.c
    apx_core/test/testsuite_server.c
    apx_core/test/testsuite_signature_parser.c
    apx_core/test/testsuite_util.c
    apx_core/test/testsuite_vm_deserializer.c
    apx_core/test/testsuite_vm_serializer.c
    apx_core/test/testsuite_vm_pack.c
    apx_core/test/testsuite_vm_unpack.c
)

# apx_core library

set (APX_COMMON_HEADERS
    apx_core/include/apx/allocator.h
    apx_core/include/apx/attribute_parser.h
    apx_core/include/apx/byte_port_map.h
    apx_core/include/apx/cfg.h
    apx_core/include/apx/client_connection.h
    apx_core/include/apx/client_internal.h
    apx_core/include/apx/client_test_connection.h
    apx_core/include/apx/client.h
    apx_core/include/apx/command.h
    apx_core/include/apx/compiler.h
    apx_core/include/apx/computation.h
    apx_core/include/apx/connection_base.h
    apx_core/include/apx/connection_interface.h
    apx_core/include/apx/connection_manager.h
    apx_core/include/apx/data_element.h
    apx_core/include/apx/data_signature.h
    apx_core/include/apx/data_type.h
    apx_core/include/apx/decoder.h
    apx_core/include/apx/deserializer.h
    apx_core/include/apx/error.h
    apx_core/include/apx/event_listener.h
    apx_core/include/apx/event_loop.h
    apx_core/include/apx/event.h
    apx_core/include/apx/file_info.h
    apx_core/include/apx/file_manager_defs.h
    apx_core/include/apx/file_manager_receiver.h
    apx_core/include/apx/file_manager_shared.h
    apx_core/include/apx/file_manager_worker.h
    apx_core/include/apx/file_manager.h
    apx_core/include/apx/file_map.h
    apx_core/include/apx/file.h
    apx_core/include/apx/node_cache.h
    apx_core/include/apx/node_data.h
    apx_core/include/apx/node_instance.h
    apx_core/include/apx/node_manager.h
    apx_core/include/apx/node.h
    apx_core/include/apx/numheader.h
    apx_core/include/apx/parser_base.h
    apx_core/include/apx/parser.h
    apx_core/include/apx/port_attribute.h
    apx_core/include/apx/port_connector_change_entry.h
    apx_core/include/apx/port_connector_change_ref.h
    apx_core/include/apx/port_connector_change_table.h
    apx_core/include/apx/port_connector_list.h
    apx_core/include/apx/port_instance.h
    apx_core/include/apx/port_signature_map_entry.h
    apx_core/include/apx/port_signature_map.h
    apx_core/include/apx/port.h
    apx_core/include/apx/program.h
    apx_core/include/apx/remotefile_cfg.h
    apx_core/include/apx/remotefile.h
    apx_core/include/apx/serializer.h
    apx_core/include/apx/server_connection.h
    apx_core/include/apx/server_extension.h
    apx_core/include/apx/server_test_connection.h
    apx_core/include/apx/server.h
    apx_core/include/apx/signature_parser.h
    apx_core/include/apx/socket_client_connection.h
    apx_core/include/apx/stream.h
    apx_core/include/apx/type_attribute.h
    apx_core/include/apx/types.h
    apx_core/include/apx/util.h
    apx_core/include/apx/vm_common.h
    apx_core/include/apx/vm_defs.h
    apx_core/include/apx/vm.h
)

set (APX_COMMON_SOURCES
    apx_core/src/allocator.c
    apx_core/src/attribute_parser.c
    apx_core/src/byte_port_map.c
    apx_core/src/compiler.c
    apx_core/src/client_connection.c
    apx_core/src/client_test_connection.c
    apx_core/src/client.c
    apx_core/src/command.c
    apx_core/src/compiler.c
    apx_core/src/computation.c
    apx_core/src/connection_base.c
    apx_core/src/connection_manager.c
    apx_core/src/data_element.c
    apx_core/src/data_signature.c
    apx_core/src/data_type.c
    apx_core/src/decoder.c
    apx_core/src/deserializer.c
    apx_core/src/event_listener.c
    apx_core/src/event_loop.c
    apx_core/src/file_info.c
    apx_core/src/file.c
    apx_core/src/file_manager_receiver.c
    apx_core/src/file_manager_shared.c
    apx_core/src/file_manager_worker.c
    apx_core/src/file_manager.c
    apx_core/src/file_map.c
    apx_core/src/event.c
    apx_core/src/node_cache.c
    apx_core/src/node_data.c
    apx_core/src/node_instance.c
    apx_core/src/node_manager.c
    apx_core/src/node.c
    apx_core/src/numheader.c
    apx_core/src/parser_base.c
    apx_core/src/parser.c
    apx_core/src/port_attribute.c
    apx_core/src/port_connector_change_entry.c
    apx_core/src/port_connector_change_ref.c
    apx_core/src/port_connector_change_table.c
    apx_core/src/port_connector_list.c
    apx_core/src/port_instance.c
    apx_core/src/port_signature_map_entry.c
    apx_core/src/port_signature_map.c
    apx_core/src/port.c
    apx_core/src/program.c
    apx_core/src/remotefile.c
    apx_core/src/serializer.c
    apx_core/src/server_connection.c
    apx_core/src/server_extension.c
    apx_core/src/server_test_connection.c
    apx_core/src/server.c
    apx_core/src/signature_parser.c
    apx_core/src/socket_client_connection.c
    apx_core/src/stream.c
    apx_core/src/type_attribute.c
    apx_core/src/util.c
    apx_core/src/vm.c
    apx_core/src/vm_common.c
)

## Submodule include
add_subdirectory(adt)
add_subdirectory(bstr)
add_subdirectory(cutil)
add_subdirectory(dtl_json)
add_subdirectory(dtl_type)
add_subdirectory(msocket)

get_directory_property(ADT_HEADER_LIST DIRECTORY adt DEFINITION ADT_HEADER_LIST)
get_directory_property(ADT_SOURCE_LIST DIRECTORY adt DEFINITION ADT_SOURCE_LIST)
get_directory_property(BSTR_HEADER_LIST DIRECTORY bstr DEFINITION BSTR_HEADER_LIST)
get_directory_property(BSTR_SOURCE_LIST DIRECTORY bstr DEFINITION BSTR_SOURCE_LIST)
get_directory_property(CUTIL_HEADER_LIST DIRECTORY cutil DEFINITION CUTIL_HEADER_LIST)
get_directory_property(CUTIL_SOURCE_LIST DIRECTORY cutil DEFINITION CUTIL_SOURCE_LIST)
get_directory_property(BYTE_ORDER_VALUE DIRECTORY cutil DEFINITION BYTE_ORDER_VALUE)
get_directory_property(DTL_JSON_HEADERS DIRECTORY dtl_json DEFINITION DTL_JSON_HEADERS)
get_directory_property(DTL_JSON_SOURCES DIRECTORY dtl_json DEFINITION DTL_JSON_SOURCES)
get_directory_property(DTL_TYPE_HEADER_LIST DIRECTORY dtl_type DEFINITION DTL_TYPE_HEADER_LIST)
get_directory_property(DTL_TYPE_SOURCE_LIST DIRECTORY dtl_type DEFINITION DTL_TYPE_SOURCE_LIST)
get_directory_property(MSOCKET_HEADERS DIRECTORY msocket DEFINITION MSOCKET_HEADERS)
get_directory_property(MSOCKET_SOURCES DIRECTORY msocket DEFINITION MSOCKET_SOURCES)
get_directory_property(MSOCKET_SERVER_HEADERS DIRECTORY msocket DEFINITION MSOCKET_SERVER_HEADERS)
get_directory_property(MSOCKET_SERVER_SOURCES DIRECTORY msocket DEFINITION MSOCKET_SERVER_SOURCES)


add_library(apx_core ${LIBRARY_TYPE}
            ${ADT_HEADER_LIST}
            ${ADT_SOURCE_LIST}
            ${APX_COMMON_HEADERS}
            ${APX_COMMON_SOURCES}
            ${BSTR_HEADER_LIST}
            ${BSTR_SOURCE_LIST}
            ${CUTIL_HEADER_LIST}
            ${CUTIL_SOURCE_LIST}
            ${DTL_JSON_HEADERS}
            ${DTL_JSON_SOURCES}
            ${DTL_TYPE_HEADER_LIST}
            ${DTL_TYPE_SOURCE_LIST}
            ${MSOCKET_HEADERS}
            ${MSOCKET_SOURCES}
            ${MSOCKET_SERVER_HEADERS}
            ${MSOCKET_SERVER_SOURCES}
)

target_compile_definitions(apx_core PUBLIC ADT_RBFH_ENABLE=1)
if(DEFINED BYTE_ORDER_VALUE)
    target_compile_definitions(apx_core PRIVATE PLATFORM_BYTE_ORDER=${BYTE_ORDER_VALUE})
endif()
if (UNIT_TEST)
    target_compile_definitions(apx_core PUBLIC UNIT_TEST)
endif()
if (LEAK_CHECK)
    target_compile_definitions(apx_core PUBLIC MEM_LEAK_CHECK)
endif()
if(MSVC)
    target_compile_definitions(apx_core PUBLIC _CRT_SECURE_NO_WARNINGS)
endif()
if(APX_DEBUG)
    target_compile_definitions(apx_core PUBLIC APX_DEBUG_ENABLE=1)
endif()
if(MSVC)
  target_compile_options(apx_core PRIVATE /W4)
else()
  target_compile_options(apx_core PRIVATE -Wall)
endif()

#Enable later once all needed symbols have been chosen for shared library export
#target_compile_options(apx_core PRIVATE -fvisibility=hidden)

target_link_libraries(apx_core PRIVATE Threads::Threads)

target_include_directories(apx_core PUBLIC
"${PROJECT_BINARY_DIR}"
"${CMAKE_CURRENT_SOURCE_DIR}/adt/inc"
"${CMAKE_CURRENT_SOURCE_DIR}/apx_core/include"
"${CMAKE_CURRENT_SOURCE_DIR}/bstr/inc"
"${CMAKE_CURRENT_SOURCE_DIR}/cutil/inc"
"${CMAKE_CURRENT_SOURCE_DIR}/dtl_type/inc"
"${CMAKE_CURRENT_SOURCE_DIR}/dtl_json/inc"
"${CMAKE_CURRENT_SOURCE_DIR}/msocket/inc"
)

set_target_properties(apx_core PROPERTIES VERSION ${apx_core_VERSION} SOVERSION ${apx_core_VERSION_MAJOR})

install(
  TARGETS apx_core
  LIBRARY DESTINATION lib
)

### Extensions
add_subdirectory(extension)

### Out-of-tree Extensions
set(APX_REGISTERED_EXTENSION_NAMES "")
foreach(ext_dir IN LISTS APX_SERVER_EXTENSION_DIRS)
    if(IS_DIRECTORY "${ext_dir}")
        get_filename_component(ext_name "${ext_dir}" NAME)
        if(ext_name IN_LIST APX_REGISTERED_EXTENSION_NAMES)
            message(FATAL_ERROR "Duplicate extension directory name '${ext_name}' in APX_SERVER_EXTENSION_DIRS: ${ext_dir}")
        endif()
        list(APPEND APX_REGISTERED_EXTENSION_NAMES "${ext_name}")
        add_subdirectory("${ext_dir}" "${CMAKE_CURRENT_BINARY_DIR}/custom_extensions/${ext_name}")
    else()
        message(FATAL_ERROR "APX_SERVER_EXTENSION_DIRS contains non-existent directory: ${ext_dir}")
    endif()
endforeach()

# Generate extensions_cfg.c based on all registered in-tree and out-of-tree extensions
set(GENERATED_EXTENSIONS_CFG "${PROJECT_BINARY_DIR}/extensions_cfg.c")
apx_generate_server_extension_registry("${GENERATED_EXTENSIONS_CFG}")

### Applications
if(NOT UNIT_TEST)
    add_subdirectory(app/apx_node)
    add_subdirectory(app/apx_control)
    #add_subdirectory(app/apx_perf_test)#
endif()
if(APX_BUILD_SERVER)
    add_subdirectory(app/apx_server)
endif()

### Executable apx_unit
if(CMAKE_PROJECT_NAME STREQUAL PROJECT_NAME)
    if (UNIT_TEST)
        add_executable(apx_unit
            apx_core/test/test_main.c
            ${APX_COMMON_TEST_SUITE}
            extension/apx_socket_extension/test/testsuite_server_socket_connection.c
            extension/apx_socket_extension/test/testsuite_socket_server_extension.c
            extension/apx_text_log_extension/test/testsuite_server_text_log.c
            extension/apx_monitor_extension/test/testsuite_monitor_extension.c
            extension/apx_monitor_extension/test/testsuite_server_monitor_state.c
            app/apx_server/src/server_cfg.c
            ${GENERATED_EXTENSIONS_CFG}
            app/apx_server/test/testsuite_server_cfg.c
        )
        target_link_libraries(apx_unit PRIVATE
            apx_core
            apx_socket_extension
            apx_text_log_extension
            apx_monitor_extension
            msocket_testsocket
            cutest
            Threads::Threads
        )
        target_include_directories(apx_unit PRIVATE
            "${PROJECT_BINARY_DIR}"
            "${CMAKE_CURRENT_SOURCE_DIR}/apx_core/test"
            "${CMAKE_CURRENT_SOURCE_DIR}/app/apx_server/src"
            "${CMAKE_CURRENT_SOURCE_DIR}/extension/apx_socket_extension/include"
            "${CMAKE_CURRENT_SOURCE_DIR}/extension/apx_text_log_extension/include"
            "${CMAKE_CURRENT_SOURCE_DIR}/extension/apx_monitor_extension/include"
        )
        target_compile_definitions(apx_unit PRIVATE UNIT_TEST)

        if (LEAK_CHECK)
            target_compile_definitions(apx_unit PRIVATE MEM_LEAK_CHECK)
            target_link_libraries(apx_unit PRIVATE cutil)
        endif()

        enable_testing()
        add_test(apx_test ${CMAKE_CURRENT_BINARY_DIR}/apx_unit)
        set_tests_properties(apx_test PROPERTIES PASS_REGULAR_EXPRESSION "OK \\([0-9]+ tests\\)")
    endif()
endif()
###