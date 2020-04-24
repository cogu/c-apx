cmake_minimum_required(VERSION 3.14)


project(apx_listen LANGUAGES C VERSION 0.1.0)

set (APX_LISTEN_HEADER_LIST
    inc/apx_connection.h
    inc/json_server_connection.h
    inc/json_server.h
)

set (APX_LISTEN_SOURCE_LIST
    src/apx_connection.c
    src/apx_listen_main.c
    src/json_server_connection.c
    src/json_server.c
)

add_executable(apx_listen ${APX_LISTEN_HEADER_LIST} ${APX_LISTEN_SOURCE_LIST})
target_link_libraries(apx_listen PRIVATE
    apx
    Threads::Threads
)

target_include_directories(apx_listen PRIVATE
"${PROJECT_BINARY_DIR}"
"${CMAKE_CURRENT_SOURCE_DIR}/inc"
)