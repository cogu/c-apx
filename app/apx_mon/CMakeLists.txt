cmake_minimum_required(VERSION 3.20)

project(apx-mon LANGUAGES C)

set(APX_MON_HEADERS
    ${CMAKE_CURRENT_SOURCE_DIR}/include/apx_diag_client_file.h
    ${CMAKE_CURRENT_SOURCE_DIR}/include/apx_diag_client.h
    ${CMAKE_CURRENT_SOURCE_DIR}/include/apx_diag_client_notification.h
)

set(APX_MON_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/src/apx_diag_client_file.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/apxmon_main.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/apx_diag_client.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/apx_diag_client_notification.c
)

add_executable(apxmon ${APX_MON_HEADERS} ${APX_MON_SOURCES})
target_link_libraries(apxmon PRIVATE
    apx_core
    Threads::Threads
)

target_include_directories(apxmon PRIVATE
    ${PROJECT_BINARY_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

install(
  TARGETS apxmon
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
  COMPONENT App
)
