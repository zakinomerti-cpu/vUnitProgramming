cmake_minimum_required(VERSION 4.3)
project(minimalGL C)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)

add_library(stdlib STATIC
        stdlib/heapAlloc/heap_cstdlib.c
        stdlib/vu_stringh_cstdlib.c
        stdlib/vu_stdio_cstdlib.c
)

add_library(core STATIC
        core/pVoidArray/pVoidArray.c
        core/memallocate/memallocate.c
)

add_library(opcodes STATIC
    opcodes/opcode.c
)

add_library(v_unit STATIC
        v_unit/v_unit.c
)

add_executable(main
    main.c
)

target_include_directories(core PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/stdlib
)

target_include_directories(opcodes PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/stdlib
        ${CMAKE_CURRENT_SOURCE_DIR}/v_unit
)

target_include_directories(v_unit PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/stdlib
        ${CMAKE_CURRENT_SOURCE_DIR}/opcodes
)

target_link_libraries(core PUBLIC stdlib)
target_link_libraries(opcodes PUBLIC stdlib)
target_link_libraries(v_unit PUBLIC opcodes)
target_link_libraries(main PRIVATE core opcodes v_unit)