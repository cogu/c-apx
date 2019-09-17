cmake_minimum_required(VERSION 3.10)
project(apx VERSION 0.3.0)
enable_language(C)

set(CMAKE_C_STANDARD 90)
set(REPO_ROOT ${CMAKE_CURRENT_SOURCE_DIR}/../../..)
add_library(adt STATIC
    ${REPO_ROOT}/adt/src/adt_ary.c
    ${REPO_ROOT}/adt/src/adt_bytearray.c
    ${REPO_ROOT}/adt/src/adt_bytes.c
    ${REPO_ROOT}/adt/src/adt_hash.c
    ${REPO_ROOT}/adt/src/adt_list.c
    ${REPO_ROOT}/adt/src/adt_ringbuf.c
    ${REPO_ROOT}/adt/src/adt_set.c
    ${REPO_ROOT}/adt/src/adt_stack.c
    ${REPO_ROOT}/adt/src/adt_str.c    
)

target_include_directories(adt PUBLIC
    ${REPO_ROOT}/adt/inc
)

target_compile_definitions(adt PRIVATE UNIT_TEST ADT_RBFH_ENABLE=1)

#add_executable(apx_unit_test
#    apx/common/test/test_main.c
#)

#target_link_libraries(apx_unit_test PRIVATE
#    adt
#)