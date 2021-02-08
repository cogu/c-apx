cmake_minimum_required(VERSION 3.10)


project(apx LANGUAGES C VERSION 0.3.1)

set(ADT_RBFH_ENABLE ON CACHE BOOL "This must always be enabled in c-apx" FORCE)

include(adt/cmake/BuildTypes.cmake)
include(adt/cmake/LeakCheck.cmake)
include(adt/cmake/UnitTest.cmake)
include(cmake/SetEnv.cmake)

option(apx_ALPHA_BUILD "Is this an alpha build?" ON)
option(BUILD_DEFAULT_SERVER "Build default APX server?" ON)
option(APX_DEBUG "Enable debug-level printouts?" OFF)

if (LEAK_CHECK)
    message(STATUS "LEAK_CHECK=${LEAK_CHECK} (C-APX)")
endif()

if (UNIT_TEST OR MSVC)
    set(BUILD_SHARED_LIB_DEFAULT OFF) #We still have some work to do before enabling DLL builds on Windows
else()
    set(BUILD_SHARED_LIB_DEFAULT ON)
endif()

if (apx_ALPHA_BUILD)
    set (apx_ALPHA_BUILD_STR "a")
else()
    set (apx_ALPHA_BUILD_STR "")
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
    apx/test/testsuite_allocator.c
    apx/test/testsuite_attribute_parser.c
    apx/test/testsuite_client_socket_connection.c
    apx/test/testsuite_client_test_connection.c
    apx/test/testsuite_client.c
    apx/test/testsuite_compiler_pack.c
    apx/test/testsuite_compiler_unpack.c
    apx/test/testsuite_computation.c
    apx/test/testsuite_data_element.c
    apx/test/testsuite_decoder.c
    apx/test/testsuite_file_info.c
    apx/test/testsuite_file_manager_receiver.c
    apx/test/testsuite_file_map.c
    apx/test/testsuite_file.c
    apx/test/testsuite_node_data.c
    apx/test/testsuite_node_manager_client.c
    apx/test/testsuite_node_manager_server.c
    apx/test/testsuite_node.c
    apx/test/testsuite_parser.c
    apx/test/testsuite_port_connection_change_entry.c
    apx/test/testsuite_port_connector_change_table.c
    apx/test/testsuite_port_signature_map.c
    apx/test/testsuite_program.c
    apx/test/testsuite_remotefile.c
    apx/test/testsuite_server_connection.c
    apx/test/testsuite_server.c
    apx/test/testsuite_signature_parser.c
    apx/test/testsuite_util.c
    apx/test/testsuite_vm_deserializer.c
    apx/test/testsuite_vm_serializer.c
    apx/test/testsuite_vm.c
)

set (APX_SERVER_SOCKET_EXTENSION_TEST_SUITE
    apx/test/extension/testsuite_apx_server_socket_connection.c
    apx/test/extension/testsuite_apx_socket_server_extension.c
)

#Library apx_srv_sock_ext
set (APX_SERVER_SOCKET_EXTENSION_HEADERS
    apx/include/apx/extension/socket_server_connection.h
    apx/include/apx/extension/socket_server_extension.h
    apx/include/apx/extension/socket_server.h
)

set (APX_SERVER_SOCKET_EXTENSION_SOURCES
    apx/src/extension/socket_server_connection.c
    apx/src/extension/socket_server_extension.c
    apx/src/extension/socket_server.c
)

add_library(apx_srv_sock_ext ${LIBRARY_TYPE} ${APX_SERVER_SOCKET_EXTENSION_HEADERS} ${APX_SERVER_SOCKET_EXTENSION_SOURCES})
if (LEAK_CHECK)
    target_compile_definitions(apx_srv_sock_ext PRIVATE MEM_LEAK_CHECK)
endif()
if (UNIT_TEST)
    target_link_libraries(apx_srv_sock_ext PRIVATE msocket_testsocket)
    target_compile_definitions(apx_srv_sock_ext PRIVATE UNIT_TEST)
endif()
if(APX_DEBUG)
    target_compile_definitions(apx_srv_sock_ext PUBLIC APX_DEBUG_ENABLE=1)
