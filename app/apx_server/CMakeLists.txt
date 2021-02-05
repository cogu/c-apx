cmake_minimum_required(VERSION 3.14)

project(apx_server LANGUAGES C)

add_executable(apx_server
  src/server_main.c
  src/extensions_cfg.h
  src/extensions_cfg.c
)

target_link_libraries(apx_server PRIVATE
apx
apx_srv_sock_ext
Threads::Threads
)
if (LEAK_CHECK)
    target_compile_definitions(apx_server PRIVATE MEM_LEAK_CHECK)
endif()
target_compile_definitions(apx_server PRIVATE USE_CONFIGURATION_FILE)
target_include_directories(apx_server PRIVATE ${PROJECT_BINARY_DIR})

install(
  TARGETS apx_server
  RUNTIME DESTINATION bin
  COMPONENT Server
)
