cmake_minimum_required(VERSION 4.1)
project(CompOrgFinalProject C)

set(CMAKE_C_STANDARD 11)

# === Main executable (if you still need it) ===
add_executable(CompOrgFinalProject main.c my_scanf.c)

# === Tests executable ===
add_executable(tests tests.c my_scanf.c)
