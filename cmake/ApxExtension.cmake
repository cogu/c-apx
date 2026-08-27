# cmake/ApxExtension.cmake
include_guard(GLOBAL)

define_property(GLOBAL PROPERTY APX_SERVER_REGISTERED_TARGETS
    BRIEF_DOCS "List of CMake targets for APX server extensions"
    FULL_DOCS "List of CMake targets for APX server extensions to be linked into apx_server")

define_property(GLOBAL PROPERTY APX_SERVER_REGISTERED_INCLUDES
    BRIEF_DOCS "List of header include lines for APX server extensions"
    FULL_DOCS "List of #include lines for extensions_cfg.c")

define_property(GLOBAL PROPERTY APX_SERVER_REGISTERED_ENTRIES
    BRIEF_DOCS "List of C struct entries for APX server extensions"
    FULL_DOCS "List of struct entries for extensions_cfg.c")

function(apx_register_server_extension)
    set(options)
    set(oneValueArgs TARGET NAME HEADER REGISTER_FN)
    set(multiValueArgs)
    cmake_parse_arguments(EXT "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

    if(NOT EXT_TARGET OR NOT EXT_NAME OR NOT EXT_HEADER OR NOT EXT_REGISTER_FN)
        message(FATAL_ERROR "apx_register_server_extension requires TARGET, NAME, HEADER, and REGISTER_FN")
    endif()

    set_property(GLOBAL APPEND PROPERTY APX_SERVER_REGISTERED_TARGETS "${EXT_TARGET}")
    set_property(GLOBAL APPEND PROPERTY APX_SERVER_REGISTERED_INCLUDES "#include \"${EXT_HEADER}\"")
    set_property(GLOBAL APPEND PROPERTY APX_SERVER_REGISTERED_ENTRIES "   {\"${EXT_NAME}\", ${EXT_REGISTER_FN}},")
endfunction()

function(apx_generate_server_extension_registry OUTPUT_FILE)
    get_property(EXT_INCLUDES GLOBAL PROPERTY APX_SERVER_REGISTERED_INCLUDES)
    get_property(EXT_ENTRIES GLOBAL PROPERTY APX_SERVER_REGISTERED_ENTRIES)

    string(REPLACE ";" "\n" GENERATED_EXTENSION_INCLUDES "${EXT_INCLUDES}")
    string(REPLACE ";" "\n" GENERATED_EXTENSION_ENTRIES "${EXT_ENTRIES}")

    configure_file(
        "${CMAKE_SOURCE_DIR}/cmake/extensions_cfg.c.in"
        "${OUTPUT_FILE}"
        @ONLY
    )
endfunction()
