cmake_minimum_required(VERSION 3.20)

project(apx-log LANGUAGES C)

set(APX_LOG_HEADERS
    ${CMAKE_CURRENT_SOURCE_DIR}/include/apx_log_file.h
    ${CMAKE_CURRENT_SOURCE_DIR}/include/apx_log_event.h
    ${CMAKE_CURRENT_SOURCE_DIR}/include/apx_log_client.h
)

set(APX_LOG_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/src/apx_log_file.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/apx_log_event.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/apx_log_client.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/apx_log_main.c
)

add_executable(apx-log ${APX_LOG_HEADERS} ${APX_LOG_SOURCES})
target_link_libraries(apx-log PRIVATE
    apx_core
    Threads::Threads
)

target_include_directories(apx-log PRIVATE
    ${PROJECT_BINARY_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

install(
  TARGETS apx-log
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
  COMPONENT App
)

