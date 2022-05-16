//
// Created by Opynicus on 2022/4/12.
//

#ifndef EXT2MIMIC_PARAMS_H
#define EXT2MIMIC_PARAMS_H

#include <iostream>
#include <ctime>
#include <cstring>
#include <fstream>
#include "windows.h"
#include "conio.h"
#include <vector>

using namespace std;

#define IMAGE_FILE_NAME "ext2.vhdx"

constexpr int MAX_FILE_NAME = 28;                               //最长文件名

constexpr int INODE_NUM = 640;                                   //Inode总数640个
constexpr int INODE_SIZE = 128;                                  //Inode大小（byte）128B
constexpr int BLOCK_SIZE = 512;                                 //Block大小（byte), 512B
constexpr int BLOCK_NUM = 10240;                                 //块号数为10240,大小为 10240 * 512B

constexpr int MAX_FREE_BLOCKS = 64;                              //空闲块堆栈大小，一个空闲堆栈最多能存多少个磁盘块地址

constexpr int BLOCK_NUM_PER_INODE = (INODE_NUM / (BLOCK_SIZE / INODE_SIZE));  //每个Inode节点占的磁盘块数(160)

constexpr int SUPERBLOCK_START_ADDR = 0;                             //超级块 偏移地址,占一个磁盘块
constexpr int BLOCK_BITMAP_START_ADDR = (1 * BLOCK_SIZE);            //Block位图 偏移地址，占20个磁盘块，最多监控 512 * 20 个磁盘块的状态
constexpr int INODE_BITMAP_START_ADDR = (BLOCK_BITMAP_START_ADDR +
    20 * BLOCK_SIZE);    //inode位图 偏移地址，占2个磁盘块，最多监控 512 * 2 个inode的状态
constexpr int INODE_TABLE_START_ADDR(
    INODE_BITMAP_START_ADDR + 2 * BLOCK_SIZE);    //inode节点区 偏移地址，占 BLOCK_NUM_PER_INODE 个磁盘块
constexpr int DATA_BLOCK_START_ADDR = (INODE_TABLE_START_ADDR + BLOCK_NUM_PER_INODE *
    BLOCK_SIZE);//block数据区 偏移地址 ，占 INODE_NUM 个磁盘块constexpr int ROOT_DIR_ADDR = INODE_TABLE_START_ADDR;
constexpr int ROOT_DIR_ADDR = INODE_TABLE_START_ADDR;

constexpr int Dir_ITEM_NUM_PER_BLOCK = (BLOCK_SIZE / sizeof(unsigned int) / 8);       //每个Block拥有的目录项数量
constexpr int BLOCK_ID0_NUM = 10;                                                    //Inode直接块数量

constexpr int ARGV_LEN = 100;
constexpr int MAX_USER_NAME = 120;
constexpr int MAX_PASSWD_LEN = 120;

#define MODE_DIR    01000                    //目录标识
#define MODE_FILE    00000                    //文件标识
#define FILE_DEF_PERMISSION 0664            //文件默认权限
#define DIR_DEF_PERMISSION    0755            //目录默认权限

#endif //EXT2MIMIC_PARAMS_H
