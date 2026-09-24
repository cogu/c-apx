cmake_minimum_required(VERSION 3.20)


project(apx-node LANGUAGES C)

set (APX_NODE_HEADER_LIST
    ${CMAKE_CURRENT_SOURCE_DIR}/include/apx_connection.h
    ${CMAKE_CURRENT_SOURCE_DIR}/include/json_server_connection.h
    ${CMAKE_CURRENT_SOURCE_DIR}/include/json_server.h
)

set (APX_NODE_SOURCE_LIST
    ${CMAKE_CURRENT_SOURCE_DIR}/src/apx_connection.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/apx_node_main.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/json_server_connection.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/json_server.c
)

add_executable(apx-node ${APX_NODE_HEADER_LIST} ${APX_NODE_SOURCE_LIST})
target_link_libraries(apx-node PRIVATE
    apx_core
    Threads::Threads
)
target_include_directories(apx-node PRIVATE
    ${PROJECT_BINARY_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)
if(APX_DEBUG)
    target_compile_definitions(apx-node PUBLIC APX_DEBUG_ENABLE=1)
endif()

install(
  TARGETS apx-node
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
  COMPONENT App
)