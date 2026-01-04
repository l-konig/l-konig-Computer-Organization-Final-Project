cmake_minimum_required(VERSION 3.16)
project(CompOrgFinalProject C)

set(CMAKE_C_STANDARD 11)

# === Tests executable ===
add_executable(tests test_my_scanf.c my_scanf.c)
