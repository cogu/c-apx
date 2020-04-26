cmake_minimum_required(VERSION 3.14)


project(apx_listen LANGUAGES C)

set (APX_LISTEN_HEADER_LIST
    ${CMAKE_CURRENT_SOURCE_DIR}/inc/apx_connection.h
    ${CMAKE_CURRENT_SOURCE_DIR}/inc/json_server_connection.h
    ${CMAKE_CURRENT_SOURCE_DIR}/inc/json_server.h
)

set (APX_LISTEN_SOURCE_LIST
    ${CMAKE_CURRENT_SOURCE_DIR}/src/apx_connection.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/apx_listen_main.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/json_server_connection.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/json_server.c
)

add_executable(apx_listen ${APX_LISTEN_HEADER_LIST} ${APX_LISTEN_SOURCE_LIST})
target_link_libraries(apx_listen PRIVATE
    apx
    Threads::Threads
)

target_include_directories(apx_listen PRIVATE
    ${PROJECT_BINARY_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/inc
)

install(
  TARGETS apx_listen
  RUNTIME DESTINATION bin
  COMPONENT App
)