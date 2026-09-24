cmake_minimum_required(VERSION 3.20)


project(apx-perf-test LANGUAGES C)

set (APX_PERF_TEST_HEADERS
    ${CMAKE_CURRENT_SOURCE_DIR}/src/application.h
)

set (APX_PERF_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/src/apx_perf_test_main.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/application.c
)

add_executable(apx-perf-test ${APX_PERF_TEST_HEADERS} ${APX_PERF_SOURCES})
target_link_libraries(apx-perf-test PRIVATE
    apx_core
    Threads::Threads
)

target_include_directories(apx-perf-test PRIVATE
    ${PROJECT_BINARY_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

install(
  TARGETS apx-perf-test
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
  COMPONENT App
)