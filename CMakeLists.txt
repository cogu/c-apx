cmake_minimum_required(VERSION 3.14)


project(apx LANGUAGES C VERSION 0.3.0)

set(ADT_RBFH_ENABLE ON CACHE BOOL "This must always be enabled in c-apx" FORCE)

include(adt/cmake/BuildTypes.cmake)
include(adt/cmake/LeakCheck.cmake)
include(adt/cmake/UnitTest.cmake)
include(cmake/SetEnv.cmake)

option(apx_ALPHA_BUILD "Is this an alpha build?" ON)

if (LEAK_CHECK)
    message(STATUS "LEAK_CHECK=${LEAK_CHECK} (C-APX)")
endif()

if (UNIT_TEST)
    message(STATUS "UNIT_TEST=${UNIT_TEST} (C-APX)")
    set(BUILD_SHARED_LIB_DEFAULT OFF)
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

configure_file(cmake/apx_build_cfg.h.in apx_build_cfg.h)

set(CMAKE_THREAD_PREFER_PTHREAD TRUE)
set(THREADS_PREFER_PTHREAD_FLAG TRUE)
find_package(Threads REQUIRED)

### apx unit tests
set (APX_COMMON_TEST_UTIL
    apx/common/test/apx_connectionEventSpy.c
    apx/common/test/apx_connectionEventSpy.h
    apx/common/test/apx_test_nodes.c
    apx/common/test/apx_test_nodes.h
    apx/common/test/apx_transmitHandlerSpy.c
    apx/common/test/apx_transmitHandlerSpy.h
)

set (APX_COMMON_TEST_SUITE
    apx/common/test/testsuite_apx_allocator.c
    apx/common/test/testsuite_apx_attributeParser.c
    apx/common/test/testsuite_apx_bytePortMap.c
    apx/common/test/testsuite_apx_compiler.c
    apx/common/test/testsuite_apx_connectionBase.c
    apx/common/test/testsuite_apx_dataElement.c
    apx/common/test/testsuite_apx_dataSignature.c
    apx/common/test/testsuite_apx_datatype.c
    apx/common/test/testsuite_apx_eventLoop.c
    apx/common/test/testsuite_apx_file.c
    apx/common/test/testsuite_apx_fileManager.c
    apx/common/test/testsuite_apx_fileManagerReceiver.c
    apx/common/test/testsuite_apx_fileManagerShared.c
    apx/common/test/testsuite_apx_fileManagerWorker.c
    apx/common/test/testsuite_apx_fileMap.c
    apx/common/test/testsuite_apx_node.c
    apx/common/test/testsuite_apx_nodeData.c
    apx/common/test/testsuite_apx_nodeInfo.c
    apx/common/test/testsuite_apx_nodeInstance.c
    apx/common/test/testsuite_apx_nodeManager.c
    apx/common/test/testsuite_apx_parser.c
    apx/common/test/testsuite_apx_port.c
    apx/common/test/testsuite_apx_portConnectionChangeEntry.c
    apx/common/test/testsuite_apx_portConnectorChangeTable.c
    apx/common/test/testsuite_apx_portSignatureMap.c
    apx/common/test/testsuite_apx_util.c
    apx/common/test/testsuite_apx_vm.c
    apx/common/test/testsuite_apx_vmDeserializer.c
    apx/common/test/testsuite_apx_vmSerializer.c
)

set (APX_CLIENT_TEST_UTIL
    apx/client/test/apx_clientEventListenerSpy.h
    apx/client/test/apx_clientEventListenerSpy.c
)

set (APX_CLIENT_TEST_SUITE
    apx/client/test/testsuite_apx_client_socketConnection.c
    apx/client/test/testsuite_apx_client_testConnection.c
    apx/client/test/testsuite_apx_client.c
)

set (APX_SERVER_TEST_SUITE
    apx/server/test/testsuite_apx_dataRouting.c
    apx/server/test/testsuite_apx_serverConnection.c
)

set (REMOTEFILE_TEST_SUITE
    remotefile/test/testsuite_remotefile.c
)
###

### Library apx_srv_sock_ext
set (APX_SERVER_SOCKET_EXTENSION_HEADERS
    apx/server_extension/socket/inc/apx_serverSocketConnection.h
    apx/server_extension/socket/inc/apx_socketServer.h
    apx/server_extension/socket/inc/apx_socketServerExtension.h
)
set (APX_SERVER_SOCKET_EXTENSION_SOURCES
    apx/server_extension/socket/src/apx_serverSocketConnection.c
    apx/server_extension/socket/src/apx_socketServer.c
    apx/server_extension/socket/src/apx_socketServerExtension.c
)

set (APX_SERVER_SOCKET_EXTENSION_TEST_SUITE
    apx/server_extension/socket/test/testsuite_apx_serverSocketConnection.c
    apx/server_extension/socket/test/testsuite_apx_socketServerExtension.c
)

add_library(apx_srv_sock_ext ${LIBRARY_TYPE} ${APX_SERVER_SOCKET_EXTENSION_HEADERS} ${APX_SERVER_SOCKET_EXTENSION_SOURCES})
if (LEAK_CHECK)
    target_compile_definitions(apx_srv_sock_ext PRIVATE MEM_LEAK_CHECK)
