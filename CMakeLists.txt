cmake_minimum_required(VERSION 3.21)
project(ext2mimic)

set(CMAKE_CXX_STANDARD 14 "-static-libgcc -static-libstdc++")

add_executable(osFileManager main.cpp src/image.cpp include/image.h include/inode.h src/inode.cpp src/superBlock.cpp include/superBlock.h include/params.h src/fs.cpp include/fs.h include/Dir.h include/bitMap.h src/bitMap.cpp src/vimimic.cpp include/vimimic.h src/utils.cpp include/utils.h)
