cmake_minimum_required(VERSION 3.20)

project(apx-sign LANGUAGES C)

set(APX_SIGN_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/src/apx_sign_main.c
)

add_executable(apx-sign ${APX_SIGN_SOURCES})
target_link_libraries(apx-sign PRIVATE
    apx_core
    Threads::Threads
)

target_include_directories(apx-sign PRIVATE
    ${PROJECT_BINARY_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

install(
  TARGETS apx-sign
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
  COMPONENT App
)