endif()
if (UNIT_TEST)
    target_link_libraries(apx_srv_sock_ext PRIVATE msocket_testsocket)
    target_compile_definitions(apx_srv_sock_ext PRIVATE UNIT_TEST)
endif()
target_link_libraries(apx_srv_sock_ext PRIVATE apx)
target_include_directories(apx_srv_sock_ext PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/apx/server_extension/socket/inc)
###

## Submodule include
add_subdirectory(adt)
add_subdirectory(bstr)
add_subdirectory(cutil)
add_subdirectory(dtl_json)
add_subdirectory(dtl_type)
add_subdirectory(msocket)
add_subdirectory(app/apx_listen)
add_subdirectory(app/apx_control)
###

# apx library

set (APX_COMMON_HEADERS
    apx/common/inc/apx_allocator.h
    apx/common/inc/apx_attributeParser.h
    apx/common/inc/apx_bytePortMap.h
    apx/common/inc/apx_cfg.h
    apx/common/inc/apx_compiler.h
    apx/common/inc/apx_connectionBase.h
    apx/common/inc/apx_dataElement.h
    apx/common/inc/apx_dataSignature.h
    apx/common/inc/apx_dataType.h
    apx/common/inc/apx_error.h
    apx/common/inc/apx_event.h
    apx/common/inc/apx_eventListener.h
    apx/common/inc/apx_eventLoop.h
    apx/common/inc/apx_file.h
    apx/common/inc/apx_fileCache.h
    apx/common/inc/apx_fileInfo.h
    apx/common/inc/apx_fileManager.h
    apx/common/inc/apx_fileManagerDefs.h
    apx/common/inc/apx_fileManagerReceiver.h
    apx/common/inc/apx_fileManagerShared.h
    apx/common/inc/apx_fileManagerWorker.h
    apx/common/inc/apx_fileMap.h
    apx/common/inc/apx_logEvent.h
    apx/common/inc/apx_msg.h
    apx/common/inc/apx_node.h
    apx/common/inc/apx_nodeData.h
    apx/common/inc/apx_nodeInfo.h
    apx/common/inc/apx_nodeInstance.h
    apx/common/inc/apx_nodeManager.h
    apx/common/inc/apx_parser.h
    apx/common/inc/apx_port.h
    apx/common/inc/apx_portAttributes.h
    apx/common/inc/apx_portConnectorChangeEntry.h
    apx/common/inc/apx_portConnectorChangeRef.h
    apx/common/inc/apx_portConnectorChangeTable.h
    apx/common/inc/apx_portConnectorList.h
    apx/common/inc/apx_portDataProps.h
    apx/common/inc/apx_portDataRef.h
    apx/common/inc/apx_portSignatureMap.h
    apx/common/inc/apx_portSignatureMapEntry.h
    apx/common/inc/apx_stream.h
    apx/common/inc/apx_transmitHandler.h
    apx/common/inc/apx_typeAttribute.h
    apx/common/inc/apx_types.h
    apx/common/inc/apx_util.h
    apx/common/inc/apx_vm.h
    apx/common/inc/apx_vmdefs.h
    apx/common/inc/apx_vmDeserializer.h
    apx/common/inc/apx_vmSerializer.h
)

set (APX_COMMON_SOURCES
    apx/common/src/apx_allocator.c
    apx/common/src/apx_attributeParser.c
    apx/common/src/apx_bytePortMap.c
    apx/common/src/apx_compiler.c
    apx/common/src/apx_connectionBase.c
    apx/common/src/apx_dataElement.c
    apx/common/src/apx_dataSignature.c
    apx/common/src/apx_dataType.c
    apx/common/src/apx_event.c
    apx/common/src/apx_eventListener.c
    apx/common/src/apx_eventLoop.c
    apx/common/src/apx_file.c
    apx/common/src/apx_fileCache.c
    apx/common/src/apx_fileInfo.c
    apx/common/src/apx_fileManager.c
    apx/common/src/apx_fileManagerReceiver.c
    apx/common/src/apx_fileManagerShared.c
    apx/common/src/apx_fileManagerWorker.c
    apx/common/src/apx_fileMap.c
    apx/common/src/apx_logEvent.c
    apx/common/src/apx_node.c
    apx/common/src/apx_nodeData.c
    apx/common/src/apx_nodeInfo.c
    apx/common/src/apx_nodeInstance.c
    apx/common/src/apx_nodeManager.c
    apx/common/src/apx_parser.c
    apx/common/src/apx_port.c
    apx/common/src/apx_portAttributes.c
    apx/common/src/apx_portConnectorChangeEntry.c
    apx/common/src/apx_portConnectorChangeRef.c
    apx/common/src/apx_portConnectorChangeTable.c
    apx/common/src/apx_portConnectorList.c
    apx/common/src/apx_portDataProps.c
    apx/common/src/apx_portDataRef.c
    apx/common/src/apx_portSignatureMap.c
    apx/common/src/apx_portSignatureMapEntry.c
    apx/common/src/apx_stream.c
    apx/common/src/apx_typeAttribute.c
    apx/common/src/apx_util.c
    apx/common/src/apx_vm.c
    apx/common/src/apx_vmDeserializer.c
    apx/common/src/apx_vmSerializer.c
)

