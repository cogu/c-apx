cmake_minimum_required(VERSION 3.14)


project(apx_control LANGUAGES C VERSION 0.1.0)

set (APX_CONTROL_HEADERS
    ${CMAKE_CURRENT_SOURCE_DIR}/inc/message_client_connection.h
)

set (APX_CONTROL_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/src/apx_control_main.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/message_client_connection.c
)

add_executable(apx_control ${APX_CONTROL_HEADERS} ${APX_CONTROL_SOURCES})
target_link_libraries(apx_control PRIVATE
    apx
    Threads::Threads
)

target_include_directories(apx_control PRIVATE
    ${PROJECT_BINARY_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/inc
)