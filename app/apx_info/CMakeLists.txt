cmake_minimum_required(VERSION 3.20)


project(apx-info LANGUAGES C)

set (APX_INFO_HEADERS
    ${CMAKE_CURRENT_SOURCE_DIR}/include/apx_connection.h
)

set (APX_INFO_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/src/apx_info_main.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/apx_connection.c
)

add_executable(apx-info ${APX_INFO_HEADERS} ${APX_INFO_SOURCES})
target_link_libraries(apx-info PRIVATE
    apx_core
    Threads::Threads
)

target_include_directories(apx-info PRIVATE
    ${PROJECT_BINARY_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)
if(APX_DEBUG)
    target_compile_definitions(apx-info PUBLIC APX_DEBUG_ENABLE=1)
endif()

install(
  TARGETS apx-info
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
  COMPONENT App
)