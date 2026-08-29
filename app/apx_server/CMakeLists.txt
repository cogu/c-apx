cmake_minimum_required(VERSION 3.20)

project(apx_server LANGUAGES C)

# Retrieve list of all registered extension targets to link
get_property(APX_SERVER_EXT_LIBS GLOBAL PROPERTY APX_SERVER_REGISTERED_TARGETS)

add_executable(apx_server
  src/server_main.c
  src/server_cfg.h
  src/server_cfg.c
  src/extensions_cfg.h
  ${GENERATED_EXTENSIONS_CFG}
)

target_link_libraries(apx_server PRIVATE
  apx_core
  ${APX_SERVER_EXT_LIBS}
  Threads::Threads
)

if (LEAK_CHECK)
    target_compile_definitions(apx_server PRIVATE MEM_LEAK_CHECK)
endif()
target_compile_definitions(apx_server PRIVATE USE_CONFIGURATION_FILE)
target_include_directories(apx_server PRIVATE src ${PROJECT_BINARY_DIR} ${CMAKE_CURRENT_BINARY_DIR})

install(
  TARGETS apx_server
  RUNTIME DESTINATION bin
  COMPONENT Server
)