set (APX_CLIENT_HEADERS
    apx/client/inc/apx_client.h
    apx/client/inc/apx_clientConnectionBase.h
    apx/client/inc/apx_clientInternal.h
    apx/client/inc/apx_clientSocketConnection.h
)
set (APX_CLIENT_SOURCES
    apx/client/src/apx_client.c
    apx/client/src/apx_clientConnectionBase.c
    apx/client/src/apx_clientSocketConnection.c
)

if (UNIT_TEST)
    list(APPEND APX_CLIENT_HEADERS apx/client/inc/apx_clientTestConnection.h)
    list(APPEND APX_CLIENT_SOURCES apx/client/src/apx_clientTestConnection.c)
endif()


set (APX_SERVER_HEADERS
    apx/server/inc/apx_connectionManager.h
    apx/server/inc/apx_server.h
    apx/server/inc/apx_serverConnectionBase.h
    apx/server/inc/apx_serverExtension.h
)

set (APX_SERVER_SOURCES
    apx/server/src/apx_connectionManager.c
    apx/server/src/apx_server.c
    apx/server/src/apx_serverConnectionBase.c
    apx/server/src/apx_serverExtension.c
)

if (UNIT_TEST)
    list(APPEND APX_SERVER_HEADERS apx/server/inc/apx_serverTestConnection.h)
    list(APPEND APX_SERVER_SOURCES apx/server/src/apx_serverTestConnection.c)
endif()

set (REMOTEFILE_HEADERS
    remotefile/inc/numheader.h
    remotefile/inc/rmf_cfg.h
    remotefile/inc/rmf.h
)

set (REMOTEFILE_SOURCES
    remotefile/src/numheader.c
    remotefile/src/rmf.c
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
            ${APX_CLIENT_HEADERS}
            ${APX_CLIENT_SOURCES}
            ${APX_SERVER_HEADERS}
            ${APX_SERVER_SOURCES}
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
            ${REMOTEFILE_HEADERS}
            ${REMOTEFILE_SOURCES}
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
#Enable later once all needed symbols have been chosen for shared library export
#target_compile_options(apx PRIVATE -fvisibility=hidden)

target_link_libraries(apx PRIVATE Threads::Threads)

target_include_directories(apx PUBLIC
"${PROJECT_BINARY_DIR}"
"${CMAKE_CURRENT_SOURCE_DIR}/adt/inc"
"${CMAKE_CURRENT_SOURCE_DIR}/apx/common/inc"
"${CMAKE_CURRENT_SOURCE_DIR}/apx/client/inc"
"${CMAKE_CURRENT_SOURCE_DIR}/apx/server/inc"
"${CMAKE_CURRENT_SOURCE_DIR}/apx/server_extension/socket/inc"
"${CMAKE_CURRENT_SOURCE_DIR}/bstr/inc"
"${CMAKE_CURRENT_SOURCE_DIR}/cutil/inc"
"${CMAKE_CURRENT_SOURCE_DIR}/dtl_type/inc"
"${CMAKE_CURRENT_SOURCE_DIR}/dtl_json/inc"
"${CMAKE_CURRENT_SOURCE_DIR}/msocket/inc"
"${CMAKE_CURRENT_SOURCE_DIR}/remotefile/inc"
)

###

### Executable apx_unit
if(CMAKE_PROJECT_NAME STREQUAL PROJECT_NAME)
    if (UNIT_TEST)
        add_executable(apx_unit
            apx/common/test/test_main.c
            ${REMOTEFILE_TEST_SUITE}
            ${APX_COMMON_TEST_SUITE}
            ${APX_CLIENT_TEST_SUITE}
            ${APX_SERVER_TEST_SUITE}
            ${APX_COMMON_TEST_UTIL}
            ${APX_CLIENT_TEST_UTIL}
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
                                "${CMAKE_CURRENT_SOURCE_DIR}/apx/common/test"
                                )
        target_compile_definitions(apx_unit PRIVATE UNIT_TEST)

        if (LEAK_CHECK)
            target_compile_definitions(apx_unit PRIVATE MEM_LEAK_CHECK)
            target_link_libraries(apx_unit PRIVATE cutil)
        endif()
    endif()
endif()
###

### Executable apx_server
if(CMAKE_PROJECT_NAME STREQUAL PROJECT_NAME)
    add_executable(apx_server apx/server_main/server_main.c)
    target_link_libraries(apx_server PRIVATE
    apx
    apx_srv_sock_ext
    Threads::Threads
    )
    if (UNIT_TEST)
        target_link_libraries(apx_server PRIVATE cutil)
    endif()
    if (LEAK_CHECK)
        target_compile_definitions(apx_server PRIVATE MEM_LEAK_CHECK)
    endif()
    target_include_directories(apx_server PRIVATE
                    "${PROJECT_BINARY_DIR}"
                    )
    target_compile_definitions(apx_server PRIVATE USE_CONFIGURATION_FILE)
endif()
###