endif()
target_link_libraries(apx_srv_sock_ext PRIVATE apx)
target_include_directories(apx_srv_sock_ext PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/apx/include)
set_target_properties(apx_srv_sock_ext PROPERTIES VERSION ${apx_VERSION} SOVERSION ${apx_VERSION_MAJOR})

install(
  TARGETS apx_srv_sock_ext
  LIBRARY DESTINATION lib
  COMPONENT Server
)

###

## Submodule include
add_subdirectory(adt)
add_subdirectory(bstr)
add_subdirectory(cutil)
add_subdirectory(dtl_json)
add_subdirectory(dtl_type)
add_subdirectory(msocket)
add_subdirectory(app/apx_node)
#add_subdirectory(app/apx_control) CG-20210204: Temporary disable while apx_client class is being upgraded
if(BUILD_DEFAULT_SERVER)
    add_subdirectory(app/apx_server)
endif()
###

# apx library

set (APX_COMMON_HEADERS
    apx/include/apx/allocator.h
    apx/include/apx/attribute_parser.h
    apx/include/apx/byte_port_map.h
    apx/include/apx/cfg.h
    apx/include/apx/client_connection.h
    apx/include/apx/client_internal.h
    apx/include/apx/client_test_connection.h
    apx/include/apx/client.h
    apx/include/apx/command.h
    apx/include/apx/compiler.h
    apx/include/apx/computation.h
    apx/include/apx/connection_base.h
    apx/include/apx/connection_interface.h
    apx/include/apx/connection_manager.h
    apx/include/apx/data_element.h
    apx/include/apx/data_signature.h
    apx/include/apx/data_type.h
    apx/include/apx/decoder.h
    apx/include/apx/deserializer.h
    apx/include/apx/error.h
    apx/include/apx/event_listener.h
    apx/include/apx/event_loop.h
    apx/include/apx/event.h
    apx/include/apx/file_info.h
    apx/include/apx/file_manager_defs.h
    apx/include/apx/file_manager_receiver.h
    apx/include/apx/file_manager_shared.h
    apx/include/apx/file_manager_worker.h
    apx/include/apx/file_manager.h
    apx/include/apx/file_map.h
    apx/include/apx/file.h
    apx/include/apx/log_event.h
    apx/include/apx/node_cache.h
    apx/include/apx/node_data.h
    apx/include/apx/node_instance.h
    apx/include/apx/node_manager.h
    apx/include/apx/node.h
    apx/include/apx/numheader.h
    apx/include/apx/parser_base.h
    apx/include/apx/parser.h
    apx/include/apx/port_attribute.h
    apx/include/apx/port_connector_change_entry.h
    apx/include/apx/port_connector_change_ref.h
    apx/include/apx/port_connector_change_table.h
    apx/include/apx/port_connector_list.h
    apx/include/apx/port_instance.h
    apx/include/apx/port_signature_map_entry.h
    apx/include/apx/port_signature_map.h
    apx/include/apx/port.h
    apx/include/apx/program.h
    apx/include/apx/remotefile_cfg.h
    apx/include/apx/remotefile.h
    apx/include/apx/serializer.h
    apx/include/apx/server_connection.h
    apx/include/apx/server_extension.h
    apx/include/apx/server_test_connection.h
    apx/include/apx/server.h
    apx/include/apx/signature_parser.h
    apx/include/apx/socket_client_connection.h
    apx/include/apx/stream.h
    apx/include/apx/type_attribute.h
    apx/include/apx/types.h
    apx/include/apx/util.h
    apx/include/apx/vm_common.h
    apx/include/apx/vm_defs.h
    apx/include/apx/vm.h
)

