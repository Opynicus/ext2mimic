cmake_minimum_required(VERSION 3.21)
project(ext2mimic)

set(CMAKE_CXX_STANDARD 14)

add_executable(osFileManager main.cpp image.cpp image.h inode.h inode.cpp superBlock.cpp superBlock.h params.h fs.cpp fs.h bitMap.cpp bitMap.h)
