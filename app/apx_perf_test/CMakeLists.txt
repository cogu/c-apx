cmake_minimum_required(VERSION 3.14)


project(apx_perf_test LANGUAGES C)

set (APX_PERF_TEST_HEADERS
    ${CMAKE_CURRENT_SOURCE_DIR}/src/application.h
)

set (APX_PERF_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/src/apx_perf_test_main.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/application.c
)

add_executable(apx_perf_test ${APX_PERF_TEST_HEADERS} ${APX_PERF_SOURCES})
target_link_libraries(apx_perf_test PRIVATE
    apx
    Threads::Threads
)

target_include_directories(apx_perf_test PRIVATE
    ${PROJECT_BINARY_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)
target_compile_definitions(apx_perf_test PRIVATE USE_CONFIGURATION_FILE)

install(
  TARGETS apx_perf_test
  RUNTIME DESTINATION bin
  COMPONENT App
)