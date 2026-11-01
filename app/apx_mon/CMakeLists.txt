cmake_minimum_required(VERSION 3.20)

project(apx-mon LANGUAGES C)

set(APX_MON_HEADERS
    ${CMAKE_CURRENT_SOURCE_DIR}/include/apx_diag_client_file.h
    ${CMAKE_CURRENT_SOURCE_DIR}/include/apx_diag_client.h
    ${CMAKE_CURRENT_SOURCE_DIR}/include/apx_diag_client_notification.h
)

set(APX_MON_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/src/apx_diag_client_file.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/apx_mon_main.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/apx_diag_client.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/apx_diag_client_notification.c
)

add_executable(apx-mon ${APX_MON_HEADERS} ${APX_MON_SOURCES})
target_link_libraries(apx-mon PRIVATE
    apx_core
    Threads::Threads
)

target_include_directories(apx-mon PRIVATE
    ${PROJECT_BINARY_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

install(
  TARGETS apx-mon
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
  COMPONENT App
)