set (APX_COMMON_SOURCES
    apx/src/allocator.c
    apx/src/attribute_parser.c
    apx/src/byte_port_map.c
    apx/src/compiler.c
    apx/src/client_connection.c
    apx/src/client_test_connection.c
    apx/src/client.c
    apx/src/command.c
    apx/src/compiler.c
    apx/src/computation.c
    apx/src/connection_base.c
    apx/src/connection_manager.c
    apx/src/data_element.c
    apx/src/data_signature.c
    apx/src/data_type.c
    apx/src/decoder.c
    apx/src/deserializer.c
    apx/src/event_listener.c
    apx/src/event_loop.c
    apx/src/file_info.c
    apx/src/file.c
    apx/src/file_manager_receiver.c
    apx/src/file_manager_shared.c
    apx/src/file_manager_worker.c
    apx/src/file_manager.c
    apx/src/file_map.c
    apx/src/log_event.c
    apx/src/node_cache.c
    apx/src/node_data.c
    apx/src/node_instance.c
    apx/src/node_manager.c
    apx/src/node.c
    apx/src/numheader.c
    apx/src/parser_base.c
    apx/src/parser.c
    apx/src/port_attribute.c
    apx/src/port_connector_change_entry.c
    apx/src/port_connector_change_ref.c
    apx/src/port_connector_change_table.c
    apx/src/port_connector_list.c
    apx/src/port_instance.c
    apx/src/port_signature_map_entry.c
    apx/src/port_signature_map.c
    apx/src/port.c
    apx/src/program.c
    apx/src/remotefile.c
    apx/src/serializer.c
    apx/src/server_connection.c
    apx/src/server_extension.c
    apx/src/server_test_connection.c
    apx/src/server.c
    apx/src/signature_parser.c
    apx/src/socket_client_connection.c
    apx/src/stream.c
    apx/src/type_attribute.c
    apx/src/util.c
    apx/src/vm.c
    apx/src/vm_common.c
)

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


add_library(apx ${LIBRARY_TYPE}
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

target_compile_definitions(apx PUBLIC ADT_RBFH_ENABLE=1)
if(DEFINED BYTE_ORDER_VALUE)
    target_compile_definitions(apx PRIVATE PLATFORM_BYTE_ORDER=${BYTE_ORDER_VALUE})
endif()
if (UNIT_TEST)
    target_compile_definitions(apx PUBLIC UNIT_TEST)
endif()
if (LEAK_CHECK)
    target_compile_definitions(apx PUBLIC MEM_LEAK_CHECK)
endif()
if(MSVC)
    target_compile_definitions(apx PUBLIC _CRT_SECURE_NO_WARNINGS)
endif()
if(APX_DEBUG)
    target_compile_definitions(apx PUBLIC APX_DEBUG_ENABLE=1)
endif()

#Enable later once all needed symbols have been chosen for shared library export
#target_compile_options(apx PRIVATE -fvisibility=hidden)

target_link_libraries(apx PRIVATE Threads::Threads)

target_include_directories(apx PUBLIC
"${PROJECT_BINARY_DIR}"
"${CMAKE_CURRENT_SOURCE_DIR}/adt/inc"
"${CMAKE_CURRENT_SOURCE_DIR}/apx/include"
"${CMAKE_CURRENT_SOURCE_DIR}/bstr/inc"
"${CMAKE_CURRENT_SOURCE_DIR}/cutil/inc"
"${CMAKE_CURRENT_SOURCE_DIR}/dtl_type/inc"
"${CMAKE_CURRENT_SOURCE_DIR}/dtl_json/inc"
"${CMAKE_CURRENT_SOURCE_DIR}/msocket/inc"
)

set_target_properties(apx PROPERTIES VERSION ${apx_VERSION} SOVERSION ${apx_VERSION_MAJOR})

install(
  TARGETS apx
  LIBRARY DESTINATION lib
)

###

### Executable apx_unit
if(CMAKE_PROJECT_NAME STREQUAL PROJECT_NAME)
    if (UNIT_TEST)
        add_executable(apx_unit
            apx/test/test_main.c
            ${APX_COMMON_TEST_SUITE}
            ${APX_SERVER_SOCKET_EXTENSION_TEST_SUITE}
        )
        target_link_libraries(apx_unit PRIVATE
            apx
            apx_srv_sock_ext
            msocket_testsocket
            cutest
            Threads::Threads
        )
        target_include_directories(apx_unit PRIVATE
                                "${PROJECT_BINARY_DIR}"
                                "${CMAKE_CURRENT_SOURCE_DIR}/apx/test"
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