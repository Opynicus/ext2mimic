cmake_minimum_required(VERSION 3.21)
project(ext2mimic)

set(CMAKE_CXX_STANDARD 14)

add_executable(osFileManager main.cpp image.cpp image.h superBlock.cpp superBlock.h params.h "test/write_head_superBlock_2_image(initial).cpp")
