cmake_minimum_required(VERSION 4.3)
project(virtcpu C)

set(CMAKE_C_STANDARD 99)

add_executable(virtcpu main.c
        v_unit.c)
