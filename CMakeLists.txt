cmake_minimum_required(VERSION 3.13)

include(pico_sdk_import.cmake)

project(midi-rate-conv)

pico_sdk_init()

add_executable(midi-rate-conv)

target_sources(midi-rate-conv PUBLIC
        ${CMAKE_CURRENT_LIST_DIR}/src/main.c
        ${CMAKE_CURRENT_LIST_DIR}/src/ring_buffer.c)

target_include_directories(midi-rate-conv PUBLIC
        ${CMAKE_CURRENT_LIST_DIR}/include
        )

target_link_libraries(midi-rate-conv PUBLIC
        pico_stdlib
        )

pico_add_extra_outputs(midi-rate-conv)
