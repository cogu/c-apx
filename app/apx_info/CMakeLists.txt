cmake_minimum_required(VERSION 3.14)


project(apx_info LANGUAGES C)

set (APX_INFO_HEADERS
    ${CMAKE_CURRENT_SOURCE_DIR}/include/apx_connection.h
)

set (APX_INFO_HEADERS
    ${CMAKE_CURRENT_SOURCE_DIR}/src/apx_info_main.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/apx_connection.c
)

add_executable(apx_info ${APX_INFO_HEADERS} ${APX_INFO_HEADERS})
target_link_libraries(apx_info PRIVATE
    apx
    Threads::Threads
)

target_include_directories(apx_info PRIVATE
    ${PROJECT_BINARY_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)
target_compile_definitions(apx_info PRIVATE USE_CONFIGURATION_FILE)
if(APX_DEBUG)
    target_compile_definitions(apx_info PUBLIC APX_DEBUG_ENABLE=1)
endif()

install(
  TARGETS apx_info
  RUNTIME DESTINATION bin
  COMPONENT App
)